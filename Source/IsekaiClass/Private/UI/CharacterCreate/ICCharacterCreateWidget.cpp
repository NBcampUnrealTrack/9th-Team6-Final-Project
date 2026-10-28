#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "UI/CharacterCreate/ICNameKeyButton.h"
#include "UI/Framework/ICPrologueWidget.h"
#include "Settings/ICGameUserSettings.h"
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
    for(int32 I=0;I<40;++I)
        if(auto* Key=Cast<UICNameKeyButton>(GetWidgetFromName(FName(*FString::Printf(TEXT("NameKey%d"),I)))))
        {Key->KeyIndex=I;Key->BindKey();Key->OnKeyChosen.AddUniqueDynamic(this,&ThisClass::ChooseKey);NameKeys.Add(Key);}
    RefreshKeyboard();
    RefreshState();
}
UWidget* UICCharacterCreateWidget::NativeGetDesiredFocusTarget() const
{
    if (Step == 0)
    {
        // CommonUI asks again when switching between a pad and keyboard. Retain the selected key.
        for(const auto& Key:NameKeys)if(Key&&(Key->HasKeyboardFocus()||Key->HasUserFocus(GetOwningPlayer())))return Key.Get();
        if(auto* Done=GetWidgetFromName(TEXT("BTN_Next"));Done&&(Done->HasKeyboardFocus()||Done->HasUserFocus(GetOwningPlayer())))return Done;
        return NameKeys.IsValidIndex(KeyboardIndex)?NameKeys[KeyboardIndex].Get():GetWidgetFromName(TEXT("BTN_Next"));
    }
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
void UICCharacterCreateWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    if(Step!=0)return;
    for(int32 I=0;I<NameKeys.Num();++I)if(NameKeys[I]->HasKeyboardFocus()||NameKeys[I]->HasUserFocus(GetOwningPlayer()))KeyboardIndex=I;
    if(auto* Done=GetWidgetFromName(TEXT("BTN_Next"));Done&&(Done->HasKeyboardFocus()||Done->HasUserFocus(GetOwningPlayer())))KeyboardIndex=40;
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
    if(auto* Keyboard=GetWidgetFromName(TEXT("NameKeyboard")))Keyboard->SetVisibility(Step==0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
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
    if(!bUpdatingName){NameComposer.Clear();NameComposer.Text=Draft.Name;}
    Flow->SetAnswers(Draft);
    RefreshKeyboard();
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
    if (!UICPrologueWidget::Show(Cast<AICPlayerController>(GetOwningPlayer())))
    {
        bStartingNewGame = false;
        RefreshState();
        RequestRefreshFocus();
    }
}

void UICCharacterCreateWidget::RefreshKeyboard()
{
    const FString Letters=bKorean?(bShift?TEXT("ㅃㅉㄸㄲㅆㅛㅕㅑㅒㅖㅁㄴㅇㄹㅎㅗㅓㅏㅣㅋㅌㅊㅍㅠㅜㅡ"):TEXT("ㅂㅈㄷㄱㅅㅛㅕㅑㅐㅔㅁㄴㅇㄹㅎㅗㅓㅏㅣㅋㅌㅊㅍㅠㅜㅡ"))
        :(bShift?TEXT("QWERTYUIOPASDFGHJKLZXCVBNM"):TEXT("qwertyuiopasdfghjklzxcvbnm"));
    for(int32 I=0;I<NameKeys.Num();++I)
    {
        FString Caption;
        if(I<26)Caption=FString::Chr(Letters[I]);
        else if(I<36)Caption=FString::FromInt(I-26);
        else if(I==36)Caption=bKorean?TEXT("한/EN"):TEXT("EN/한");
        else if(I==37)Caption=bShift?TEXT("Shift ●"):TEXT("Shift");
        else if(I==39)Caption=TEXT("공백");
        else if(I==38)Caption=TEXT("지우기");
        if(auto* Label=Cast<UTextBlock>(NameKeys[I]->GetContent()))Label->SetText(FText::FromString(Caption));
        NameKeys[I]->SetIsEnabled(!bStartingNewGame);
    }
}
void UICCharacterCreateWidget::ChooseKey(int32 Index)
{
    if(Step!=0||bStartingNewGame)return;
    if(Index==36)bKorean=!bKorean;
    else if(Index==37)bShift=!bShift;
    else if(Index==38)NameComposer.Backspace();
    else if(Index==40){Next();return;}
    else
    {
        const FString Letters=bKorean?(bShift?TEXT("ㅃㅉㄸㄲㅆㅛㅕㅑㅒㅖㅁㄴㅇㄹㅎㅗㅓㅏㅣㅋㅌㅊㅍㅠㅜㅡ"):TEXT("ㅂㅈㄷㄱㅅㅛㅕㅑㅐㅔㅁㄴㅇㄹㅎㅗㅓㅏㅣㅋㅌㅊㅍㅠㅜㅡ"))
            :(bShift?TEXT("QWERTYUIOPASDFGHJKLZXCVBNM"):TEXT("qwertyuiopasdfghjklzxcvbnm"));
        if(Index>=0&&Index<26)NameComposer.Append(Letters[Index]);
        else if(Index>=26&&Index<36)NameComposer.Append(TEXT('0')+Index-26);
        else if(Index==39)NameComposer.Append(TEXT(' '));
    }
    TGuardValue<bool> Updating(bUpdatingName,true);
    const FText Value=FText::FromString(NameComposer.Text);
    if(NameInput)NameInput->SetText(Value);
    // SetText updates the UMG value without emitting the user-edit event in UE 5.8.
    NameChanged(Value);
}
FReply UICCharacterCreateWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(bStartingNewGame)return FReply::Handled();
    if(Step!=0)return Super::NativeOnPreviewKeyDown(Geometry,Event);
    if(NameInput&&(NameInput->HasKeyboardFocus()||NameInput->HasFocusedDescendants())&&!Event.GetKey().IsGamepadKey())
    {
        if(Event.GetKey()==EKeys::Enter){if(!Event.IsRepeat())Next();return FReply::Handled();}
        return Super::NativeOnPreviewKeyDown(Geometry,Event);
    }
    for(int32 I=0;I<NameKeys.Num();++I)if(NameKeys[I]->HasKeyboardFocus()||NameKeys[I]->HasUserFocus(GetOwningPlayer()))KeyboardIndex=I;
    if(auto* Done=GetWidgetFromName(TEXT("BTN_Next"));Done&&(Done->HasKeyboardFocus()||Done->HasUserFocus(GetOwningPlayer())))KeyboardIndex=40;
    const FKey Key=UICGameUserSettings::MenuKey(Event.GetKey());
    if(Key==EKeys::Up||Key==EKeys::W||Key==EKeys::Down||Key==EKeys::S||Key==EKeys::Left||Key==EKeys::A||Key==EKeys::Right||Key==EKeys::D)
    {
        const int32 Delta=Key==EKeys::Up||Key==EKeys::W?-10:Key==EKeys::Down||Key==EKeys::S?10:Key==EKeys::Left||Key==EKeys::A?-1:1;
        if(KeyboardIndex==40)KeyboardIndex=Delta<0?39:0;
        else if(Delta==10&&KeyboardIndex>=30)KeyboardIndex=40;
        else KeyboardIndex=(KeyboardIndex+Delta+40)%40;
        if(KeyboardIndex==40)if(auto* Done=GetWidgetFromName(TEXT("BTN_Next")))Done->SetUserFocus(GetOwningPlayer());
        if(NameKeys.IsValidIndex(KeyboardIndex))NameKeys[KeyboardIndex]->SetUserFocus(GetOwningPlayer());
        return FReply::Handled();
    }
    if(Key==EKeys::Enter||Key==EKeys::SpaceBar)
    {if(!Event.IsRepeat())ChooseKey(KeyboardIndex);return FReply::Handled();}
    if(Key==EKeys::BackSpace||UICGameUserSettings::IsGamepadBackKey(Event.GetKey()))
    {if(!Event.IsRepeat())ChooseKey(38);return FReply::Handled();}
    if(Event.GetKey()==EKeys::Gamepad_LeftShoulder){if(!Event.IsRepeat())ChooseKey(36);return FReply::Handled();}
    if(Event.GetKey()==EKeys::Gamepad_RightShoulder){if(!Event.IsRepeat())ChooseKey(37);return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
