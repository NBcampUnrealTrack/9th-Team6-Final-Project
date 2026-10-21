#include "UI/MainMenu/ICMainMenuWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Components/Button.h"
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
void UICMainMenuWidget::Quit()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
