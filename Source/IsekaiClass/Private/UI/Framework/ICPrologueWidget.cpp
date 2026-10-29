#include "UI/Framework/ICPrologueWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Settings/ICGameUserSettings.h"
#include "Audio/ICBGMSubsystem.h"
#include "UI/Gameplay/ICMovementCooldownWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
class FICPrologueInput final : public IInputProcessor
{
public:
    explicit FICPrologueInput(UICPrologueWidget* Widget):Owner(Widget){}
    virtual void Tick(const float,FSlateApplication&,TSharedRef<ICursor>) override {}
    virtual bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& Event) override
    {return Owner.IsValid()&&Owner->IsActivated()&&Owner->HandleProgressKey(Event);}
    virtual bool HandleKeyUpEvent(FSlateApplication&,const FKeyEvent& Event) override
    {return Owner.IsValid()&&Owner->IsActivated()&&Owner->HandleProgressKeyUp(Event);}
private:
    TWeakObjectPtr<UICPrologueWidget> Owner;
};
}

UICPrologueWidget* UICPrologueWidget::Show(AICPlayerController* PC,bool RevealWorld)
{
    if(!PC||!PC->GetLocalPlayer())return nullptr;
    auto* UI=PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
    auto* Type=LoadClass<UICPrologueWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_Prologue.WBP_IC_Prologue_C"));
    if(!Type)return nullptr;
    auto* Widget=CreateWidget<UICPrologueWidget>(PC,Type);if(!Widget)return nullptr;
    Widget->State=RevealWorld?EState::Revealing:EState::FadeIn;
    Widget->SetRenderOpacity(RevealWorld?1.f:0.f);
    auto* Flow=PC->GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    Flow->Phase=EICGameFlowPhase::Prologue;
    // Mount before closing the prior screen, so no uncovered frame reaches the viewport.
    UI->OpenScreen(Widget,true,20000);
    if(RevealWorld)PC->SetMenu(TEXT("Prologue"));
    Widget->SetUserFocus(PC);
    return Widget;
}
void UICPrologueWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);BaseFontSize=LineText->GetFont().Size;
    if(auto* Table=ScriptTable.LoadSynchronous())
    {
        TArray<FICPrologueRow*> Rows;Table->GetAllRows(TEXT("Prologue"),Rows);
        for(const auto* Row:Rows)Script.Add(*Row);
        Script.StableSort([](const auto& A,const auto& B){return A.Order<B.Order;});
    }
    if(auto* Table=PresentationTable.LoadSynchronous())if(auto* Row=Table->FindRow<FICProloguePresentationRow>(TEXT("Default"),TEXT("Prologue"),false))Presentation=*Row;
    SetRenderOpacity(State==EState::Revealing?1.f:0.f);
}
void UICPrologueWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if(auto* Music=GetWorld()->GetSubsystem<UICBGMSubsystem>())Music->SetPrologueMusic(Presentation.InitialMusicRow);
    SkipGauge->SetProgress(0);
    SkipGauge->SetVisibility(State==EState::Revealing?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    if(State!=EState::Revealing)StartLine();
    if(!ProgressInput&&FSlateApplication::IsInitialized())
    {
        ProgressInput=MakeShared<FICPrologueInput>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(ProgressInput,0);
        FSlateApplication::Get().OnApplicationActivationStateChanged().AddWeakLambda(this,[this](bool Active)
        {if(!Active){bSkipHeld=false;SkipElapsed=0;if(SkipGauge)SkipGauge->SetProgress(0);}});
    }
}
void UICPrologueWidget::NativeDestruct()
{
    if(ProgressInput&&FSlateApplication::IsInitialized())FSlateApplication::Get().UnregisterInputPreProcessor(ProgressInput);
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().OnApplicationActivationStateChanged().RemoveAll(this);
    ProgressInput.Reset();Super::NativeDestruct();
}
UWidget* UICPrologueWidget::NativeGetDesiredFocusTarget() const {return const_cast<UICPrologueWidget*>(this);}
bool UICPrologueWidget::HandleProgressKey(const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::H||Event.GetKey()==EKeys::Gamepad_FaceButton_Top)
    {if(State==EState::Reading||State==EState::FadeIn)bSkipHeld=true;return true;}
    if(Event.GetKey()!=EKeys::G&&!UICGameUserSettings::IsGamepadAcceptKey(Event.GetKey()))return false;
    if(!Event.IsRepeat())Advance();return true;
}
bool UICPrologueWidget::HandleProgressKeyUp(const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::H||Event.GetKey()==EKeys::Gamepad_FaceButton_Top)
    {bSkipHeld=false;SkipElapsed=0;if(SkipGauge)SkipGauge->SetProgress(0);return true;}
    return Event.GetKey()==EKeys::G||UICGameUserSettings::IsGamepadAcceptKey(Event.GetKey());
}
void UICPrologueWidget::StartLine()
{
    Characters=0;Typing=0;bSoundPlayed=false;LineText->SetText(FText::GetEmpty());Prompt->SetText(FText::GetEmpty());
    if(!Script.IsValidIndex(LineIndex)){FinishReading();return;}
    const auto& Row=Script[LineIndex];
    if(auto* Music=GetWorld()->GetSubsystem<UICBGMSubsystem>())
    {
        if(Row.MusicAction==EICPrologueMusicAction::Play)Music->SetPrologueMusic(Row.MusicRow);
        else if(Row.MusicAction==EICPrologueMusicAction::Stop)Music->SetPrologueMusic(NAME_None);
    }
}
void UICPrologueWidget::FinishReading()
{
    if(State==EState::Traveling||State==EState::Revealing||State==EState::FadeOut)return;
    State=EState::FadeOut;Elapsed=0;bSkipHeld=false;SkipElapsed=0;
    Prompt->SetText(FText::GetEmpty());SkipPrompt->SetText(FText::GetEmpty());SkipGauge->SetVisibility(ESlateVisibility::Collapsed);
}
bool UICPrologueWidget::Advance()
{
    if(!IsLineComplete())return false;
    if(LineIndex+1==Script.Num()){FinishReading();return true;}
    ++LineIndex;StartLine();return true;
}
void UICPrologueWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    const float Fade=FMath::Max(.1f,Presentation.FadeSeconds);
    Elapsed+=DeltaSeconds;
    if(State==EState::FadeIn)
    {SetRenderOpacity(FMath::Clamp(Elapsed/Fade,0.f,1.f));if(Elapsed>=Fade){State=EState::Reading;Elapsed=0;if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SetMenu(TEXT("Prologue"));}return;}
    if(State==EState::Reading)
    {
        if(!Script.IsValidIndex(LineIndex)){FinishReading();return;}
        const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
        SkipPrompt->SetText(PC&&PC->IsUsingGamepad()?Presentation.GamepadSkipPrompt:Presentation.SkipPrompt);
        SkipElapsed=bSkipHeld?SkipElapsed+DeltaSeconds:0.f;
        const float SkipProgress=FMath::Clamp(SkipElapsed/FMath::Max(.1f,Presentation.SkipHoldSeconds),0.f,1.f);
        SkipGauge->SetProgress(SkipProgress);
        if(SkipProgress>=1.f){FinishReading();return;}
        const auto& Row=Script[LineIndex];const FString Text=Row.Text.ToString();
        if(!bSoundPlayed&&Typing>=FMath::Max(0.f,Row.SoundDelay))
        {
            bSoundPlayed=true;
            if(auto* Sound=Row.Sound.LoadSynchronous())UGameplayStatics::PlaySound2D(this,Sound,Row.SoundVolume);
        }
        // Keep the complete sentence on one line, including at small viewport sizes.
        const float Width=Geometry.GetLocalSize().X;
        const float FontSize=FMath::Min(BaseFontSize,Width*.88f/FMath::Max(1,Text.Len()));
        auto Font=LineText->GetFont();Font.Size=FMath::Max(12,FMath::FloorToInt(FontSize));LineText->SetFont(Font);
        Typing+=DeltaSeconds;
        const int32 Count=FMath::Min(Text.Len(),FMath::FloorToInt(Typing/FMath::Max(.001f,Row.CharacterInterval>0?Row.CharacterInterval:Presentation.CharacterInterval)));
        if(Count!=Characters){Characters=Count;LineText->SetText(FText::FromString(Text.Left(Characters)));}
        if(IsLineComplete())
        {
            Prompt->SetText(PC&&PC->IsUsingGamepad()?Presentation.GamepadNextPrompt:Presentation.NextPrompt);
        }
        return;
    }
    if(State==EState::FadeOut)
    {
        LineText->SetRenderOpacity(1.f-FMath::Clamp(Elapsed/Fade,0.f,1.f));
        if(Elapsed>=Fade)
        {
            State=EState::Traveling;
            auto* PC=Cast<AICPlayerController>(GetOwningPlayer());auto* Flow=GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
            if(!Flow->BeginNewGame(PC))
            {PC->SetMenu(TEXT("Loading"));GetOwningLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>()->CloseScreen(this);}
        }
        return;
    }
    if(State==EState::Revealing)
    {
        SetRenderOpacity(1.f-FMath::Clamp(Elapsed/Fade,0.f,1.f));
        if(Elapsed>=Fade)
        {
            State=EState::Traveling;
            GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->CompleteNewGame();
            GetOwningLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>()->CloseScreen(this);
            if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SetMenu(TEXT(""));
        }
    }
}
FReply UICPrologueWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    HandleProgressKey(Event);
    return FReply::Handled();
}

FReply UICPrologueWidget::NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event)
{HandleProgressKeyUp(Event);return FReply::Handled();}
