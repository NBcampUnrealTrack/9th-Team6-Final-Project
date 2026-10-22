#include "UI/MainMenu/ICMainMenuWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
void UICMainMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
        ICFrontendLayout::Build(WidgetTree, TEXT("Mainmenu"));
    SetIsFocusable(true);
    StartButton = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Start")));
    if (StartButton)
        StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Start);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Continue"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Continue);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_SaveList"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::SaveList);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Settings"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Settings);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Quit"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Quit);
}
void UICMainMenuWidget::FocusStartButton()
{
    if (StartButton)
        StartButton->SetUserFocus(GetOwningPlayer());
}
UWidget* UICMainMenuWidget::NativeGetDesiredFocusTarget() const
{
    return StartButton;
}
void UICMainMenuWidget::Start()
{
    if (auto* PC = Cast<AICPlayerController>(GetOwningPlayer()))
    {
        GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->BeginQuestions();
        PC->SetMenu(TEXT("Create"));
    }
}
void UICMainMenuWidget::Continue()
{
    if (auto* PC = Cast<AICPlayerController>(GetOwningPlayer()))
        if (!PC->ContinueLatestSave())
            if (auto* Subtitle = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Subtitle"))))
                Subtitle->SetText(PC->NoticeDisplayText);
}
void UICMainMenuWidget::Settings()
{
    if (auto* PC = Cast<AICPlayerController>(GetOwningPlayer()))
        PC->SetMenu(TEXT("Settings"));
}
void UICMainMenuWidget::SaveList()
{
    if (auto* PC = Cast<AICPlayerController>(GetOwningPlayer()))
        PC->OpenLoadMenu();
}
void UICMainMenuWidget::Quit()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
