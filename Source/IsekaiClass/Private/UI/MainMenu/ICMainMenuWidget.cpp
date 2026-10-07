#include "UI/MainMenu/ICMainMenuWidget.h"
#include "Debug/ICLog.h"
#include "UI/Save/ICSaveLoadWidget.h"
#include "Save/ICSaveSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    FICMainMenuImages DefaultImages(const TCHAR* Name, bool bHasDisabled)
    {
        IC_TRACE_FUNCTION();
        FICMainMenuImages Result;
        const FString Root = TEXT("/Game/IsekaiClass/UI/Images/MainMenu/");
        const FString Base = FString(TEXT("IC_Main_")) + Name;
        auto Path = [&Root, &Base](const TCHAR* Suffix)
        {
            IC_TRACE_FUNCTION();
            const FString Asset = Base + Suffix;
            return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Root + Asset + TEXT(".") + Asset));
        };
        Result.Normal = Path(TEXT(""));
        Result.Hover = Path(TEXT("_Hover"));
        Result.Pressed = Path(TEXT("_Pressed"));
        if (bHasDisabled) Result.Disabled = Path(TEXT("_Disabled"));
        return Result;
    }
}

UICMainMenuWidget::UICMainMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    IC_TRACE_FUNCTION();
    ContinueImages = DefaultImages(TEXT("Continue"), true);
    StartImages = DefaultImages(TEXT("Start"), false);
    LoadImages = DefaultImages(TEXT("Load"), false);
    QuitImages = DefaultImages(TEXT("Quit"), false);
}

void UICMainMenuWidget::NativeOnInitialized()
{
    IC_TRACE_FUNCTION();
    Super::NativeOnInitialized();
    if (BTN_Continue) BTN_Continue->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleContinueClicked);
    if (BTN_Start) BTN_Start->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStartClicked);
    if (BTN_Load) BTN_Load->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleLoadClicked);
    if (BTN_Settings) BTN_Settings->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSettingsClicked);
    if (BTN_Quit) BTN_Quit->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
}

void UICMainMenuWidget::NativeConstruct()
{
    IC_TRACE_FUNCTION();
    Super::NativeConstruct();
    PendingAction = EExitAction::None;
    FadeElapsed = 0.f;
    UE_LOG(LogICUI, Log, TEXT("[MainMenu.Construct] Widget=%s StartMap=%s LoadWidgetClass=%s"),*GetNameSafe(this),*StartMap.ToSoftObjectPath().ToString(),*GetNameSafe(SaveLoadWidgetClass.Get()));
    AnimationSeconds = 0.0;
    SetRenderOpacity(1.f);
    SetIsEnabled(true);
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UICSaveSubsystem* Saves = GI->GetSubsystem<UICSaveSubsystem>())
        {
            Saves->SetAutoSaveSuspended(true);
            Saves->SetSafetyState(FICSaveSafety());
            Saves->OnSaveFinished.AddUniqueDynamic(this, &ThisClass::HandleSaveFinished);
        }
        if (UICTimeSubsystem* Time = GI->GetSubsystem<UICTimeSubsystem>())
        { Time->PrepareForTravel(); }
    }
    if (IMG_MainFogA) FogBaseA = IMG_MainFogA->GetRenderTransform().Translation;
    if (IMG_MainFogB) FogBaseB = IMG_MainFogB->GetRenderTransform().Translation;
    ResolveButtonImages();
    SetNotice(FText::GetEmpty());
    RefreshContinueAvailability();
    UpdateAmbientImages();
}

void UICMainMenuWidget::NativeDestruct()
{
    IC_TRACE_FUNCTION();
    if (UGameInstance* GI = GetGameInstance())
        if (UICSaveSubsystem* Saves = GI->GetSubsystem<UICSaveSubsystem>())
            Saves->OnSaveFinished.RemoveDynamic(this, &ThisClass::HandleSaveFinished);
    if (SaveLoadScreen)
    {
        SaveLoadScreen->OnClosed.RemoveDynamic(this, &ThisClass::HandleSaveLoadClosed);
        SaveLoadScreen->RemoveFromParent();
        SaveLoadScreen = nullptr;
    }
    if (IMG_MainFogA) IMG_MainFogA->SetRenderTranslation(FogBaseA);
    if (IMG_MainFogB) IMG_MainFogB->SetRenderTranslation(FogBaseB);
    Super::NativeDestruct();
    UE_LOG(LogICUI, Verbose, TEXT("[MainMenu.Destruct] Widget=%s: 저장 이벤트 구독 정리"),*GetNameSafe(this));
}

void UICMainMenuWidget::ResolveButtonImages()
{
    IC_TRACE_FUNCTION();
    ResolvedImages.Reset();
    LastImageStates.Init(INDEX_NONE, 4);
    FICMainMenuImages* Themes[] = { &ContinueImages, &StartImages, &LoadImages, &QuitImages };
    UImage* Labels[] = { IMG_Continue.Get(), IMG_Start.Get(), IMG_Load.Get(), IMG_Quit.Get() };
    UButton* Buttons[] = { BTN_Continue.Get(), BTN_Start.Get(), BTN_Load.Get(), BTN_Quit.Get() };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        UTexture2D* Normal = Themes[Index]->Normal.LoadSynchronous();
        if (!Normal && Labels[Index]) Normal = Cast<UTexture2D>(Labels[Index]->GetBrush().GetResourceObject());
        ResolvedImages.Add(Normal);
        ResolvedImages.Add(Themes[Index]->Hover.LoadSynchronous());
        ResolvedImages.Add(Themes[Index]->Pressed.LoadSynchronous());
        ResolvedImages.Add(Themes[Index]->Disabled.LoadSynchronous());
        if (Labels[Index])
        {
            Labels[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
            Labels[Index]->SetColorAndOpacity(FLinearColor::White);
            Labels[Index]->SetRenderOpacity(1.f);
            // Child Image draws the text and highlight. Remove old button chrome.
            if (Buttons[Index])
            {
                FButtonStyle Style = Buttons[Index]->GetStyle();
                Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
                Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
                Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
                Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
                Style.NormalPadding = FMargin(0.f);
                Style.PressedPadding = FMargin(0.f);
                Style.NormalForeground = FSlateColor(FLinearColor::White);
                Style.HoveredForeground = FSlateColor(FLinearColor::White);
                Style.PressedForeground = FSlateColor(FLinearColor::White);
                Style.DisabledForeground = FSlateColor(FLinearColor::White);
                Buttons[Index]->SetStyle(Style);
                Buttons[Index]->SetBackgroundColor(FLinearColor::White);
            }
        }
    }
}

void UICMainMenuWidget::UpdateButtonImages()
{
    IC_TRACE_FUNCTION();
    UButton* Buttons[] = { BTN_Continue.Get(), BTN_Start.Get(), BTN_Load.Get(), BTN_Quit.Get() };
    UImage* Labels[] = { IMG_Continue.Get(), IMG_Start.Get(), IMG_Load.Get(), IMG_Quit.Get() };
    APlayerController* PC = GetOwningPlayer();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        if (!Buttons[Index] || !Labels[Index] || !LastImageStates.IsValidIndex(Index)) continue;
        const bool bFocused = PC && Buttons[Index]->HasUserFocus(PC);
        const int32 State = !Buttons[Index]->GetIsEnabled() ? 3 : Buttons[Index]->IsPressed() ? 2 : (Buttons[Index]->IsHovered() || bFocused) ? 1 : 0;
        if (LastImageStates[Index] == State) continue;
        LastImageStates[Index] = State;
        UTexture2D* Texture = ResolvedImages.IsValidIndex(Index * 4 + State) ? ResolvedImages[Index * 4 + State].Get() : nullptr;
        if (!Texture && ResolvedImages.IsValidIndex(Index * 4)) Texture = ResolvedImages[Index * 4].Get();
        if (Texture) Labels[Index]->SetBrushFromTexture(Texture, false);
        // Disabled PNG already contains reduced alpha. Do not dim it twice here.
        const bool bMissingDisabled = State == 3 && !ResolvedImages[Index * 4 + 3];
        Labels[Index]->SetRenderOpacity(bMissingDisabled ? 0.32f : 1.f);
    }
}

void UICMainMenuWidget::RefreshContinueAvailability()
{
    IC_TRACE_FUNCTION();
    bContinueAvailable = false;
    bContinueBusy = false;
    if (UGameInstance* GI = GetGameInstance())
        if (UICSaveSubsystem* Saves = GI->GetSubsystem<UICSaveSubsystem>())
        {
            bContinueBusy = Saves->bSaving;
            const TArray<FICSaveSlotInfo> Slots = Saves->GetSlotList();
            const FICSaveSlotInfo* Auto = Slots.FindByPredicate([](const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION(); return Info.Slot == 0; });
            bContinueAvailable = Auto && Auto->bValid;
        }
    if (BTN_Continue) BTN_Continue->SetIsEnabled(bContinueAvailable && !bContinueBusy);
    UpdateButtonImages();
}

void UICMainMenuWidget::FocusStartButton()
{
    IC_TRACE_FUNCTION();
    UButton* Initial = BTN_Continue && BTN_Continue->GetIsEnabled() ? BTN_Continue.Get() : BTN_Start.Get();
    if (Initial && GetOwningPlayer()) Initial->SetUserFocus(GetOwningPlayer());
    UpdateButtonImages();
}

void UICMainMenuWidget::SetNotice(const FText& Text)
{
    IC_TRACE_FUNCTION();
    if (IC_Main_Notice) IC_Main_Notice->SetText(Text);
    if (!Text.IsEmpty()) UE_LOG(LogICUI, Verbose, TEXT("[MainMenu.Notice] %s"), *Text.ToString());
}

bool UICMainMenuWidget::HasValidStartMap() const
{
    IC_TRACE_FUNCTION();
    return !StartMap.IsNull() && !StartMap.ToSoftObjectPath().GetLongPackageName().IsEmpty();
}

void UICMainMenuWidget::HandleContinueClicked()
{
    IC_TRACE_FUNCTION();
    if (PendingAction != EExitAction::None || SaveLoadScreen) return;
    RefreshContinueAvailability();
    if (!bContinueAvailable || bContinueBusy)
    { SetNotice(NSLOCTEXT("ICMainMenu", "NoAuto", "불러올 수 있는 자동 저장 데이터가 없습니다.")); return; }
    if (!HasValidStartMap())
    { SetNotice(NSLOCTEXT("ICMainMenu", "NoStartMap", "Start Map을 지정하세요.")); return; }
    BeginExit(EExitAction::Continue);
}

void UICMainMenuWidget::HandleStartClicked()
{
    IC_TRACE_FUNCTION();
    if (PendingAction != EExitAction::None || SaveLoadScreen) return;
    if (!HasValidStartMap())
    { SetNotice(NSLOCTEXT("ICMainMenu", "NoStartMap", "Start Map을 지정하세요.")); return; }
    if (UGameInstance* GI = GetGameInstance())
        if (UICSaveSubsystem* Saves = GI->GetSubsystem<UICSaveSubsystem>())
            if (Saves->bSaving) { SetNotice(NSLOCTEXT("ICMainMenu", "Saving", "저장이 끝난 뒤 다시 선택하세요.")); return; }
    BeginExit(EExitAction::NewGame);
}

void UICMainMenuWidget::BeginExit(EExitAction Action)
{
    IC_TRACE_FUNCTION();
    PendingAction = Action;
    UE_LOG(LogICUI, Log, TEXT("[MainMenu.ExitRequested] Action=%d Map=%s"),static_cast<int32>(Action),*StartMap.ToSoftObjectPath().ToString());
    FadeElapsed = 0.f;
    SetNotice(FText::GetEmpty());
    SetIsEnabled(false);
    if (ExitFadeSeconds <= 0.f) CompleteExit();
}

void UICMainMenuWidget::CompleteExit()
{
    IC_TRACE_FUNCTION();
    const EExitAction Action = PendingAction;
    PendingAction = EExitAction::None;
    if (Action == EExitAction::Quit)
    { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); SetRenderOpacity(1.f); SetIsEnabled(true); return; }
    UGameInstance* GI = GetGameInstance();
    UICSaveSubsystem* Saves = GI ? GI->GetSubsystem<UICSaveSubsystem>() : nullptr;
    UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    UICTimeSubsystem* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr;
    bool bSuccess = HasValidStartMap() && Saves && State && Time && !Saves->bSaving;
    if (bSuccess && Action == EExitAction::Continue) bSuccess = Saves->LoadSlot(0);
    else if (bSuccess && Action == EExitAction::NewGame)
    {
        // Reset the in-memory session only. No save file is deleted or overwritten here.
        bSuccess = State->RestoreState(FICWorldState());
        if (bSuccess) { Time->ResetClock(); Saves->ResetSession(); }
    }
    else bSuccess = false;
    if (!bSuccess)
    {
        UE_LOG(LogICUI, Warning, TEXT("[MainMenu.ExitFailed] Action=%d Map=%s Saves=%s State=%s Time=%s"),static_cast<int32>(Action),*StartMap.ToSoftObjectPath().ToString(),*GetNameSafe(Saves),*GetNameSafe(State),*GetNameSafe(Time));
        SetRenderOpacity(1.f); SetIsEnabled(true); RefreshContinueAvailability(); FocusStartButton();
        SetNotice(Action == EExitAction::Continue && Saves ? FText::FromString(Saves->GetLastMessage()) : NSLOCTEXT("ICMainMenu", "SessionFailed", "게임을 시작할 수 없습니다. Start Map과 저장 시스템 설정을 확인하세요."));
        return;
    }
    Saves->SetAutoSaveSuspended(true);
    Time->PrepareForTravel();
    UGameplayStatics::SetGamePaused(this, false);
    if (APlayerController* PC = GetOwningPlayer()) { PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor = false; }
    UE_LOG(LogICUI, Log, TEXT("[MainMenu.Travel] Action=%d Map=%s"),static_cast<int32>(Action),*StartMap.ToSoftObjectPath().ToString());
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, StartMap);
}

void UICMainMenuWidget::HandleLoadClicked()
{
    IC_TRACE_FUNCTION();
    if (PendingAction != EExitAction::None || SaveLoadScreen) return;
    if (!SaveLoadWidgetClass)
    { SetNotice(NSLOCTEXT("ICMainMenu", "NoLoadClass", "Save Load Widget Class를 지정하세요.")); return; }
    SaveLoadScreen = CreateWidget<UICSaveLoadWidget>(GetOwningPlayer(), SaveLoadWidgetClass);
    if (!SaveLoadScreen){UE_LOG(LogICUI, Warning, TEXT("[MainMenu.LoadWidgetFailed] Class=%s"),*GetNameSafe(SaveLoadWidgetClass.Get()));return;}
    SaveLoadScreen->Configure(false, StartMap);
    SaveLoadScreen->OnClosed.AddUniqueDynamic(this, &ThisClass::HandleSaveLoadClosed);
    SaveLoadScreen->AddToViewport(100); SetIsEnabled(false);
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = true;
        FInputModeUIOnly Mode; Mode.SetWidgetToFocus(SaveLoadScreen->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); PC->SetInputMode(Mode);
    }
}

void UICMainMenuWidget::HandleSaveLoadClosed()
{
    IC_TRACE_FUNCTION();
    SaveLoadScreen = nullptr; SetIsEnabled(true); RefreshContinueAvailability();
    if (APlayerController* PC = GetOwningPlayer())
    {
        FInputModeUIOnly Mode; Mode.SetWidgetToFocus(TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); PC->SetInputMode(Mode); PC->bShowMouseCursor = true;
        if (BTN_Load) BTN_Load->SetUserFocus(PC);
    }
}

void UICMainMenuWidget::HandleSettingsClicked()
{
    IC_TRACE_FUNCTION();
    // The settings screen is intentionally deferred; no Blueprint event is required.
    SetNotice(NSLOCTEXT("ICMainMenu", "SettingsLater", "설정 화면은 추후 연결 예정입니다."));
}

void UICMainMenuWidget::HandleQuitClicked()
{
    IC_TRACE_FUNCTION();
    if (PendingAction == EExitAction::None && !SaveLoadScreen) BeginExit(EExitAction::Quit);
}

void UICMainMenuWidget::HandleSaveFinished(int32 /*SlotIndex*/, bool /*bSuccess*/, const FString& /*Message*/)
{
    IC_TRACE_FUNCTION();
    if (PendingAction == EExitAction::None) RefreshContinueAvailability();
}

void UICMainMenuWidget::UpdateAmbientImages()
{
    IC_TRACE_FUNCTION();
    if (!bAnimateBackground) return;
    const float FogPhase = static_cast<float>(FMath::Fmod(AnimationSeconds, static_cast<double>(FMath::Max(FogPeriodSeconds, 1.f))) / FMath::Max(FogPeriodSeconds, 1.f) * 2.0 * PI);
    const float GlowPhase = static_cast<float>(FMath::Fmod(AnimationSeconds, static_cast<double>(FMath::Max(GlowPeriodSeconds, 1.f))) / FMath::Max(GlowPeriodSeconds, 1.f) * 2.0 * PI);
    if (IMG_MainFogA)
    {
        IMG_MainFogA->SetRenderTranslation(FogBaseA + FVector2D(
    30.f * FMath::Sin(FogPhase),
    10.f * FMath::Sin(FogPhase)));
        IMG_MainFogB->SetRenderOpacity(
     0.12f - 0.04f * FMath::Sin(FogPhase));
    }
    if (IMG_MainFogB)
    {
        IMG_MainFogB->SetRenderTranslation(FogBaseB - FVector2D(
    30.f * FMath::Sin(FogPhase),
    10.f * FMath::Sin(FogPhase)));
        IMG_MainGlow->SetRenderOpacity(
    0.15f + 0.08f * FMath::Sin(GlowPhase));
    }
    if (IMG_MainGlow) IMG_MainGlow->SetRenderOpacity(0.025f + 0.015f * FMath::Sin(GlowPhase));
}

void UICMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    IC_TRACE_FUNCTION();
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (PendingAction != EExitAction::None)
    {
        FadeElapsed += InDeltaTime;
        const float Progress = FMath::Clamp(FadeElapsed / FMath::Max(ExitFadeSeconds, 0.001f), 0.f, 1.f);
        const float Smooth = Progress * Progress * (3.f - 2.f * Progress);
        SetRenderOpacity(1.f - Smooth);
        if (Progress >= 1.f) CompleteExit();
        return;
    }
    if (SaveLoadScreen || !GetIsEnabled()) return;
    AnimationSeconds += InDeltaTime;
    UpdateAmbientImages(); UpdateButtonImages();
}
