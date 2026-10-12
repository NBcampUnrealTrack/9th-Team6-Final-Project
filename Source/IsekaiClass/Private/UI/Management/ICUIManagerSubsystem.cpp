#include "UI/Management/ICUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Time/ICTimeSubsystem.h"
#include "Debug/ICLog.h"
bool UICUIManagerSubsystem::OpenScreen(UUserWidget* Widget,bool PauseWorld,int32 ZOrder)
{
    IC_TRACE_FUNCTION();if(!IsValid(Widget)||Widget->GetOwningLocalPlayer()!=GetLocalPlayer())return false;
    CloseScreen(Widget);if(!Widget->AddToPlayerScreen(ZOrder))return false;
    Screens.Add({Widget,PauseWorld,ZOrder});RefreshContext();return true;
}
void UICUIManagerSubsystem::CloseScreen(UUserWidget* Widget)
{
    IC_TRACE_FUNCTION();
    // Destructing a parent can close a modal. Remove it before nested focus updates.
    Screens.RemoveAll([Widget](const FScreen& S){return S.Widget==Widget;});
    if(MenuFocus.Get()==Widget)MenuFocus.Reset();
    if(Widget){if(auto* T=GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())T->ReleasePauseRequests(Widget);Widget->RemoveFromParent();}
    RefreshContext();
}
void UICUIManagerSubsystem::SetMenuContext(APlayerController* Controller,UUserWidget* Focus,bool MenuOpen)
{
    IC_TRACE_FUNCTION();if(!Controller||Controller->GetLocalPlayer()!=GetLocalPlayer())return;
    MenuController=Controller;MenuFocus=Focus;bMenuOpen=MenuOpen;RefreshContext();
}
void UICUIManagerSubsystem::RefreshContext()
{
    IC_TRACE_FUNCTION();auto* Time=GetLocalPlayer()->GetGameInstance()?GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;
    if(Time){Time->SetPauseRequest(this,TEXT("Menu"),bMenuOpen);for(const FScreen& S:Screens)if(S.Widget.IsValid())Time->SetPauseRequest(S.Widget.Get(),TEXT("Screen"),S.Pause);}
    auto* PC=MenuController.IsValid()?MenuController.Get():GetLocalPlayer()->GetPlayerController(GetWorld());if(!PC)return;
    UUserWidget* Focus=MenuFocus.Get();int32 Z=MIN_int32;for(const FScreen& S:Screens)if(S.Widget.IsValid()&&S.Z>=Z){Focus=S.Widget.Get();Z=S.Z;}
    PC->bShowMouseCursor=bMenuOpen||!Screens.IsEmpty();
    if(PC->bShowMouseCursor){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);if(Focus)Mode.SetWidgetToFocus(Focus->TakeWidget());PC->SetInputMode(Mode);}
    else{FInputModeGameOnly Mode;PC->SetInputMode(Mode);}
}
void UICUIManagerSubsystem::CloseAllScreens()
{
    IC_TRACE_FUNCTION();auto Copy=Screens;Screens.Reset();for(const FScreen& S:Copy)if(S.Widget.IsValid()){if(auto* T=GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())T->ReleasePauseRequests(S.Widget.Get());S.Widget->RemoveFromParent();}
    bMenuOpen=false;MenuFocus.Reset();if(auto* T=GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())T->ReleasePauseRequests(this);RefreshContext();MenuController.Reset();
}
void UICUIManagerSubsystem::Tick(float DeltaTime)
{
    IC_TRACE_FUNCTION();bool Changed=false;for(int32 I=Screens.Num()-1;I>=0;--I)if(!Screens[I].Widget.IsValid()||!Screens[I].Widget->IsInViewport())
    {if(Screens[I].Widget.IsValid())if(auto* T=GetLocalPlayer()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>())T->ReleasePauseRequests(Screens[I].Widget.Get());Screens.RemoveAt(I);Changed=true;}
    if(MenuController.IsStale()){bMenuOpen=false;MenuFocus.Reset();Changed=true;}if(Changed)RefreshContext();
}
bool UICUIManagerSubsystem::IsTickable() const { IC_TRACE_FUNCTION();return !IsTemplate()&&(!Screens.IsEmpty()||bMenuOpen); }
TStatId UICUIManagerSubsystem::GetStatId() const { IC_TRACE_FUNCTION();RETURN_QUICK_DECLARE_CYCLE_STAT(UICUIManagerSubsystem,STATGROUP_Tickables); }
void UICUIManagerSubsystem::Deinitialize() { IC_TRACE_FUNCTION();CloseAllScreens();Super::Deinitialize(); }
