#include "UI/ICMainMenuWidget.h"
#include "UI/ICSaveLoadWidget.h"
#include "../../Core/ICSaveSubsystem.h"
#include "../../Core/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"

#include "Components/Button.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UICMainMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (BTN_Start) BTN_Start->OnClicked.AddUniqueDynamic(this, &UICMainMenuWidget::HandleStartClicked);
    if (BTN_Load) BTN_Load->OnClicked.AddUniqueDynamic(this, &UICMainMenuWidget::HandleLoadClicked);
    if (BTN_Settings) BTN_Settings->OnClicked.AddUniqueDynamic(this, &UICMainMenuWidget::HandleSettingsClicked);
    if (BTN_Quit) BTN_Quit->OnClicked.AddUniqueDynamic(this, &UICMainMenuWidget::HandleQuitClicked);
}

void UICMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    // A returned gameplay session must not keep advancing or autosaving behind the menu.
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UICSaveSubsystem* Saves = GI->GetSubsystem<UICSaveSubsystem>())
        { Saves->SetAutoSaveSuspended(true); Saves->SetSafetyState(FICSaveSafety()); }
        if (UICTimeSubsystem* Time = GI->GetSubsystem<UICTimeSubsystem>())
        { Time->SetRunning(false); Time->SetMenuPaused(true); }
    }
}

void UICMainMenuWidget::FocusStartButton()
{
    if (BTN_Start && GetOwningPlayer()) BTN_Start->SetUserFocus(GetOwningPlayer());
}

void UICMainMenuWidget::HandleStartClicked()
{
    if (StartMap.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("MainMenu: Set StartMap in WBP_MainMenu Class Defaults."));
        OnStartMapNotConfigured();
        return;
    }

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, StartMap);
}

void UICMainMenuWidget::HandleLoadClicked()
{
    if (SaveLoadScreen && SaveLoadScreen->IsInViewport()) return;
    if (!SaveLoadWidgetClass) { OnLoadRequested(); return; }
    SaveLoadScreen = CreateWidget<UICSaveLoadWidget>(GetOwningPlayer(), SaveLoadWidgetClass);
    if (!SaveLoadScreen) return;
    SaveLoadScreen->Configure(false, StartMap);
    SaveLoadScreen->OnClosed.AddUniqueDynamic(this, &ThisClass::HandleSaveLoadClosed);
    SaveLoadScreen->AddToViewport(100);
    SetIsEnabled(false);
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = true;
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(SaveLoadScreen->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
    }
}
void UICMainMenuWidget::HandleSaveLoadClosed()
{
    SaveLoadScreen = nullptr; SetIsEnabled(true);
    if (APlayerController* PC = GetOwningPlayer())
    {
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode); PC->bShowMouseCursor = true;
        if (BTN_Load) BTN_Load->SetUserFocus(PC);
    }
}
void UICMainMenuWidget::HandleSettingsClicked() { OnSettingsRequested(); }

void UICMainMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
