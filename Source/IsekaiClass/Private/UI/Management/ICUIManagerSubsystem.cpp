#include "UI/Management/ICUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Time/ICTimeSubsystem.h"
#include "Debug/ICLog.h"
#include "UI/Framework/ICUIRootWidget.h"
#include "UI/Framework/ICUITheme.h"
#include "CommonActivatableWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/NavigationConfig.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Settings/ICGameUserSettings.h"
#include "Data/Text/ICLocalization.h"
#include "HAL/IConsoleManager.h"
namespace
{
struct FICScaledFont {float Base=0,Applied=0;};
TMap<TWeakObjectPtr<UTextBlock>,FICScaledFont> ScaledFonts;
void ApplyUserTextScale(UWidget* Widget,float Scale)
{
    if(!Widget)return;
    if(auto* Text=Cast<UTextBlock>(Widget))
    {
        const FText Localized=ICLocalization::Localize(Text->GetText());
        if(!Localized.IdenticalTo(Text->GetText()))Text->SetText(Localized);
        auto Font=Text->GetFont();auto& Size=ScaledFonts.FindOrAdd(TWeakObjectPtr<UTextBlock>(Text),FICScaledFont{Font.Size,Font.Size});
        if(!FMath::IsNearlyEqual(Font.Size,Size.Applied))Size.Base=Font.Size;
        const float Desired=FMath::Max(8.f,Size.Base*Scale);
        if(!FMath::IsNearlyEqual(Font.Size,Desired)){Font.Size=Desired;Text->SetFont(Font);}
        Size.Applied=Desired;
    }
    else if(auto* User=Cast<UUserWidget>(Widget)){if(User->WidgetTree)ApplyUserTextScale(User->WidgetTree->RootWidget,Scale);}
    else if(auto* Panel=Cast<UPanelWidget>(Widget))for(auto* Child:Panel->GetAllChildren())ApplyUserTextScale(Child,Scale);
}
UWidget* FirstKeyboardTarget(UWidget* Widget)
{
    if(!Widget||!Widget->IsVisible()||!Widget->GetIsEnabled())return nullptr;
    if(auto* User=Cast<UUserWidget>(Widget))
    {if(User->WidgetTree)if(auto* Target=FirstKeyboardTarget(User->WidgetTree->RootWidget))return Target;}
    else if(auto* Switcher=Cast<UWidgetSwitcher>(Widget))
    {if(auto* Target=FirstKeyboardTarget(Switcher->GetActiveWidget()))return Target;}
    else if(auto* Panel=Cast<UPanelWidget>(Widget))
        for(auto* Child:Panel->GetAllChildren())if(auto* Target=FirstKeyboardTarget(Child))return Target;
    const auto Slate=Widget->GetCachedWidget();return Slate.IsValid()&&Slate->SupportsKeyboardFocus()?Widget:nullptr;
}
class FICKeyboardNavigation : public FNavigationConfig
{
public:
    explicit FICKeyboardNavigation(TSharedRef<FNavigationConfig> InFallback):Fallback(InFallback){}
    virtual EUINavigation GetNavigationDirectionFromKey(const FKeyEvent& Event) const override
    {
        const FKey Key=Event.GetKey();
        if((Key==EKeys::W||Key==EKeys::A||Key==EKeys::S||Key==EKeys::D)&&!Event.IsControlDown()&&!Event.IsAltDown())
        {
            const auto Focus=FSlateApplication::Get().GetUserFocusedWidget(Event.GetUserIndex());
            const FName Type=Focus.IsValid()?Focus->GetType():NAME_None;
            // Character creation, save names and other text fields must still accept WASD letters.
            if(Type==TEXT("SEditableText")||Type==TEXT("SMultiLineEditableText")||Type==TEXT("SEditableTextBox")||Type==TEXT("SMultiLineEditableTextBox"))return EUINavigation::Invalid;
            return Key==EKeys::W?EUINavigation::Up:Key==EKeys::S?EUINavigation::Down:Key==EKeys::A?EUINavigation::Left:EUINavigation::Right;
        }
        return Fallback->GetNavigationDirectionFromKey(Event);
    }
    virtual EUINavigation GetNavigationDirectionFromAnalog(const FAnalogInputEvent& Event) override {return Fallback->GetNavigationDirectionFromAnalog(Event);}
    virtual EUINavigationAction GetNavigationActionFromKey(const FKeyEvent& Event) const override
    {
        const auto Focus=FSlateApplication::Get().GetUserFocusedWidget(Event.GetUserIndex());
        const FName Type=Focus.IsValid()?Focus->GetType():NAME_None;
        const bool Editing=Type==TEXT("SEditableText")||Type==TEXT("SEditableTextBox")||Type==TEXT("SMultiLineEditableText")||Type==TEXT("SMultiLineEditableTextBox")||Type==TEXT("SInputKeySelector");
        if(!Editing&&(Event.GetKey()==EKeys::G||Event.GetKey()==EKeys::F))return EUINavigationAction::Accept;
        if(UICGameUserSettings::IsGamepadAcceptKey(Event.GetKey()))return EUINavigationAction::Accept;
        if(UICGameUserSettings::IsGamepadBackKey(Event.GetKey()))return EUINavigationAction::Back;
        return Fallback->GetNavigationActionFromKey(Event);
    }
private:
    TSharedRef<FNavigationConfig> Fallback;
};
int32 KeyboardNavigationUsers=0;
int32 PreviousVirtualAcceptMouse=1;
TSharedPtr<FNavigationConfig> PreviousKeyboardNavigation,KeyboardNavigation;
void RegisterKeyboardNavigation()
{
    if(KeyboardNavigationUsers++==0)
    {if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(TEXT("CommonUI.ShouldVirtualAcceptSimulateMouseButton")))PreviousVirtualAcceptMouse=Variable->GetInt();PreviousKeyboardNavigation=FSlateApplication::Get().GetNavigationConfig();KeyboardNavigation=MakeShared<FICKeyboardNavigation>(PreviousKeyboardNavigation.ToSharedRef());FSlateApplication::Get().SetNavigationConfig(KeyboardNavigation.ToSharedRef());}
}
void UnregisterKeyboardNavigation()
{
    if(--KeyboardNavigationUsers==0)
    {if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(TEXT("CommonUI.ShouldVirtualAcceptSimulateMouseButton")))Variable->Set(PreviousVirtualAcceptMouse,ECVF_SetByGameOverride);if(FSlateApplication::IsInitialized()&&FSlateApplication::Get().GetNavigationConfig()==KeyboardNavigation)FSlateApplication::Get().SetNavigationConfig(PreviousKeyboardNavigation.ToSharedRef());KeyboardNavigation.Reset();PreviousKeyboardNavigation.Reset();}
}
}
bool UICUIManagerSubsystem::EnsureRoot()
{
    if(!bKeyboardNavigationRegistered&&FSlateApplication::IsInitialized()){RegisterKeyboardNavigation();bKeyboardNavigationRegistered=true;}
    if (Root && Root->GetWorld() == GetWorld() && Root->IsInViewport())
        return true;
    if (Root)
        Root->RemoveFromParent();
    auto* PC = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (!PC)
        return false;
    Root = CreateWidget<UICUIRootWidget>(PC, UICUIRootWidget::StaticClass());
    return Root && Root->AddToPlayerScreen(0);
}
void UICUIManagerSubsystem::SetHUD(UUserWidget* Widget)
{
    if (EnsureRoot())
        Root->SetHUD(Widget);
}
void UICUIManagerSubsystem::AddHUDWidget(UUserWidget* Widget)
{
    if(EnsureRoot())Root->AddHUDWidget(Widget);
}
bool UICUIManagerSubsystem::OpenScreen(UUserWidget* Widget, bool PauseWorld, int32 ZOrder)
{
    IC_TRACE_FUNCTION();
    if (!IsValid(Widget) || Widget->GetOwningLocalPlayer() != GetLocalPlayer())
        return false;
    CloseScreen(Widget);
    if (!EnsureRoot())
        return false;
    Root->Mount(Widget, ZOrder);
    if(Widget->WidgetTree)ICUITheme::Apply(Widget->WidgetTree);
    Screens.Add({Widget, PauseWorld, ZOrder});
    RefreshContext();
    return true;
}
void UICUIManagerSubsystem::CloseScreen(UUserWidget* Widget)
{
    IC_TRACE_FUNCTION();

    Screens.RemoveAll([Widget](const FScreen& S) { return S.Widget == Widget; });
    if (MenuFocus.Get() == Widget)
        MenuFocus.Reset();
    if (Widget)
    {
        if (auto* T = GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())
            T->ReleasePauseRequests(Widget);
        if (Root)
            Root->Unmount(Widget);
        else
            Widget->RemoveFromParent();
    }
    RefreshContext();
}
void UICUIManagerSubsystem::SetMenuContext(APlayerController* Controller, UUserWidget* Focus, bool MenuOpen)
{
    IC_TRACE_FUNCTION();
    if (!Controller || Controller->GetLocalPlayer() != GetLocalPlayer())
        return;
    MenuController = Controller;
    MenuFocus = Focus;
    bMenuOpen = MenuOpen;
    RefreshContext();
}
void UICUIManagerSubsystem::RefreshContext()
{
    IC_TRACE_FUNCTION();
    auto* Time = GetLocalPlayer()->GetGameInstance()
                     ? GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>()
                     : nullptr;
    if (Time)
    {
        Time->SetPauseRequest(this, TEXT("Menu"), bMenuOpen);
        for (const FScreen& S : Screens)
            if (S.Widget.IsValid())
                Time->SetPauseRequest(S.Widget.Get(), TEXT("Screen"), S.Pause);
    }
    auto* PC = MenuController.IsValid() ? MenuController.Get() : GetLocalPlayer()->GetPlayerController(GetWorld());
    if (!PC)
        return;
    UUserWidget* Focus = MenuFocus.Get();
    int32 Z = MIN_int32;
    for (const FScreen& S : Screens)
        if (S.Widget.IsValid() && S.Z >= Z)
        {
            Focus = S.Widget.Get();
            Z = S.Z;
        }
    const bool HasMenu=bMenuOpen||!Screens.IsEmpty();
    PC->bShowMouseCursor=false;
    PC->bEnableClickEvents=false;PC->bEnableMouseOverEvents=false;
    // These screens use focused-widget key navigation. A synthetic mouse click bypasses their A-key handlers.
    if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(TEXT("CommonUI.ShouldVirtualAcceptSimulateMouseButton")))
        Variable->Set(0,ECVF_SetByGameOverride);
    if (HasMenu)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(true);
        if (Focus)
        {
            Mode.SetWidgetToFocus(Focus->TakeWidget());
            UWidget* Desired=nullptr;
            if (auto* Active = Cast<UCommonActivatableWidget>(Focus))
                Desired=Active->GetDesiredFocusTarget();
            if(!Desired)Desired=FirstKeyboardTarget(Focus);
            if(Desired&&(!Desired->IsVisible()||!Desired->GetIsEnabled()))Desired=nullptr;
            if(Desired)Mode.SetWidgetToFocus(Desired->TakeWidget());
            PendingFocus=Desired?Desired:Focus;
        }
        PC->SetInputMode(Mode);
    }
    else
    {
        PendingFocus.Reset();
        FInputModeGameOnly Mode;
        PC->SetInputMode(Mode);
    }
}
void UICUIManagerSubsystem::CloseAllScreens()
{
    IC_TRACE_FUNCTION();
    auto Copy = Screens;
    Screens.Reset();
    for (const FScreen& S : Copy)
        if (S.Widget.IsValid())
        {
            if (auto* T = GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())
                T->ReleasePauseRequests(S.Widget.Get());
            if (Root)
                Root->Unmount(S.Widget.Get());
            else
                S.Widget->RemoveFromParent();
        }
    if (Root)
    {
        Root->RemoveFromParent();
        Root = nullptr;
    }
    bMenuOpen = false;
    MenuFocus.Reset();
    if (auto* T = GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())
        T->ReleasePauseRequests(this);
    RefreshContext();
    MenuController.Reset();
}
void UICUIManagerSubsystem::Tick(float DeltaTime)
{
    IC_TRACE_FUNCTION();
    // Newly mounted screens may not have a Slate path until the next layout pass.
    // Complete the original focus request once; never take focus back on every key.
    if(auto* Target=PendingFocus.Get())
    {
        if(auto* PC=GetLocalPlayer()->GetPlayerController(GetWorld());PC&&Target->IsVisible()&&Target->GetIsEnabled())
        {
            Target->SetUserFocus(PC);
            if(Target->HasUserFocus(PC)||Target->HasFocusedDescendants())PendingFocus.Reset();
        }
        else PendingFocus.Reset();
    }
    if(Root)ApplyUserTextScale(Root,FMath::Clamp(UICGameUserSettings::Get()->UIScale,.75f,1.3f));
    for(const auto& Screen:Screens)if(Screen.Widget.IsValid())ICUITheme::UpdateFocus(Screen.Widget.Get());
    if(ScaledFonts.Num()>4096)for(auto It=ScaledFonts.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    bool Changed = false;
    for (int32 I = Screens.Num() - 1; I >= 0; --I)
        if (!Screens[I].Widget.IsValid() || (!Screens[I].Widget->GetCachedWidget().IsValid()))
        {
            if (Screens[I].Widget.IsValid())
                if (auto* T = GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())
                    T->ReleasePauseRequests(Screens[I].Widget.Get());
            Screens.RemoveAt(I);
            Changed = true;
        }
    if (MenuController.IsStale())
    {
        bMenuOpen = false;
        MenuFocus.Reset();
        Changed = true;
    }
    if (Changed)
        RefreshContext();
}
bool UICUIManagerSubsystem::IsTickable() const
{
    IC_TRACE_FUNCTION();
    return !IsTemplate() && (!Screens.IsEmpty() || bMenuOpen);
}
TStatId UICUIManagerSubsystem::GetStatId() const
{
    IC_TRACE_FUNCTION();
    RETURN_QUICK_DECLARE_CYCLE_STAT(UICUIManagerSubsystem, STATGROUP_Tickables);
}
void UICUIManagerSubsystem::Deinitialize()
{
    IC_TRACE_FUNCTION();
    CloseAllScreens();
    if(bKeyboardNavigationRegistered){UnregisterKeyboardNavigation();bKeyboardNavigationRegistered=false;}
    Super::Deinitialize();
}
