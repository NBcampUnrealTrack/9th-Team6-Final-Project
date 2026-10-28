#include "UI/Framework/ICPrologueWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Settings/ICGameUserSettings.h"
#include "Blueprint/WidgetTree.h"
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
    {return Owner.IsValid()&&Owner->IsActivated()&&(Event.GetKey()==EKeys::G||UICGameUserSettings::IsGamepadAcceptKey(Event.GetKey()));}
private:
    TWeakObjectPtr<UICPrologueWidget> Owner;
};
}

const TArray<FString>& UICPrologueWidget::Lines()
{
    static const TArray<FString> Text={
        TEXT("나는 어느 날 갑자기 이세계에 떨어졌다."),
        TEXT("이유도, 돌아갈 방법도 알 수 없었다."),
        TEXT("어디선가 봤던 숲과 하늘, 그리고 사람을 공격하는 낯익은 드래곤들까지."),
        TEXT("모든 게 유행하던 게임 속에서 보던 것들이었다."),
        TEXT("다행히 숲에서 쓰러진 나를 마을 사람들이 발견해 주었다."),
        TEXT("그들의 도움으로 목숨을 건졌고, 작은 시골 마을에 정착하게 되었다."),
        TEXT("처음에는 어떻게든 원래 세계로 돌아가려고 했다."),
        TEXT("하지만 아무리 찾아봐도 돌아갈 방법은 없었다."),
        TEXT("그렇게 하루가 지나고, 한 달이 지나고..."),
        TEXT("어느새 3년이라는 시간이 흘렀다."),
        TEXT("이제는 몬스터도, 마법도 그다지 놀랍지 않다."),
        TEXT("마을 사람들과도 친해졌고, 이곳의 생활에도 제법 익숙해졌다."),
        TEXT("물론 원래 세계가 그립지 않은 건 아니다."),
        TEXT("그래도 당장은 어쩔 수 없으니까."),
        TEXT("오늘도 평소와 다름없는 하루가 시작된다."),
        TEXT("그런데 그날, 3년 동안 꺼져 있던 스마트폰에서 익숙한 알림음이 울렸다."),
        TEXT("믿을 수 없게도, 화면에는 예전에 사용하던 반 단체 채팅방이 떠 있었다."),
        TEXT("친구1 : 야!!!! 살아 있는 사람 있냐?!?!"),
        TEXT("친구10 : 얘들아ㅠㅠ 나 진짜 무서워..."),
        TEXT("친구2 : 일단 진정해. 지금 상황부터 파악하자."),
        TEXT("친구1 : 아니, 여긴 대체 어디냐고! 몬스터까지 돌아다닌다니까?!"),
        TEXT("플레이어 : 잠깐... 너희야?"),
        TEXT("친구1 : 야!!! 너 살아 있었냐?!?!"),
        TEXT("친구3 : 헐!! 진짜 너야?!ㅠㅠ"),
        TEXT("친구2 : 너도 이 세계에 있는 거야?"),
        TEXT("플레이어 : 응. 난 여기서 벌써 3년째 살고 있어."),
        TEXT("친구1 : 3년?! 뭐야, 너 완전 이세계 고인물이네?!"),
        TEXT("플레이어 : 너희는 지금 어디 있어?"),
        TEXT("친구3 : 어떤 착한 언니가 구해줘서 여관에 있어!"),
        TEXT("친구2 : 시골 마을에 있는 여관이야. 정확한 위치는 모르겠어."),
        TEXT("플레이어 : 혹시 여관주인 머리가 회색이야?"),
        TEXT("친구3 : 응응!! 맞아!! 어떻게 알았어?!"),
        TEXT("플레이어 : ...너희 지금 내가 사는 마을에 있는 것 같은데?"),
        TEXT("친구1 : 진짜?! 그럼 당장 만나자!!"),
        TEXT("친구2 : 좋아. 만나서 앞으로 어떻게 할지 이야기하자."),
        TEXT("친구3 : 빨리 와아!! 기다리고 있을게!"),
        TEXT("친구1 : 아, 맞다! 혹시 Y랑도 연락됐어?"),
        TEXT("플레이어 : Y? 아직인데. 왜?"),
        TEXT("친구2 : 걔가 예전에 읽었다던 소설 기억나?"),
        TEXT("친구2 : 아무래도 우리가 떨어진 세계가 그 소설이랑 관련 있는 것 같아."),
        TEXT("플레이어 : ...뭐? 그게 무슨 소리야?"),
        TEXT("[연결이 끊어졌습니다.]"),
        TEXT("갑자기 스마트폰 화면이 꺼졌다."),
        TEXT("몇 번이고 전원을 눌러 봤지만 아무런 반응도 없었다."),
        TEXT("...대체 무슨 일이 일어나고 있는 거지?"),
        TEXT("그 순간, 눈앞에 처음 보는 글자가 나타났다."),
        TEXT("[시스템을 활성화합니다.]"),
        TEXT("[사용자 확인 완료.]"),
        TEXT("[상태 정보 동기화 완료.]"),
        TEXT("[새로운 퀘스트가 등록되었습니다.]"),
        TEXT("[퀘스트 01. 이세계에 온 친구]"),
        TEXT("...상태창? 3년 동안 한 번도 본 적 없었는데."),
        TEXT("갑자기 나타난 상태창도, Y가 말한 소설도 신경 쓰인다."),
        TEXT("하지만 지금은 그보다 중요한 일이 있다."),
        TEXT("우선 친구들부터 만나러 가자.")
    };
    return Text;
}
UICPrologueWidget* UICPrologueWidget::Show(AICPlayerController* PC,bool RevealWorld)
{
    if(!PC||!PC->GetLocalPlayer())return nullptr;
    auto* UI=PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
    auto* Widget=CreateWidget<UICPrologueWidget>(PC,StaticClass());
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
    Super::NativeOnInitialized();SetIsFocusable(true);
    auto* Black=WidgetTree->ConstructWidget<UBorder>();Black->SetBrushColor(FLinearColor::Black);Black->SetPadding(FMargin(0));WidgetTree->RootWidget=Black;
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Black->SetContent(Canvas);
    LineText=WidgetTree->ConstructWidget<UTextBlock>();LineText->SetFont(ICFrontendLayout::Font(27));
    LineText->SetColorAndOpacity(FLinearColor(.94f,.94f,.94f));LineText->SetJustification(ETextJustify::Center);LineText->SetAutoWrapText(false);
    auto* LineSlot=Canvas->AddChildToCanvas(LineText);LineSlot->SetAnchors(FAnchors(.5f,.5f));LineSlot->SetAlignment(FVector2D(.5f,.5f));LineSlot->SetAutoSize(true);
    Prompt=WidgetTree->ConstructWidget<UTextBlock>();Prompt->SetFont(ICFrontendLayout::Font(16));Prompt->SetColorAndOpacity(FLinearColor(.65f,.65f,.65f));
    auto* Hint=Canvas->AddChildToCanvas(Prompt);Hint->SetAnchors(FAnchors(.5f,.72f));Hint->SetAlignment(FVector2D(.5f,.5f));Hint->SetAutoSize(true);
    SetRenderOpacity(State==EState::Revealing?1.f:0.f);
}
void UICPrologueWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if(!ProgressInput&&FSlateApplication::IsInitialized())
    {
        ProgressInput=MakeShared<FICPrologueInput>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(ProgressInput,0);
    }
}
void UICPrologueWidget::NativeDestruct()
{
    if(ProgressInput&&FSlateApplication::IsInitialized())FSlateApplication::Get().UnregisterInputPreProcessor(ProgressInput);
    ProgressInput.Reset();Super::NativeDestruct();
}
UWidget* UICPrologueWidget::NativeGetDesiredFocusTarget() const {return const_cast<UICPrologueWidget*>(this);}
bool UICPrologueWidget::HandleProgressKey(const FKeyEvent& Event)
{
    if(Event.GetKey()!=EKeys::G&&!UICGameUserSettings::IsGamepadAcceptKey(Event.GetKey()))return false;
    if(!Event.IsRepeat())Advance();
    return true;
}
bool UICPrologueWidget::Advance()
{
    if(!IsLineComplete())return false;
    Prompt->SetText(FText::GetEmpty());
    if(LineIndex+1==Lines().Num()){State=EState::FadeOut;Elapsed=0;return true;}
    ++LineIndex;Characters=0;Typing=0;LineText->SetText(FText::GetEmpty());return true;
}
void UICPrologueWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    constexpr float Fade=1.2f;
    Elapsed+=DeltaSeconds;
    if(State==EState::FadeIn)
    {SetRenderOpacity(FMath::Clamp(Elapsed/Fade,0.f,1.f));if(Elapsed>=Fade){State=EState::Reading;Elapsed=0;if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SetMenu(TEXT("Prologue"));}return;}
    if(State==EState::Reading)
    {
        // Keep the complete sentence on one line, including at small viewport sizes.
        const float Width=Geometry.GetLocalSize().X;
        const float FontSize=FMath::Min(27.f,Width*.88f/FMath::Max(1,Lines()[LineIndex].Len()));
        LineText->SetFont(ICFrontendLayout::Font(FMath::Max(12,FMath::FloorToInt(FontSize))));
        Typing+=DeltaSeconds;
        const int32 Count=FMath::Min(Lines()[LineIndex].Len(),FMath::FloorToInt(Typing/.045f));
        if(Count!=Characters){Characters=Count;LineText->SetText(FText::FromString(Lines()[LineIndex].Left(Characters)));}
        if(IsLineComplete())
        {
            const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
            Prompt->SetText(PC&&PC->IsUsingGamepad()?NSLOCTEXT("ICPrologue","PadNext","확인 버튼 · 다음 대사"):NSLOCTEXT("ICPrologue","Next","G · 다음 대사"));
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
