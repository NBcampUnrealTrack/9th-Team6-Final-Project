#include "UI/Dialogue/ICNPCDialogueWidget.h"

#include "UI/Dialogue/ICDialogueEntryWidget.h"
#include "NPC/Character/ICNPCBase.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "Quest/ICQuestSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"

static const FName DialogueQuestsEntry(TEXT("IC.Dialogue.Quests"));
static const FName DialogueAcceptEntry(TEXT("IC.Dialogue.Accept"));
static const FName DialogueTurnInEntry(TEXT("IC.Dialogue.TurnIn"));
static const FName DialogueNextEntry(TEXT("IC.Dialogue.Next"));
static const FName DialogueBackEntry(TEXT("IC.Dialogue.Back"));
static const FName DialogueCloseEntry(TEXT("IC.Dialogue.Close"));

void UICNPCDialogueWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
}

void UICNPCDialogueWidget::OpenForNPC(AICNPCBase* InNPC)
{
    NPC = InNPC;
    if (!NPC || !GetGameInstance()) return;

    const FICNPCRow& Definition = NPC->GetNPCDefinition();
    const FICTextLookupResult Name = UICTextLibrary::ResolveText(NPCStringTable, Definition.NameKey);
    SpeakerText->SetText(Name.bSuccess ? Name.Text : FText::FromName(Definition.ID));

    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    for (const FName DialogueID : Definition.DialogueIDs)
    {
        FICDialogueRow Dialogue;
        if (Definitions && Definitions->GetDialogue(DialogueID, Dialogue).bSuccess
            && Dialogue.NPCID == Definition.ID)
        {
            ShowNode(DialogueID);
            return;
        }
    }
    ShowQuestList();
}

void UICNPCDialogueWidget::NativeDestruct()
{
    if (NPC) NPC->EndInteraction();
    if (AICPlayerController* Controller = Cast<AICPlayerController>(GetOwningPlayer()))
    {
        Controller->bInDialogue = false;
    }
    Super::NativeDestruct();
}

void UICNPCDialogueWidget::AddEntry(FName EntryID, const FText& Label)
{
    if (!EntryWidgetClass) return;
    UICDialogueEntryWidget* Entry = CreateWidget<UICDialogueEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
    if (!Entry) return;
    Entry->SetEntry(EntryID, Label);
    Entry->OnEntryPressed.AddUniqueDynamic(this, &ThisClass::HandleEntry);
    Entries->AddChildToVerticalBox(Entry);
}

void UICNPCDialogueWidget::ShowQuestList()
{
    SelectedQuestID = NAME_None;
    SelectedDialogueID = NAME_None;
    Entries->ClearChildren();
    DialogueText->SetText(QuestPromptText);
    if (!NPC || !GetGameInstance()) return;

    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    UICWorldStateSubsystem* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    TArray<FName> QuestIDs;
    if (Definitions && Quests && State
        && Definitions->GetDefinitionIDs(EICDefinitionType::Quest, QuestIDs).bSuccess)
    {
        const FICWorldState World = State->ExportState();
        for (const FName QuestID : QuestIDs)
        {
            FICQuestRow Quest;
            if (!Definitions->GetQuest(QuestID, Quest).bSuccess
                || (Quest.OfferNPCID != NPC->GetNPCDefinition().ID
                    && Quest.TurnInNPCID != NPC->GetNPCDefinition().ID)) continue;

            const FICQuestState* Progress = World.Quests.Find(QuestID);
            const bool IsActive = Progress && Progress->bAccepted && !Progress->bCompleted;
            const bool IsOfferable = Quest.OfferNPCID == NPC->GetNPCDefinition().ID
                && Quests->CanOfferQuest(QuestID);
            if (!IsActive && !IsOfferable) continue;

            const bool IsReady = IsActive && Quest.TurnInNPCID == NPC->GetNPCDefinition().ID
                && Quests->AreQuestObjectivesReady(QuestID);
            const FText& Status = IsReady ? ReadyStatusText : IsActive ? ActiveStatusText
                : OfferStatusText;
            const FICTextLookupResult Name = UICTextLibrary::ResolveText(QuestStringTable, Quest.NameKey);
            AddEntry(QuestID, FText::Format(QuestEntryFormatText, Status,
                Name.bSuccess ? Name.Text : FText::FromName(QuestID)));
        }
    }
    AddEntry(DialogueCloseEntry, CloseText);
}

void UICNPCDialogueWidget::ShowQuest(FName QuestID)
{
    if (!NPC || !GetGameInstance()) return;
    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    UICWorldStateSubsystem* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    FICQuestRow Quest;
    if (!Definitions || !Quests || !State || !Definitions->GetQuest(QuestID, Quest).bSuccess)
    {
        ShowQuestList();
        return;
    }

    SelectedQuestID = QuestID;
    const FICWorldState World = State->ExportState();
    const FICQuestState* Progress = World.Quests.Find(QuestID);
    const bool IsActive = Progress && Progress->bAccepted && !Progress->bCompleted;
    FName DialogueID;
    if (IsActive)
    {
        DialogueID = Quest.TurnInNPCID == NPC->GetNPCDefinition().ID
            && Quests->AreQuestObjectivesReady(QuestID)
            ? Quest.ReadyDialogueID : Quest.ActiveDialogueID;
    }
    else if (Quests->CanOfferQuest(QuestID)) DialogueID = Quest.OfferDialogueID;
    else
    {
        ShowQuestList();
        return;
    }

    if (DialogueID.IsNone())
    {
        ShowQuestList();
        return;
    }
    ShowNode(DialogueID);
}

void UICNPCDialogueWidget::ShowNode(FName DialogueID, FName NodeID)
{
    if (!NPC || !GetGameInstance()) return;
    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    FICDialogueRow Dialogue;
    if (!Definitions || !Definitions->GetDialogue(DialogueID, Dialogue).bSuccess
        || Dialogue.NPCID != NPC->GetNPCDefinition().ID)
    {
        ShowQuestList();
        return;
    }

    const FName CurrentNodeID = NodeID.IsNone() ? Dialogue.EntryNodeID : NodeID;
    const FICDialogueNode* Node = Dialogue.Nodes.FindByPredicate(
        [CurrentNodeID](const FICDialogueNode& Candidate) { return Candidate.NodeID == CurrentNodeID; });
    if (!Node)
    {
        ShowQuestList();
        return;
    }
    const FICTextLookupResult Line = UICTextLibrary::ResolveText(DialogueStringTable, Node->TextKey);
    if (!Line.bSuccess)
    {
        ShowQuestList();
        return;
    }

    SelectedDialogueID = DialogueID;
    SelectedNodeID = CurrentNodeID;
    DialogueText->SetText(Line.Text);
    Entries->ClearChildren();
    for (const FICDialogueChoice& Choice : Node->Choices)
    {
        const FICTextLookupResult Label = UICTextLibrary::ResolveText(DialogueStringTable, Choice.TextKey);
        if (Label.bSuccess) AddEntry(Choice.ChoiceID, Label.Text);
    }
    if (!Node->NextNodeID.IsNone()) AddEntry(DialogueNextEntry, NextText);

    if (SelectedQuestID.IsNone()) AddEntry(DialogueQuestsEntry, QuestsText);
    else
    {
        UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
        FICQuestRow Quest;
        if (Node->NextNodeID.IsNone() && Node->Choices.IsEmpty() && Quests
            && Definitions->GetQuest(SelectedQuestID, Quest).bSuccess)
        {
            if (Quests->CanOfferQuest(SelectedQuestID)) AddEntry(DialogueAcceptEntry, AcceptText);
            else if (Quest.TurnInNPCID == NPC->GetNPCDefinition().ID
                && Quests->AreQuestObjectivesReady(SelectedQuestID))
            {
                AddEntry(DialogueTurnInEntry, TurnInText);
            }
        }
        AddEntry(DialogueBackEntry, BackText);
    }
    AddEntry(DialogueCloseEntry, CloseText);
}

void UICNPCDialogueWidget::HandleEntry(FName EntryID)
{
    if (EntryID == DialogueCloseEntry) { CloseDialogue(); return; }
    if (EntryID == DialogueQuestsEntry || EntryID == DialogueBackEntry) { ShowQuestList(); return; }
    if (!NPC || !GetGameInstance()) return;

    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    if (EntryID == DialogueAcceptEntry || EntryID == DialogueTurnInEntry)
    {
        if (Quests && (EntryID == DialogueAcceptEntry ? Quests->AcceptQuest(SelectedQuestID)
            : Quests->TurnInQuest(SelectedQuestID, NPC->GetNPCDefinition().ID))) ShowQuestList();
        return;
    }

    FICQuestRow Quest;
    if (Definitions && Definitions->GetQuest(EntryID, Quest).bSuccess
        && (Quest.OfferNPCID == NPC->GetNPCDefinition().ID
            || Quest.TurnInNPCID == NPC->GetNPCDefinition().ID))
    {
        ShowQuest(EntryID);
        return;
    }

    FICDialogueRow Dialogue;
    if (!Definitions || !Definitions->GetDialogue(SelectedDialogueID, Dialogue).bSuccess) return;
    const FICDialogueNode* Node = Dialogue.Nodes.FindByPredicate(
        [this](const FICDialogueNode& Candidate) { return Candidate.NodeID == SelectedNodeID; });
    if (!Node) return;
    if (EntryID == DialogueNextEntry) { ShowNode(SelectedDialogueID, Node->NextNodeID); return; }
    const FICDialogueChoice* Choice = Node->Choices.FindByPredicate(
        [EntryID](const FICDialogueChoice& Candidate) { return Candidate.ChoiceID == EntryID; });
    if (Choice)
    {
        if (Choice->NextNodeID.IsNone()) ShowQuestList();
        else ShowNode(SelectedDialogueID, Choice->NextNodeID);
    }
}

void UICNPCDialogueWidget::CloseDialogue()
{
    if (AICPlayerController* Controller = Cast<AICPlayerController>(GetOwningPlayer()))
    {
        Controller->SetMenu(TEXT(""));
    }
    else RemoveFromParent();
}

FReply UICNPCDialogueWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
    if (KeyEvent.GetKey() == EKeys::Escape)
    {
        CloseDialogue();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}
