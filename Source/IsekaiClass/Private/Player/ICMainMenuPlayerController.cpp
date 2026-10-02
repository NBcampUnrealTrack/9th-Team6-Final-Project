#include "Player/ICMainMenuPlayerController.h"
#include "UI/ICMainMenuWidget.h"

void AICMainMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
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
