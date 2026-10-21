#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Framework/ICConfirmPopupWidget.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Save/ICSaveSubsystem.h"
void UICCharacterCreateWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
        ICFrontendLayout::Build(WidgetTree, TEXT("Questions"));
    SetIsFocusable(true);
    NameInput = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("EDIT_Name")));
    if (NameInput)
        NameInput->OnTextChanged.AddUniqueDynamic(this, &ThisClass::NameChanged);
    for (int32 I = 0; I < 4; ++I)
        ChoiceButtons.Add(Cast<UButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Choice%d"), I)))));
    if (ChoiceButtons[0])
        ChoiceButtons[0]->OnClicked.AddUniqueDynamic(this, &ThisClass::Choice0);
    if (ChoiceButtons[1])
        ChoiceButtons[1]->OnClicked.AddUniqueDynamic(this, &ThisClass::Choice1);
    if (ChoiceButtons[2])
        ChoiceButtons[2]->OnClicked.AddUniqueDynamic(this, &ThisClass::Choice2);
    if (ChoiceButtons[3])
        ChoiceButtons[3]->OnClicked.AddUniqueDynamic(this, &ThisClass::Choice3);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Next"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Next);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Back"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Back);
    RefreshState();
}
UWidget* UICCharacterCreateWidget::NativeGetDesiredFocusTarget() const
{
    if (Step == 0)
        return NameInput;
    if (Step < 5 && !ChoiceButtons.IsEmpty())
        return ChoiceButtons[0];
    return GetWidgetFromName(TEXT("BTN_Next"));
}
void UICCharacterCreateWidget::StoreName()
{
    if (Step == 0 && NameInput)
    {
        auto* Flow = GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
        auto Draft = Flow->Answers;
        Draft.Name = NameInput->GetText().ToString();
        Flow->SetAnswers(Draft);
    }
}
void UICCharacterCreateWidget::RefreshState()
{
    auto* Flow = GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    auto SetText = [this](const TCHAR* Name, const FText& Value) {
        if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(Name)))
            Text->SetText(Value);
    };
    SetText(TEXT("TXT_Step"),
            FText::FromString(Step < 5 ? FString::Printf(TEXT("%d / 5"), Step + 1) : TEXT("5 / 5 · 시작 준비")));
    SetText(TEXT("TXT_Question"), Step < 5 ? ICFrontendLayout::Text(*FString::Printf(TEXT("Question%d"), Step + 1))
                                           : ICFrontendLayout::Text(TEXT("Review")));
    if (NameInput)
    {
        NameInput->SetVisibility(Step == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (Step == 0 && NameInput->GetText().ToString() != Flow->Answers.Name)
            NameInput->SetText(FText::FromString(Flow->Answers.Name));
    }
    for (int32 I = 0; I < 4; ++I)
    {
        auto* Button = ChoiceButtons[I].Get();
        if (!Button)
            continue;
        const bool Visible = Step > 0 && Step < 5 && (Step != 1 || I < 2);
        Button->SetVisibility(Visible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (Visible)
            if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
                Text->SetText(Step == 1 ? ICFrontendLayout::Text(I == 0 ? TEXT("Male") : TEXT("Female"))
                                        : Flow->GetOption(Step, I).Label);
    }
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Next"))))
    {
        Button->SetVisibility(Step == 0 || Step == 5 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        Button->SetIsEnabled(Step != 0 || FICCreationAnswers::IsNameValid(Flow->Answers.Name));
        if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
            Text->SetText(ICFrontendLayout::Text(Step == 5 ? TEXT("Begin") : TEXT("Next")));
    }
    FText Detail;
    if (Step == 0)
        Detail = ICFrontendLayout::Text(TEXT("NameHint"));
    else if (Step == 3)
        Detail = ICFrontendLayout::Text(TEXT("MusicHelp"));
    else if (Step == 4)
        Detail = ICFrontendLayout::Text(TEXT("HappinessHelp"));
    else if (Step == 5)
        Detail = FText::FromString(FString::Printf(
            TEXT("%s · %s\n\n%s\n%s\n\n%s\n%s\n\n%s"), *Flow->Answers.Name.TrimStartAndEnd(),
            *ICFrontendLayout::Text(Flow->Answers.Gender == 0 ? TEXT("Male") : TEXT("Female")).ToString(),
            *Flow->GetOption(2, Flow->Answers.Story).Label.ToString(),
            *Flow->GetOption(2, Flow->Answers.Story).ResultText.ToString(),
            *Flow->GetOption(3, Flow->Answers.Music).Label.ToString(),
            *Flow->GetOption(3, Flow->Answers.Music).ResultText.ToString(),
            *Flow->GetOption(4, Flow->Answers.Happiness).Label.ToString()));
    SetText(TEXT("TXT_Detail"), Detail);
    SetText(TEXT("TXT_Error"), FText::FromString(Flow->LastError));
}
void UICCharacterCreateWidget::NameChanged(const FText& Value)
{
    if (Step != 0)
        return;
    auto* Flow = GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    auto Draft = Flow->Answers;
    Draft.Name = Value.ToString();
    Flow->SetAnswers(Draft);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Next"))))
        Button->SetIsEnabled(FICCreationAnswers::IsNameValid(Draft.Name));
}
void UICCharacterCreateWidget::SelectAnswer(int32 Index)
{
    if (Step < 1 || Step > 4 || Index < 0 || Index >= (Step == 1 ? 2 : 4))
        return;
    auto* Flow = GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    auto Draft = Flow->Answers;
    if (Step == 1)
        Draft.Gender = Index;
    else if (Step == 2)
        Draft.Story = Index;
    else if (Step == 3)
        Draft.Music = Index;
    else
        Draft.Happiness = Index;
    Flow->SetAnswers(Draft);
    ++Step;
    RefreshState();
    RequestRefreshFocus();
}
void UICCharacterCreateWidget::Choice0()
{
    SelectAnswer(0);
}
void UICCharacterCreateWidget::Choice1()
{
    SelectAnswer(1);
}
void UICCharacterCreateWidget::Choice2()
{
    SelectAnswer(2);
}
void UICCharacterCreateWidget::Choice3()
{
    SelectAnswer(3);
}
void UICCharacterCreateWidget::GoBack()
{
    Back();
}
void UICCharacterCreateWidget::Back()
{
    StoreName();
    if (Step > 0)
    {
        --Step;
        RefreshState();
        RequestRefreshFocus();
    }
    else
        GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->ReturnToTitle(
            Cast<AICPlayerController>(GetOwningPlayer()));
}
void UICCharacterCreateWidget::Next()
{
    StoreName();
    auto* Flow = GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    if (Step == 0 && FICCreationAnswers::IsNameValid(Flow->Answers.Name))
    {
        Step = 1;
        RefreshState();
        RequestRefreshFocus();
    }
    else if (Step == 5 && Flow->Answers.IsComplete())
        ConfirmStart();
}
void UICCharacterCreateWidget::ConfirmStart()
{
    auto* UI = GetOwningLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
    UClass* Class = LoadClass<UICConfirmPopupWidget>(
        nullptr, TEXT("/Game/IsekaiClass/UI/Widget/WBP_ConfirmPopup.WBP_ConfirmPopup_C"));
    auto* Popup =
        CreateWidget<UICConfirmPopupWidget>(GetOwningPlayer(), Class ? Class : UICConfirmPopupWidget::StaticClass());
    if (!Popup)
        return;
    const auto Slots = GetGameInstance()->GetSubsystem<UICSaveSubsystem>()->GetSlotList();
    const bool Overwrite = Slots.IsValidIndex(0) && Slots[0].bExists;
    Popup->Configure(ICFrontendLayout::Text(Overwrite ? TEXT("NewGameOverwrite") : TEXT("NewGameConfirm")),
                     FSimpleDelegate::CreateWeakLambda(this, [this]() {
                         if (!GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>()->BeginNewGame(
                                 Cast<AICPlayerController>(GetOwningPlayer())))
                             RefreshState();
                     }));
    UI->OpenScreen(Popup, true, 2000);
}
