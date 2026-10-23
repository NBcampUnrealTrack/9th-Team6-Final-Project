#include "UI/Quest/ICQuestListWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "UI/Framework/ICConfirmPopupWidget.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Data/Rows/Quest/ICQuestRow.h"
#include "Data/Text/ICTextLibrary.h"
#include "Debug/ICLog.h"
#include "Internationalization/StringTable.h"
#include "Quest/ICQuestSubsystem.h"
#include "UI/Quest/ICQuestDisplay.h"
#include "World/State/ICWorldStateSubsystem.h"

void UICQuestListWidget::SetQuestData(const FICQuestRow& Definition, const FICQuestState& Quest,
    const FICWorldState& World, const UICQuestSubsystem* Quests, UStringTable* Strings)
{
    DisplayedQuestID = Definition.ID;
    UpdateAction(Definition, Quest);
    const FICTextLookupResult NameResult = UICTextLibrary::ResolveText(Strings, Definition.NameKey);
    if (!NameResult.bSuccess)
    {
        UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.TextMissing] Quest=%s Field=Name Key=%s Reason=%s"),
            *Definition.ID.ToString(), *Definition.NameKey, *NameResult.FailureReason);
    }
    QuestNameText->SetText(NameResult.Text);

    const FICTextLookupResult DescriptionResult = UICTextLibrary::ResolveText(Strings, Definition.DescriptionKey);
    if (!DescriptionResult.bSuccess)
    {
        UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.TextMissing] Quest=%s Field=Description Key=%s Reason=%s"),
            *Definition.ID.ToString(), *Definition.DescriptionKey, *DescriptionResult.FailureReason);
    }
    QuestDescriptionText->SetText(DescriptionResult.Text);

    TArray<FText> ObjectiveLines;
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
        {
            const FICTextLookupResult Result = UICTextLibrary::ResolveText(Strings, Objective.DescriptionKey);
            if (!Result.bSuccess)
            {
                UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.TextMissing] Quest=%s Objective=%s Key=%s Reason=%s"),
                    *Definition.ID.ToString(), *Objective.ObjectiveID.ToString(), *Objective.DescriptionKey, *Result.FailureReason);
                continue;
            }
            ObjectiveLines.Add(FICQuestDisplay::FormatObjective(Result.Text, Objective, Quest, World, Quests));
        }
    }
    QuestObjectivesText->SetText(FText::Join(FText::FromString(TEXT("\n")), ObjectiveLines));
}

void UICQuestListWidget::UpdateAction(const FICQuestRow& Definition, const FICQuestState& Quest)
{
    if (!QuestAbandonButton)
    {
        UWidget* OriginalRoot = WidgetTree->RootWidget;
        UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestActionLayout"));
        WidgetTree->RootWidget = Layout;
        if (OriginalRoot) Layout->AddChildToVerticalBox(OriginalRoot);
        QuestAbandonButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestAbandonButton"));
        Layout->AddChildToVerticalBox(QuestAbandonButton);
    }
    bReaccept = Quest.bAbandoned;
    UTextBlock* Label = Cast<UTextBlock>(QuestAbandonButton->GetContent());
    if (!Label && !QuestAbandonButton->GetContent())
    {
        Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        QuestAbandonButton->AddChild(Label);
    }
    if (Label)
    {
        Label->SetText(bReaccept ? NSLOCTEXT("ICQuest", "Reaccept", "다시 수락")
            : NSLOCTEXT("ICQuest", "Abandon", "포기"));
    }
    const UICQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICQuestSubsystem>() : nullptr;
    const bool bAllowed = Quests && (bReaccept ? Quests->CanAcceptQuest(Definition.ID)
        : Quests->CanAbandonQuest(Definition.ID));
    QuestAbandonButton->SetVisibility(bReaccept || Definition.bAllowAbandon
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    QuestAbandonButton->SetIsEnabled(bAllowed);
    QuestAbandonButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RequestQuestAction);
}

void UICQuestListWidget::RequestQuestAction()
{
    UICQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICQuestSubsystem>() : nullptr;
    if (!Quests) return;
    if (bReaccept)
    {
        Quests->AcceptQuest(DisplayedQuestID);
        return;
    }
    if (!Quests->CanAbandonQuest(DisplayedQuestID) || !GetOwningLocalPlayer()) return;
    UICUIManagerSubsystem* UI = GetOwningLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
    UICConfirmPopupWidget* Popup = CreateWidget<UICConfirmPopupWidget>(GetOwningPlayer(), UICConfirmPopupWidget::StaticClass());
    if (!UI || !Popup) return;
    const FName QuestID = DisplayedQuestID;
    Popup->Configure(NSLOCTEXT("ICQuest", "ConfirmAbandon", "퀘스트를 포기할까요? 다시 수락하면 목표 진행도가 초기화됩니다."),
        FSimpleDelegate::CreateWeakLambda(Quests, [Quests, QuestID]() { Quests->AbandonQuest(QuestID); }));
    UI->OpenScreen(Popup, true, 100);
}
