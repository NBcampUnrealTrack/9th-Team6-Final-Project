#include "UI/ICMainMenuWidget.h"

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

void UICMainMenuWidget::HandleLoadClicked() { OnLoadRequested(); }
void UICMainMenuWidget::HandleSettingsClicked() { OnSettingsRequested(); }

void UICMainMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
