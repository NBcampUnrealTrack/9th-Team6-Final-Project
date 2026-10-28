#include "UI/Gameplay/ICDeathWidget.h"
#include "Settings/ICGameUserSettings.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Engine/GameInstance.h"

void UICDeathWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if(!WidgetTree->RootWidget)ICFrontendLayout::Build(WidgetTree,TEXT("Death"));
    SetIsFocusable(true);
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Load"))))Button->OnClicked.AddUniqueDynamic(this,&ThisClass::LoadGame);
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("BTN_MainMenu"))))Button->OnClicked.AddUniqueDynamic(this,&ThisClass::MainMenu);
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("BTN_Quit"))))Button->OnClicked.AddUniqueDynamic(this,&ThisClass::QuitGame);
}
UWidget* UICDeathWidget::NativeGetDesiredFocusTarget() const
{return GetWidgetFromName(TEXT("BTN_Load"));}
FReply UICDeathWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Escape||Event.GetKey()==EKeys::Equals||UICGameUserSettings::IsGamepadBackKey(Event.GetKey()))return FReply::Handled();
    return Super::NativeOnKeyDown(Geometry,Event);
}
void UICDeathWidget::GoBack() {}
void UICDeathWidget::LoadGame()
{
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->IsDead()&&PC->Menu==TEXT("Death"))PC->OpenLoadMenu();
}
void UICDeathWidget::MainMenu()
{
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->IsDead()&&PC->Menu==TEXT("Death"))
        if(auto* Flow=GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>())Flow->ReturnToTitle(PC);
}
void UICDeathWidget::QuitGame()
{
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->IsDead()&&PC->Menu==TEXT("Death"))PC->RequestQuit();
}
