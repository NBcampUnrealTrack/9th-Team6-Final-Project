#include "UI/MainMenu/ICMainMenuWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Settings/ICGameUserSettings.h"
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

    SetIsFocusable(true);
    StartButton = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Start")));
    if (StartButton)
        StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Start);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Continue"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Continue);
    if (auto* PC = Cast<AICPlayerController>(GetOwningPlayer()))
        PC->OnNoticeChanged.AddUniqueDynamic(this, &ThisClass::RefreshNotice);
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
FReply UICMainMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const FKey Key=UICGameUserSettings::MenuKey(Event.GetKey());
    const TCHAR* Names[]={TEXT("BTN_Continue"),TEXT("BTN_Start"),TEXT("BTN_SaveList"),TEXT("BTN_Settings"),TEXT("BTN_Quit")};
    TArray<UButton*> Choices;
    for(const auto* Name:Names)if(auto* Button=Cast<UButton>(GetWidgetFromName(Name));Button&&Button->IsVisible()&&Button->GetIsEnabled())Choices.Add(Button);
    for(int32 I=0;I<Choices.Num();++I)if(Choices[I]->HasUserFocus(GetOwningPlayer()))Selected=I;
    if(Choices.IsEmpty())return FReply::Handled();
    Selected=FMath::Clamp(Selected,0,Choices.Num()-1);
    if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::S||Key==EKeys::Down)
    {
        Selected=(Selected+(Key==EKeys::S||Key==EKeys::Down?1:-1)+Choices.Num())%Choices.Num();
        Choices[Selected]->SetUserFocus(GetOwningPlayer());return FReply::Handled();
    }
    if(Key==EKeys::Enter||Key==EKeys::SpaceBar)
    {if(!Event.IsRepeat())Choices[Selected]->OnClicked.Broadcast();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
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
        PC->ContinueLatestSave();
}
void UICMainMenuWidget::RefreshNotice(const FString& Notice)
{
    if (auto* PC = Cast<AICPlayerController>(GetOwningPlayer()); PC && PC->Menu == TEXT("MainMenu"))
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
