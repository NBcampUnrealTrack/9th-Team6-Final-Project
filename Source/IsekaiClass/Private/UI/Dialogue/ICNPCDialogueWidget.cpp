#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "Settings/ICGameUserSettings.h"

#include "UI/Dialogue/ICDialogueEntryWidget.h"
#include "UI/Dialogue/ICNPCGiftWidget.h"
#include "UI/Life/ICLifeWidgets.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "UI/Dialogue/ICAffinityHeartsWidget.h"
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
static const FName DialogueRecruitEntry(TEXT("IC.Dialogue.Recruit"));
static const FName DialogueDismissEntry(TEXT("IC.Dialogue.Dismiss"));
static const FName DialogueTalkEntry(TEXT("IC.Dialogue.Talk"));
static const FName DialogueGiftEntry(TEXT("IC.Dialogue.Gift"));
static const FName DialogueTradeEntry(TEXT("IC.Dialogue.Trade"));
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
    BoundState = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    if (BoundState)
    {
        BoundState->OnCharacterChanged.AddUniqueDynamic(this, &ThisClass::CharacterChanged);
        BoundState->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::RefreshAffinity);
    }
    RefreshAffinity();
}

void UICNPCDialogueWidget::OpenForNPC(AICNPCBase* InNPC)
{
    NPC = InNPC;
    if (!NPC || !GetGameInstance()) return;

    const FICNPCRow& Definition = NPC->GetNPCDefinition();
    const FICTextLookupResult Name = UICTextLibrary::ResolveText(NPCStringTable, Definition.NameKey);
    SpeakerText->SetText(Name.bSuccess ? Name.Text : FText::FromName(Definition.ID));
    RefreshAffinity();

    SelectedQuestID=NAME_None;
    ShowInitialDialogue();
}

void UICNPCDialogueWidget::ShowInteractionOptions()
{
    if (bUpdatingOptions) return;
    TGuardValue<bool> Guard(bUpdatingOptions, true);
    bShowingInteractionOptions = true;
    bRecruitEntryShown = false;
    bDismissEntryShown = false;
    SelectedQuestID = NAME_None;
    ResetDialogue();
    ClearEntries();
    DialogueText->SetText(FText::GetEmpty());
    if (!NPC) return;

    const auto Options = NPC->GetInteractionOptions();
    for (const auto& Option : Options)
        if (Option.Option == EICNPCInteractionOption::Recruit || Option.Option == EICNPCInteractionOption::Dismiss)
        {
            const bool Dismiss = Option.Option == EICNPCInteractionOption::Dismiss;
            AddEntry(Dismiss ? DialogueDismissEntry : DialogueRecruitEntry, Option.Label, Option.bEnabled);
            bDismissEntryShown = Dismiss;
            bRecruitEntryShown = !Dismiss;
        }
    AddEntry(DialogueQuestsEntry, QuestsText);
    if(GetGameInstance())
    {auto* Definitions=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;for(FName ServiceID:NPC->GetNPCDefinition().ServiceIDs)if(const auto* Service=Definitions->Find<FICServiceRow>(ServiceID,R);Service&&Service->Type==EICServiceType::Shop){AddEntry(DialogueTradeEntry,NSLOCTEXT("ICShop","DialogueEntry","상점 · 구매 / 판매"));break;}}
    for (const FICNPCInteractionOption& Option : Options)
    {
        FName EntryID;
        switch (Option.Option)
        {
        case EICNPCInteractionOption::Gift: EntryID = DialogueGiftEntry; break;
        case EICNPCInteractionOption::End: EntryID = DialogueCloseEntry; break;
        default: continue;
        }
        const bool IsSupported = Option.Option == EICNPCInteractionOption::Talk
            || Option.Option == EICNPCInteractionOption::Gift
            || Option.Option == EICNPCInteractionOption::End;
        AddEntry(EntryID, Option.Label, Option.bEnabled && IsSupported);
    }
}

void UICNPCDialogueWidget::ResetDialogue()
{
    SelectedDialogueID = NAME_None;
    SelectedNodeID = NAME_None;
    DialogueEventID.Invalidate();
}

void UICNPCDialogueWidget::ShowInitialDialogue()
{
    if (!NPC || !GetGameInstance()) return;

    SelectedQuestID = NAME_None;
    ResetDialogue();
    const FICNPCRow& Definition = NPC->GetNPCDefinition();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    const FName ObjectiveDialogueID = Quests ? Quests->GetNPCObjectiveDialogue(Definition.ID) : NAME_None;
    if (!ObjectiveDialogueID.IsNone() && TryShowNode(ObjectiveDialogueID)) return;
    for (FName DialogueID : Definition.DialogueIDs)
    {
        if (TryShowNode(DialogueID)) return;
    }
    bShowingInteractionOptions = false;
    DialogueEventID = FGuid::NewGuid();
    ClearEntries();
    DialogueText->SetText(NSLOCTEXT("ICDialogue","Greeting","안녕하세요. 무슨 일로 오셨나요?"));
    AddEntry(DialogueOptionsEntry, NextText);
}

void UICNPCDialogueWidget::NativeDestruct()
{
    if(GiftPicker)
    {
        auto* Closing=GiftPicker.Get();GiftPicker=nullptr;
        Closing->OnClosed.RemoveDynamic(this,&ThisClass::GiftPickerClosed);Closing->ClosePicker();
    }
    if (BoundState)
    {
        BoundState->OnCharacterChanged.RemoveDynamic(this, &ThisClass::CharacterChanged);
        BoundState->OnWorldRestored.RemoveDynamic(this, &ThisClass::RefreshAffinity);
        BoundState = nullptr;
    }
    if (NPC) NPC->EndInteraction();
    NPC = nullptr;
    if (AICPlayerController* Controller = Cast<AICPlayerController>(GetOwningPlayer()))
    {
        Controller->bInDialogue = false;
    }
    Super::NativeDestruct();
}

void UICNPCDialogueWidget::CharacterChanged(FName CharacterID)
{
    if (IsValid(NPC) && NPC->GetNPCDefinition().ID == CharacterID) RefreshAffinity();
}

void UICNPCDialogueWidget::RefreshAffinity()
{
    auto* State = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    const auto* Character = State && IsValid(NPC) ? State->GetStoredCharacter(NPC->GetNPCDefinition().ID) : nullptr;
    if (AffinityHearts) AffinityHearts->SetAffinity(Character ? Character->Affinity : 0);
    if (bShowingInteractionOptions && !bUpdatingOptions && IsValid(NPC)
        && (bRecruitEntryShown != NPC->CanRecruit() || bDismissEntryShown != NPC->CanDismiss()))
        ShowInteractionOptions();
}

void UICNPCDialogueWidget::AddEntry(FName EntryID, const FText& Label, bool IsEnabled)
{
    if (!EntryWidgetClass) return;
    UICDialogueEntryWidget* Entry = CreateWidget<UICDialogueEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
    if (!Entry) return;
    Entry->SetEntry(EntryID, Label, IsEnabled);
    Entry->OnEntryPressed.AddUniqueDynamic(this, &ThisClass::HandleEntry);
    Entry->OnEntryFocused.AddUniqueDynamic(this, &ThisClass::HandleEntryFocused);
    Entries->AddChildToVerticalBox(Entry);
    if(!SelectedEntry.IsValid()&&Entry->CanSelectEntry())SelectEntry(Entry);
}

void UICNPCDialogueWidget::ShowGiftPicker()
{
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(GiftPicker||!IsValid(NPC)||!PC||!GetOwningLocalPlayer())return;
    auto* Class=LoadClass<UICNPCGiftWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_NPCGiftPicker.WBP_IC_NPCGiftPicker_C"));
    GiftPicker=Class?CreateWidget<UICNPCGiftWidget>(PC,Class):nullptr;
    if(!GiftPicker)return;
    GiftPicker->OnClosed.AddUniqueDynamic(this,&ThisClass::GiftPickerClosed);
    GiftPicker->OpenForNPC(NPC);
    if(!GetOwningLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>()->OpenScreen(GiftPicker,true,200))
    {GiftPicker->OnClosed.RemoveDynamic(this,&ThisClass::GiftPickerClosed);GiftPicker=nullptr;}
}

void UICNPCDialogueWidget::GiftPickerClosed(const FText& Message)
{
    if(GiftPicker)GiftPicker->OnClosed.RemoveDynamic(this,&ThisClass::GiftPickerClosed);
    GiftPicker=nullptr;
    if(!IsValid(NPC))return;
    ShowInteractionOptions();
    if(!Message.IsEmpty())DialogueText->SetText(Message);
    for(auto* Child:Entries->GetAllChildren())
        if(auto* Entry=Cast<UICDialogueEntryWidget>(Child);Entry&&Entry->GetEntryID()==DialogueGiftEntry)
        {SelectEntry(Entry);break;}
}

void UICNPCDialogueWidget::ClearEntries()
{
    SelectedEntry.Reset();bEntryFocusPending=false;
    if(Entries)
    {
        // Slate can deliver delayed focus events after a row has been removed.
        for(auto* Child:Entries->GetAllChildren())
            if(auto* Entry=Cast<UICDialogueEntryWidget>(Child))
            {
                Entry->OnEntryFocused.RemoveDynamic(this,&ThisClass::HandleEntryFocused);
                Entry->OnEntryPressed.RemoveDynamic(this,&ThisClass::HandleEntry);
            }
        Entries->ClearChildren();
    }
}

void UICNPCDialogueWidget::SelectEntry(UICDialogueEntryWidget* Entry,bool RequestFocus)
{
    if(!Entry||Entry->GetParent()!=Entries||!Entry->CanSelectEntry())return;
    SelectedEntry=Entry;
    for(auto* Child:Entries->GetAllChildren())
        if(auto* Option=Cast<UICDialogueEntryWidget>(Child))Option->SetSelected(Option==Entry);
    bEntryFocusPending=RequestFocus;
}

void UICNPCDialogueWidget::HandleEntryFocused(FName EntryID)
{
    for(auto* Child:Entries->GetAllChildren())
        if(auto* Entry=Cast<UICDialogueEntryWidget>(Child);Entry&&Entry->GetEntryID()==EntryID)
        {SelectEntry(Entry,false);return;}
}

void UICNPCDialogueWidget::MoveSelection(int32 Direction)
{
    TArray<UICDialogueEntryWidget*> Options;
    for(auto* Child:Entries->GetAllChildren())
        if(auto* Entry=Cast<UICDialogueEntryWidget>(Child);Entry&&Entry->CanSelectEntry())Options.Add(Entry);
    if(Options.IsEmpty())return;
    const int32 Current=Options.IndexOfByKey(SelectedEntry.Get());
    const int32 Next=Current==INDEX_NONE?0:(Current+Direction+Options.Num())%Options.Num();
    SelectEntry(Options[Next]);
}

void UICNPCDialogueWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    if(GiftPicker)return;
    // New choices join Slate after they are built. Focus them once their path exists.
    if(bEntryFocusPending&&SelectedEntry.IsValid())
    {
        auto* Entry=SelectedEntry.Get();Entry->FocusEntry();
        bEntryFocusPending=!Entry->HasEntryFocus();
    }
}

void UICNPCDialogueWidget::ShowQuestList()
{
    bShowingInteractionOptions = false;
    SelectedQuestID = NAME_None;
    ResetDialogue();
    ClearEntries();
    DialogueText->SetText(QuestPromptText);
    if (!NPC || !GetGameInstance()) return;

    AddQuestEntries();
    AddEntry(DialogueOptionsEntry, BackText);
    AddEntry(DialogueCloseEntry, CloseText);
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
            const FText& Status = IsReady ? ReadyStatusText : IsActive ? ActiveStatusText
                : OfferStatusText;
            const FICTextLookupResult Name = UICTextLibrary::ResolveText(QuestStringTable, Quest.NameKey);
            AddEntry(QuestID, FText::Format(QuestEntryFormatText, Status,
                Name.bSuccess ? Name.Text : FText::FromName(QuestID)));
        }
    }
}

void UICNPCDialogueWidget::ShowQuest(FName QuestID)
{
    bShowingInteractionOptions = false;
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
        ResetDialogue();
        ClearEntries();
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
    if (TryShowNode(DialogueID, NodeID)) return;
    ResetDialogue();
    if (SelectedQuestID.IsNone()) ShowInteractionOptions();
    else ShowQuestList();
}

bool UICNPCDialogueWidget::TryShowNode(FName DialogueID, FName NodeID)
{
    if (!NPC || !GetGameInstance() || DialogueID.IsNone()) return false;
    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    FICDialogueRow Dialogue;
    if (!Definitions || !Definitions->GetDialogue(DialogueID, Dialogue).bSuccess
        || Dialogue.NPCID != NPC->GetNPCDefinition().ID)
    {
        return false;
    }

    const FName CurrentNodeID = NodeID.IsNone() ? Dialogue.EntryNodeID : NodeID;
    const FICDialogueNode* Node = Dialogue.Nodes.FindByPredicate(
        [CurrentNodeID](const FICDialogueNode& Candidate) { return Candidate.NodeID == CurrentNodeID; });
    if (!Node)
    {
        return false;
    }
    const FICTextLookupResult Line = UICTextLibrary::ResolveText(DialogueStringTable, Node->TextKey);
    if (!Line.bSuccess)
    {
        return false;
    }

    bShowingInteractionOptions = false;
    SelectedDialogueID = DialogueID;
    if (NodeID.IsNone())
    {
        DialogueEventID = FGuid::NewGuid();
    }
    SelectedNodeID = CurrentNodeID;
    DialogueText->SetText(Line.Text);
    ClearEntries();
    for (const FICDialogueChoice& Choice : Node->Choices)
    {
        const FICTextLookupResult Label = UICTextLibrary::ResolveText(DialogueStringTable, Choice.TextKey);
        if (Label.bSuccess) AddEntry(Choice.ChoiceID, Label.Text);
    }
    if (!Node->NextNodeID.IsNone()) AddEntry(DialogueNextEntry, NextText);

    if (SelectedQuestID.IsNone())
    {
        if(Node->NextNodeID.IsNone()&&Node->Choices.IsEmpty())AddEntry(DialogueOptionsEntry, NextText);
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
    if(!SelectedQuestID.IsNone())AddEntry(DialogueCloseEntry, CloseText);
    return true;
}

void UICNPCDialogueWidget::HandleEntry(FName EntryID)
{
    if(EntryID==DialogueTradeEntry&&NPC)
    {auto* Definitions=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;for(FName ServiceID:NPC->GetNPCDefinition().ServiceIDs)if(const auto* Service=Definitions->Find<FICServiceRow>(ServiceID,R);Service&&Service->Type==EICServiceType::Shop){UICShopWidget::Open(Cast<AICPlayerController>(GetOwningPlayer()),ServiceID);return;}}
    if (EntryID == DialogueDismissEntry)
    {
        auto* Controller = Cast<AICPlayerController>(GetOwningPlayer());
        if (IsValid(NPC) && NPC->TryDismiss(Controller)) CloseDialogue();
        else ShowInteractionOptions();
        return;
    }
    if (EntryID == DialogueRecruitEntry)
    {
        auto* Controller = Cast<AICPlayerController>(GetOwningPlayer());
        if (IsValid(NPC) && NPC->TryRecruit(Controller)) CloseDialogue();
        else
        {
            ShowInteractionOptions();
            if (Controller) DialogueText->SetText(Controller->NoticeDisplayText);
        }
        return;
    }
    if(EntryID==DialogueGiftEntry)
    {
        ShowGiftPicker();return;
    }
    if (EntryID == DialogueCloseEntry || EntryID == DialogueOptionsEntry
        || EntryID == DialogueQuestsEntry || EntryID == DialogueBackEntry)
    {
        CompleteDialogue();
    }
    if (EntryID == DialogueCloseEntry) { CloseDialogue(); return; }
    if (EntryID == DialogueTalkEntry) { ShowInitialDialogue(); return; }
    if (EntryID == DialogueOptionsEntry) { ShowInteractionOptions(); return; }
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
        if (Choice->NextNodeID.IsNone())
        {
            CompleteDialogue(true);
            if (SelectedQuestID.IsNone()) ShowInteractionOptions();
            else ShowQuestList();
        }
        else ShowNode(SelectedDialogueID, Choice->NextNodeID);
    }
}

void UICNPCDialogueWidget::CompleteDialogue(bool IsTerminalChoice)
{
    if (!NPC || !GetGameInstance() || !DialogueEventID.IsValid()) return;
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    if (!Quests) return;
    if (!SelectedDialogueID.IsNone())
    {
        UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
        FICDialogueRow Dialogue;
        if (!Definitions || !Definitions->GetDialogue(SelectedDialogueID, Dialogue).bSuccess) return;
        const FICDialogueNode* Node = Dialogue.Nodes.FindByPredicate(
            [this](const FICDialogueNode& Candidate) { return Candidate.NodeID == SelectedNodeID; });
        if (!Node || (!IsTerminalChoice && (!Node->NextNodeID.IsNone() || !Node->Choices.IsEmpty()))) return;
    }
    // Invalidate before notifying gameplay: quest completion may trigger UI callbacks.
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
    const FKey Key=KeyEvent.GetKey();
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->ForwardDialogueMovementKey(Key,true))return FReply::Handled();
    if(Key==EKeys::W||Key==EKeys::Up||Key==EKeys::Gamepad_DPad_Up)
    {MoveSelection(-1);return FReply::Handled();}
    if(Key==EKeys::S||Key==EKeys::Down||Key==EKeys::Gamepad_DPad_Down)
    {MoveSelection(1);return FReply::Handled();}
    if(Key==EKeys::Enter||Key==EKeys::Virtual_Accept||UICGameUserSettings::IsGamepadAcceptKey(Key))
    {
        if(!KeyEvent.IsRepeat())
        {
            if(!SelectedEntry.IsValid()||!SelectedEntry->CanSelectEntry())MoveSelection(1);
            if(SelectedEntry.IsValid())SelectedEntry->ActivateEntry();
        }
        return FReply::Handled();
    }
    if (Key == EKeys::Escape || UICGameUserSettings::IsGamepadBackKey(Key))
    {
        CloseDialogue();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}
FReply UICNPCDialogueWidget::NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& KeyEvent)
{
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->ForwardDialogueMovementKey(KeyEvent.GetKey(),false))return FReply::Handled();
    return Super::NativeOnKeyUp(Geometry,KeyEvent);
}
