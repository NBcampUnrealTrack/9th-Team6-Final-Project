#include "Core/Controller/ICMainMenuPlayerController.h"
#include "UI/MainMenu/ICMainMenuWidget.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Engine/GameInstance.h"

void AICMainMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
    if(UGameInstance* Instance=GetGameInstance())
    {
        if(UICTimeSubsystem* Time=Instance->GetSubsystem<UICTimeSubsystem>())
        {Time->SetRunning(false);Time->SetMenuPaused(true);}
        if(UICSaveSubsystem* Saves=Instance->GetSubsystem<UICSaveSubsystem>())Saves->SetAutoSaveSuspended(true);
    }
    if (!MainMenuWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenu: Set MainMenuWidgetClass in BP_MainMenuPC."));
        return;
    }
    MainMenuWidget = CreateWidget<UICMainMenuWidget>(this, MainMenuWidgetClass);
    if (!MainMenuWidget) return;
    MainMenuWidget->AddToViewport();
    bShowMouseCursor = true;
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    MainMenuWidget->FocusStartButton();
}
