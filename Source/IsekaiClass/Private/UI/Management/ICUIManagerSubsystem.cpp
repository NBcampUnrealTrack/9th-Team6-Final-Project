#include "UI/Management/ICUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Time/ICTimeSubsystem.h"
#include "Debug/ICLog.h"
#include "UI/Framework/ICUIRootWidget.h"
#include "CommonActivatableWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/NavigationConfig.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"
namespace
{
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
    virtual EUINavigationAction GetNavigationActionFromKey(const FKeyEvent& Event) const override {return Fallback->GetNavigationActionFromKey(Event);}
private:
    TSharedRef<FNavigationConfig> Fallback;
};
int32 KeyboardNavigationUsers=0;
TSharedPtr<FNavigationConfig> PreviousKeyboardNavigation,KeyboardNavigation;
void RegisterKeyboardNavigation()
{
    if(KeyboardNavigationUsers++==0)
    {PreviousKeyboardNavigation=FSlateApplication::Get().GetNavigationConfig();KeyboardNavigation=MakeShared<FICKeyboardNavigation>(PreviousKeyboardNavigation.ToSharedRef());FSlateApplication::Get().SetNavigationConfig(KeyboardNavigation.ToSharedRef());}
}
void UnregisterKeyboardNavigation()
{
    if(--KeyboardNavigationUsers==0)
    {if(FSlateApplication::IsInitialized()&&FSlateApplication::Get().GetNavigationConfig()==KeyboardNavigation)FSlateApplication::Get().SetNavigationConfig(PreviousKeyboardNavigation.ToSharedRef());KeyboardNavigation.Reset();PreviousKeyboardNavigation.Reset();}
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
    PC->bShowMouseCursor = bMenuOpen || !Screens.IsEmpty();
    if (PC->bShowMouseCursor)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        if (Focus)
        {
            Mode.SetWidgetToFocus(Focus->TakeWidget());
            UWidget* Desired=nullptr;
            if (auto* Active = Cast<UCommonActivatableWidget>(Focus))
                Desired=Active->GetDesiredFocusTarget();
            if(!Desired)Desired=FirstKeyboardTarget(Focus);
            if(Desired)Mode.SetWidgetToFocus(Desired->TakeWidget());
        }
        PC->SetInputMode(Mode);
    }
    else
    {
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
