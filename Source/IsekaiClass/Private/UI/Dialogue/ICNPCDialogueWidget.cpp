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

static const FName DialogueTalkEntry(TEXT("IC.Dialogue.Talk"));
static const FName DialogueOptionsEntry(TEXT("IC.Dialogue.Options"));
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

    ShowInteractionOptions();
}

void UICNPCDialogueWidget::ShowInteractionOptions()
{
    SelectedQuestID = NAME_None;
    SelectedDialogueID = NAME_None;
    SelectedNodeID = NAME_None;
    Entries->ClearChildren();
    DialogueText->SetText(FText::GetEmpty());
    if (!NPC) return;

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        UICDefinitionSubsystem* Definitions = GameInstance->GetSubsystem<UICDefinitionSubsystem>();
        const FICNPCRow& Definition = NPC->GetNPCDefinition();
        for (const FName DialogueID : Definition.DialogueIDs)
        {
            FICDialogueRow Dialogue;
            if (!Definitions || !Definitions->GetDialogue(DialogueID, Dialogue).bSuccess
                || Dialogue.NPCID != Definition.ID) continue;

            const FICDialogueNode* Node = Dialogue.Nodes.FindByPredicate(
                [&Dialogue](const FICDialogueNode& Candidate)
                {
                    return Candidate.NodeID == Dialogue.EntryNodeID;
                });
            if (!Node) continue;

            const FICTextLookupResult Greeting = UICTextLibrary::ResolveText(DialogueStringTable, Node->TextKey);
            if (Greeting.bSuccess)
            {
                DialogueText->SetText(Greeting.Text);
                break;
            }
        }
    }

    AddQuestEntries();
    for (const FICNPCInteractionOption& Option : NPC->GetInteractionOptions())
    {
        FName EntryID;
        switch (Option.Option)
        {
        case EICNPCInteractionOption::Talk: EntryID = DialogueTalkEntry; break;
        case EICNPCInteractionOption::End: EntryID = DialogueCloseEntry; break;
        default: continue;
        }
        AddEntry(EntryID, Option.Label, Option.bEnabled);
    }
}

void UICNPCDialogueWidget::ShowInitialDialogue()
{
    if (!NPC || !GetGameInstance()) return;

    const FICNPCRow& Definition = NPC->GetNPCDefinition();
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
    ShowInteractionOptions();
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

void UICNPCDialogueWidget::AddEntry(FName EntryID, const FText& Label, bool IsEnabled)
{
    if (!EntryWidgetClass) return;
    UICDialogueEntryWidget* Entry = CreateWidget<UICDialogueEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
    if (!Entry) return;
    Entry->SetEntry(EntryID, Label, IsEnabled);
    Entry->OnEntryPressed.AddUniqueDynamic(this, &ThisClass::HandleEntry);
    Entries->AddChildToVerticalBox(Entry);
}

void UICNPCDialogueWidget::AddQuestEntries()
{
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
            if (!IsOfferable && !IsReady) continue;
            const FICTextLookupResult Name = UICTextLibrary::ResolveText(QuestStringTable, Quest.NameKey);
            const FText QuestName = Name.bSuccess ? Name.Text : FText::FromName(QuestID);
            AddEntry(QuestID, IsReady
                ? FText::Format(QuestEntryFormatText, ReadyStatusText, QuestName)
                : QuestName);
        }
    }
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
        ShowInteractionOptions();
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
        ShowInteractionOptions();
        return;
    }

    if (DialogueID.IsNone())
    {
        SelectedDialogueID = NAME_None;
        SelectedNodeID = NAME_None;
        Entries->ClearChildren();
        const FICTextLookupResult Description = UICTextLibrary::ResolveText(QuestStringTable, Quest.DescriptionKey);
        DialogueText->SetText(Description.bSuccess ? Description.Text : FText::FromName(QuestID));
        if (IsActive)
        {
            if (Quest.TurnInNPCID == NPC->GetNPCDefinition().ID && Quests->AreQuestObjectivesReady(QuestID))
            {
                AddEntry(DialogueTurnInEntry, TurnInText);
            }
        }
        else
        {
            AddEntry(DialogueAcceptEntry, AcceptText);
        }
        AddEntry(DialogueBackEntry, BackText);
        AddEntry(DialogueCloseEntry, CloseText);
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
        ShowInteractionOptions();
        return;
    }

    const FName CurrentNodeID = NodeID.IsNone() ? Dialogue.EntryNodeID : NodeID;
    const FICDialogueNode* Node = Dialogue.Nodes.FindByPredicate(
        [CurrentNodeID](const FICDialogueNode& Candidate) { return Candidate.NodeID == CurrentNodeID; });
    if (!Node)
    {
        ShowInteractionOptions();
        return;
    }
    const FICTextLookupResult Line = UICTextLibrary::ResolveText(DialogueStringTable, Node->TextKey);
    if (!Line.bSuccess)
    {
        ShowInteractionOptions();
        return;
    }

    SelectedDialogueID = DialogueID;
    if (NodeID.IsNone())
    {
        DialogueEventID = FGuid::NewGuid();
    }
    SelectedNodeID = CurrentNodeID;
    DialogueText->SetText(Line.Text);
    Entries->ClearChildren();
    for (const FICDialogueChoice& Choice : Node->Choices)
    {
        const FICTextLookupResult Label = UICTextLibrary::ResolveText(DialogueStringTable, Choice.TextKey);
        if (Label.bSuccess) AddEntry(Choice.ChoiceID, Label.Text);
    }
    if (!Node->NextNodeID.IsNone()) AddEntry(DialogueNextEntry, NextText);

    if (SelectedQuestID.IsNone())
    {
        AddEntry(DialogueOptionsEntry, BackText);
    }
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
    if (EntryID == DialogueCloseEntry || EntryID == DialogueOptionsEntry
        || EntryID == DialogueBackEntry)
    {
        CompleteDialogue();
    }
    if (EntryID == DialogueCloseEntry) { CloseDialogue(); return; }
    if (EntryID == DialogueTalkEntry) { ShowInitialDialogue(); return; }
    if (EntryID == DialogueOptionsEntry || EntryID == DialogueBackEntry)
    {
        ShowInteractionOptions();
        return;
    }
    if (!NPC || !GetGameInstance()) return;

    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    if (EntryID == DialogueAcceptEntry || EntryID == DialogueTurnInEntry)
    {
        if (Quests && (EntryID == DialogueAcceptEntry ? Quests->AcceptQuest(SelectedQuestID)
            : Quests->TurnInQuest(SelectedQuestID, NPC->GetNPCDefinition().ID))) ShowInteractionOptions();
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
        if (Choice->NextNodeID.IsNone())
        {
            CompleteDialogue(true);
            ShowInteractionOptions();
        }
        else ShowNode(SelectedDialogueID, Choice->NextNodeID);
    }
}

void UICNPCDialogueWidget::CompleteDialogue(bool IsTerminalChoice)
{
    if (!NPC || !GetGameInstance() || !DialogueEventID.IsValid())
    {
        return;
    }
    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    FICDialogueRow Dialogue;
    if (!Definitions || !Quests || !Definitions->GetDialogue(SelectedDialogueID, Dialogue).bSuccess)
    {
        return;
    }
    const FICDialogueNode* Node = Dialogue.Nodes.FindByPredicate(
        [this](const FICDialogueNode& Candidate) { return Candidate.NodeID == SelectedNodeID; });
    if (!Node || (!IsTerminalChoice && (!Node->NextNodeID.IsNone() || !Node->Choices.IsEmpty())))
    {
        return;
    }
    const FGuid EventID = DialogueEventID;
    DialogueEventID.Invalidate();
    Quests->ReportObjectiveEvent(EICQuestObjectiveType::Talk, NPC->GetNPCDefinition().ID,
        1, EventID, EICLifeActivity::Gathering, NAME_None, SelectedDialogueID);
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
