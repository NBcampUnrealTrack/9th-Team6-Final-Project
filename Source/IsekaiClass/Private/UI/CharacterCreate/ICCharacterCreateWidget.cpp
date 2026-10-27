#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
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
    RefreshState();
}
UWidget* UICCharacterCreateWidget::NativeGetDesiredFocusTarget() const
{
    if (Step == 0)
        return NameInput;
    if (!ChoiceButtons.IsEmpty())
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
    // Hide obsolete controls as well if an older preview Blueprint is loaded.
    for (const auto* Name : {TEXT("TXT_Step"), TEXT("TXT_Detail"), TEXT("BTN_Back")})
        if (auto* Widget = GetWidgetFromName(Name))
            Widget->SetVisibility(ESlateVisibility::Collapsed);
    SetText(TEXT("TXT_Question"), ICFrontendLayout::Text(*FString::Printf(TEXT("Question%d"), Step + 1)));
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
        Button->SetIsEnabled(!bStartingNewGame);
        if (Visible)
            if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
                Text->SetText(Step == 1 ? ICFrontendLayout::Text(I == 0 ? TEXT("Male") : TEXT("Female"))
                                        : Flow->GetOption(Step, I).Label);
    }
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Next"))))
    {
        Button->SetVisibility(Step == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        Button->SetIsEnabled(Step != 0 || FICCreationAnswers::IsNameValid(Flow->Answers.Name));
        if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
            Text->SetText(ICFrontendLayout::Text(TEXT("SetName")));
    }
    SetText(TEXT("TXT_Error"), FText::FromString(Flow->LastError));
    if (auto* Error = GetWidgetFromName(TEXT("TXT_Error")))
        Error->SetVisibility(Flow->LastError.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
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
    if (bStartingNewGame || Step < 1 || Step > 4 || Index < 0 || Index >= (Step == 1 ? 2 : 4))
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
    if (Step == 4)
    {
        StartGame();
        return;
    }
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
    // Character creation advances only through the displayed answer buttons.
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
}
void UICCharacterCreateWidget::StartGame()
{
    auto* Flow = GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
    if (bStartingNewGame || !Flow->Answers.IsComplete())
        return;
    bStartingNewGame = true;
    RefreshState();
    if (!Flow->BeginNewGame(Cast<AICPlayerController>(GetOwningPlayer())))
    {
        bStartingNewGame = false;
        RefreshState();
        RequestRefreshFocus();
    }
}
