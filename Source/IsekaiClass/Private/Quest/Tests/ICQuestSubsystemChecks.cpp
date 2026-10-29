#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Quest/ICQuestSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

struct FICQuestFixture
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UICDefinitionCatalog* Catalog = NewObject<UICDefinitionCatalog>();
    UICDefinitionSubsystem* Definitions = NewObject<UICDefinitionSubsystem>(GameInstance);
    UICWorldStateSubsystem* State = NewObject<UICWorldStateSubsystem>(GameInstance);
    UICTimeSubsystem* Time = NewObject<UICTimeSubsystem>(GameInstance);
    UICQuestSubsystem* Quests = NewObject<UICQuestSubsystem>(GameInstance);
    UICInventoryComponent* Inventory = NewObject<UICInventoryComponent>();

    template<class T>
    void AddTable(EICDefinitionType Type, const TArray<T>& Rows)
    {
        UDataTable* Table = NewObject<UDataTable>(GameInstance);
        Table->RowStruct = T::StaticStruct();
        for (const T& Row : Rows)
        {
            Table->AddRow(Row.ID, Row);
        }
        FICDefinitionBinding Binding;
        Binding.Type = Type;
        Binding.RowStruct = T::StaticStruct();
        Binding.Table = Table;
        Catalog->Tables.Add(Binding);
    }

    FICQuestFixture()
    {
        FICItemRow Ring;
        Ring.ID = Ring.ItemID = TEXT("ITM_QST_998");
        Ring.Kind = EPlayerItemKind::Quest;
        Ring.Tab = EPlayerItemTab::Quest;
        FICItemRow Food;
        Food.ID = Food.ItemID = TEXT("ITM_FOOD_998");
        Food.Kind = EPlayerItemKind::Consumable;
        AddTable<FICItemRow>(EICDefinitionType::Item, {Ring, Food});

        FICRewardRow Reward;
        Reward.ID = TEXT("RWD_RING_998");
        FICItemAmount FoodAmount;
        FoodAmount.ItemID = Food.ID;
        FoodAmount.Count = 10;
        Reward.Items.Add(FoodAmount);
        AddTable<FICRewardRow>(EICDefinitionType::Reward, {Reward});

        FICQuestRow RingQuest;
        RingQuest.ID = TEXT("QST_RING_998");
        RingQuest.IsAutoAccept = true;
        RingQuest.bAllowAbandon = false;
        RingQuest.CompletionMode = EICQuestCompletionMode::SystemEvent;
        RingQuest.RewardID = Reward.ID;
        FICQuestObjectiveDefinition RingObjective;
        RingObjective.ObjectiveID = TEXT("OBJ_RING");
        RingObjective.Type = EICQuestObjectiveType::PossessItem;
        RingObjective.Target.Type = EICDefinitionType::Item;
        RingObjective.Target.ID = Ring.ID;
        FICQuestObjectiveGroup RingGroup;
        RingGroup.Objectives.Add(RingObjective);
        RingQuest.ObjectiveGroups.Add(RingGroup);

        FICQuestRow DefeatQuest = RingQuest;
        DefeatQuest.ID = TEXT("QST_DEFEAT_998");
        DefeatQuest.ObjectiveGroups[0].Objectives[0].Type = EICQuestObjectiveType::Defeat;
        DefeatQuest.ObjectiveGroups[0].Objectives[0].Target.Type = EICDefinitionType::Monster;
        DefeatQuest.ObjectiveGroups[0].Objectives[0].Target.ID = TEXT("MON_TEST_998");

        FICQuestRow DeliverQuest = RingQuest;
        DeliverQuest.ID = TEXT("QST_DELIVER_998");
        DeliverQuest.ObjectiveGroups[0].Objectives[0].Type = EICQuestObjectiveType::DeliverItem;
        AddTable<FICQuestRow>(EICDefinitionType::Quest, {RingQuest, DefeatQuest, DeliverQuest});

        Definitions->ConfigureCatalog(Catalog);
        Inventory->BeginStaging(Definitions, 0);
        Quests->ConfigureServices(Definitions, State, Time);
        FICWorldState World;
        World.bPlayerCreated = true;
        World.PlayerName = TEXT("검증");
        World.FirstSkill = TEXT("SKL_FIXTURE");
        World.Inventory = Inventory->GetSnapshot();
        FICCharacterState Player;
        Player.CharacterId = UICWorldStateSubsystem::PlayerId();
        Player.Kind = EICCharacterKind::Player;
        World.Characters.Add(Player.CharacterId, Player);
        State->RestoreState(World);
        Inventory->BindWorldState(State);
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestStorageCheck, "IC.Runtime.QuestPossessionExcludesStorage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICQuestStorageCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    FICQuestObjectiveDefinition Objective;Objective.Type=EICQuestObjectiveType::PossessItem;
    Objective.RequiredCount=10;
    Objective.Target.Type=EICDefinitionType::Item;Objective.Target.ID=TEXT("ITM_FOOD_998");
    auto World=Fixture.State->ExportState();
    FPlayerItemInstance Item;Item.InstanceID=FGuid::NewGuid();Item.ItemID=Objective.Target.ID;Item.Count=5;
    World.Inventory.Storage.FindOrAdd(TEXT("PlayerHome")).Slots.Add(Item);
    int32 Count=-1;FICQuestState Quest;
    TestTrue(TEXT("Progress resolves"),Fixture.Quests->GetObjectiveProgress(Objective,Quest,World,Count));
    TestEqual(TEXT("Stored items do not count"),Count,0);
    World.Inventory.Bag.Slots[0]=Item;
    Fixture.Quests->GetObjectiveProgress(Objective,Quest,World,Count);
    TestEqual(TEXT("Withdrawal counts carried items only"),Count,5);
    World.Inventory.Bag.Slots[0]=FPlayerItemInstance();
    Fixture.Quests->GetObjectiveProgress(Objective,Quest,World,Count);
    TestEqual(TEXT("Deposit removes possession progress"),Count,0);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestRuntimeCheck, "IC.Runtime.QuestLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestRuntimeCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    FICWorldState World = Fixture.State->ExportState();
    const FICQuestState* Ring = World.Quests.Find(TEXT("QST_RING_998"));
    TestTrue(TEXT("Ring quest auto accepted"), Ring && Ring->bAccepted);
    TestEqual(TEXT("Reward absent before goal"), Fixture.Inventory->CountItem(TEXT("ITM_FOOD_998")), 0);

    FICQuestRow DefeatDefinition;
    Fixture.Definitions->GetQuest(TEXT("QST_DEFEAT_998"), DefeatDefinition);
    FICQuestObjectiveDefinition DefeatObjective = DefeatDefinition.ObjectiveGroups[0].Objectives[0];
    DefeatObjective.RequiredCount = 10;
    int32 CurrentCount = -1;
    TestTrue(TEXT("Defeat progress available"), Fixture.Quests->GetObjectiveProgress(DefeatObjective,
        World.Quests.FindChecked(TEXT("QST_DEFEAT_998")), World, CurrentCount));
    TestEqual(TEXT("Defeat progress starts at zero"), CurrentCount, 0);

    int32 RunningCount=0;for(const auto& Pair:World.Quests)if(Pair.Value.bStarted&&!Pair.Value.bCompleted)++RunningCount;
    TestEqual(TEXT("Automatic acceptance starts only one quest"),RunningCount,1);
    TestTrue(TEXT("Start possession quest"),Fixture.Quests->StartQuest(TEXT("QST_RING_998")));
    TestTrue(TEXT("Existing quest item granted"), Fixture.Inventory->AddItem(TEXT("ITM_QST_998"), 1, false));
    World = Fixture.State->ExportState();
    FICQuestRow DeliverDefinition;
    Fixture.Definitions->GetQuest(TEXT("QST_DELIVER_998"), DeliverDefinition);
    TestTrue(TEXT("Delivery progress available"), Fixture.Quests->GetObjectiveProgress(
        DeliverDefinition.ObjectiveGroups[0].Objectives[0],
        World.Quests.FindChecked(TEXT("QST_DELIVER_998")), World, CurrentCount));
    TestEqual(TEXT("Owned quest item appears in delivery progress"), CurrentCount, 1);
    Ring = World.Quests.Find(TEXT("QST_RING_998"));
    TestTrue(TEXT("Possession completes quest"), Ring && Ring->bCompleted && Ring->bRewardClaimed);
    TestTrue(TEXT("Reward receipt recorded"), Ring && World.CompletedTransactionIDs.Contains(Ring->RewardTransactionID));
    TestEqual(TEXT("DT reward amount granted"), Fixture.Inventory->CountItem(TEXT("ITM_FOOD_998")), 10);

    TestTrue(TEXT("Start delivery quest"),Fixture.Quests->StartQuest(TEXT("QST_DELIVER_998")));
    TestTrue(TEXT("Quest item delivery consumes dedicated inventory"), Fixture.Quests->DeliverItem(
        TEXT("QST_DELIVER_998"), TEXT("OBJ_RING")));
    World = Fixture.State->ExportState();
    TestTrue(TEXT("Delivery quest completed"), World.Quests.FindChecked(TEXT("QST_DELIVER_998")).bCompleted);
    TestEqual(TEXT("Delivered quest item removed"), World.Inventory.QuestItems.Num(), 0);

    const FGuid EventID = FGuid::NewGuid();
    TestTrue(TEXT("Start defeat quest"),Fixture.Quests->StartQuest(TEXT("QST_DEFEAT_998")));
    TestTrue(TEXT("Defeat event advances shared runner"), Fixture.Quests->ReportObjectiveEvent(
        EICQuestObjectiveType::Defeat, TEXT("MON_TEST_998"), 1, EventID));
    TestFalse(TEXT("Duplicate event rejected"), Fixture.Quests->ReportObjectiveEvent(
        EICQuestObjectiveType::Defeat, TEXT("MON_TEST_998"), 1, EventID));
    World = Fixture.State->ExportState();
    TestTrue(TEXT("Defeat quest completed"), World.Quests.FindChecked(TEXT("QST_DEFEAT_998")).bCompleted);
    TestTrue(TEXT("Defeat progress available after event"), Fixture.Quests->GetObjectiveProgress(DefeatObjective,
        World.Quests.FindChecked(TEXT("QST_DEFEAT_998")), World, CurrentCount));
    TestEqual(TEXT("Defeat progress advances to one of ten"), CurrentCount, 1);
    TestEqual(TEXT("Each quest rewards once"), Fixture.Inventory->CountItem(TEXT("ITM_FOOD_998")), 30);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestForceCompleteCheck, "IC.Runtime.QuestForceComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestForceCompleteCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    auto Waiting=Fixture.State->ExportState();for(auto& Pair:Waiting.Quests)Pair.Value.bStarted=false;
    TestTrue(TEXT("Prepare accepted waiting quests for force-complete checks"),Fixture.State->RestoreState(Waiting));
    TestEqual(TEXT("Waiting quests do not complete through active-quest cheat"),Fixture.Quests->ForceCompleteActiveQuests(),0);
    for(const FName ID:{FName(TEXT("QST_DEFEAT_998")),FName(TEXT("QST_DELIVER_998")),FName(TEXT("QST_RING_998"))})
    {TestTrue(TEXT("Start one quest"),Fixture.Quests->StartQuest(ID));TestEqual(TEXT("Only the started quest is forced complete"),Fixture.Quests->ForceCompleteActiveQuests(),1);}

    const FICWorldState World = Fixture.State->ExportState();
    for (const auto& Entry : World.Quests)
    {
        TestTrue(TEXT("Quest completed with reward receipt"), Entry.Value.bCompleted
            && Entry.Value.bRewardClaimed && World.CompletedTransactionIDs.Contains(Entry.Value.RewardTransactionID));
    }
    TestEqual(TEXT("Quest rewards granted once"), Fixture.Inventory->CountItem(TEXT("ITM_FOOD_998")), 30);
    TestEqual(TEXT("Already completed quests are skipped"), Fixture.Quests->ForceCompleteActiveQuests(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestRepeatCheck, "IC.Runtime.QuestRepeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestRepeatCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    FICQuestRow Repeat;
    Repeat.ID = TEXT("QST_REPEAT_998");
    Repeat.bRepeatable = true;
    Repeat.OfferNPCID = Repeat.TurnInNPCID = TEXT("NPC_R01_101");
    FICQuestObjectiveDefinition Objective;
    Objective.ObjectiveID = TEXT("OBJ_DEFEAT");
    Objective.Type = EICQuestObjectiveType::Defeat;
    Objective.Target.Type = EICDefinitionType::Monster;
    Objective.Target.ID = TEXT("MON_TEST_998");
    FICQuestObjectiveGroup Group;
    Group.Objectives.Add(Objective);
    Repeat.ObjectiveGroups.Add(Group);
    FICDefinitionBinding& Binding = Fixture.Catalog->Tables.Last();
    UDataTable* Table = Binding.Table.LoadSynchronous();
    Table->AddRow(Repeat.ID, Repeat);

    TestTrue(TEXT("Repeat quest is offered"), Fixture.Quests->CanOfferQuest(Repeat.ID));
    TestTrue(TEXT("First acceptance succeeds"), Fixture.Quests->AcceptQuest(Repeat.ID));
    TestFalse(TEXT("Active repeat quest cannot be accepted again"), Fixture.Quests->AcceptQuest(Repeat.ID));
    TestTrue(TEXT("Start repeat quest"),Fixture.Quests->StartQuest(Repeat.ID));
    TestTrue(TEXT("Objective advances"), Fixture.Quests->ReportObjectiveEvent(
        EICQuestObjectiveType::Defeat, Objective.Target.ID, 1, FGuid::NewGuid()));
    TestTrue(TEXT("Ready quest can be reported"), Fixture.Quests->AreQuestObjectivesReady(Repeat.ID));
    TestTrue(TEXT("First report completes"), Fixture.Quests->TurnInQuest(Repeat.ID, Repeat.TurnInNPCID));
    TestTrue(TEXT("Repeat quest is offered immediately"), Fixture.Quests->CanOfferQuest(Repeat.ID));
    TestTrue(TEXT("Second acceptance succeeds"), Fixture.Quests->AcceptQuest(Repeat.ID));
    const FICWorldState World = Fixture.State->ExportState();
    const FICQuestState& Progress = World.Quests.FindChecked(Repeat.ID);
    TestFalse(TEXT("Previous objective progress cleared"), Progress.bCompleted);
    TestEqual(TEXT("Completion history retained"), Progress.CompletionCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestDialogueDeliveryCheck, "IC.Runtime.QuestDialogueDelivery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestDialogueDeliveryCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    FICQuestRow Definition;
    Definition.ID = TEXT("QST_DIALOGUE_DELIVER_998");
    Definition.OfferNPCID = Definition.TurnInNPCID = TEXT("NPC_R01_101");
    Definition.CompletionMode = EICQuestCompletionMode::NPCInteraction;
    Definition.bRepeatable = true;

    FICItemAmount Item;
    Item.ItemID = TEXT("ITM_QST_998");
    Item.Count = 1;
    Definition.AcceptanceQuestItems.Add(Item);

    FICQuestObjectiveDefinition Objective;
    Objective.ObjectiveID = TEXT("OBJ_DELIVER_RING");
    Objective.Type = EICQuestObjectiveType::DeliverItem;
    Objective.Target.Type = EICDefinitionType::Item;
    Objective.Target.ID = Item.ItemID;
    FICQuestObjectiveGroup Group;
    Group.Objectives.Add(Objective);
    Definition.ObjectiveGroups.Add(Group);

    UDataTable* Table = Fixture.Catalog->Tables.Last().Table.LoadSynchronous();
    Table->AddRow(Definition.ID, Definition);

    TestTrue(TEXT("Quest can be accepted"), Fixture.Quests->AcceptQuest(Definition.ID));
    FICWorldState World = Fixture.State->ExportState();
    TestEqual(TEXT("Quest item granted on acceptance"), World.Inventory.QuestItems.Num(), 1);
    TestFalse(TEXT("Waiting quest cannot be reported"),Fixture.Quests->AreQuestObjectivesReady(Definition.ID));
    TestTrue(TEXT("Start delivery from journal"),Fixture.Quests->StartQuest(Definition.ID));
    TestTrue(TEXT("Quest can be reported with granted item"),
        Fixture.Quests->AreQuestObjectivesReady(Definition.ID));
    TestTrue(TEXT("Dialogue report completes quest"),
        Fixture.Quests->TurnInQuest(Definition.ID, Definition.TurnInNPCID));
    World = Fixture.State->ExportState();
    TestTrue(TEXT("Quest completed"), World.Quests.FindChecked(Definition.ID).bCompleted);
    TestEqual(TEXT("Submitted item consumed"), World.Inventory.QuestItems.Num(), 0);

    TestTrue(TEXT("Repeat quest can be accepted again"), Fixture.Quests->AcceptQuest(Definition.ID));
    World = Fixture.State->ExportState();
    TestEqual(TEXT("Repeat quest grants a fresh item"), World.Inventory.QuestItems.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestAcceptTrackingCheck,"IC.Runtime.QuestAcceptTracking",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICQuestAcceptTrackingCheck::RunTest(const FString&)
{
    FICQuestFixture F;auto* Table=F.Catalog->Tables.Last().Table.LoadSynchronous();
    for(FName ID:Table->GetRowNames())
    {FICQuestRow Definition;F.Definitions->GetQuest(ID,Definition);Definition.IsAutoAccept=false;Table->AddRow(ID,Definition);}
    const FName First=TEXT("QST_ACCEPT_FIRST_998"),Second=TEXT("QST_ACCEPT_SECOND_998"),Third=TEXT("QST_ACCEPT_THIRD_998"),Target=TEXT("MON_TEST_998");
    FICQuestRow Template;F.Definitions->GetQuest(TEXT("QST_DEFEAT_998"),Template);
    Template.IsAutoAccept=false;Template.bAllowAbandon=true;Template.RewardID=NAME_None;
    Template.ObjectiveGroups[0].Objectives[0].RequiredCount=3;
    for(FName ID:{First,Second,Third}){auto Definition=Template;Definition.ID=ID;Table->AddRow(ID,Definition);}
    auto Empty=F.State->ExportState();Empty.Quests.Reset();
    TestTrue(TEXT("Begin with an empty quest list"),F.State->RestoreState(Empty));
    TestTrue(TEXT("First NPC acceptance succeeds"),F.Quests->AcceptQuest(First));
    TestTrue(TEXT("An empty HUD automatically tracks the accepted quest"),F.State->ExportState().Quests[First].bStarted);
    TestTrue(TEXT("Automatic tracking advances actual objectives"),F.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat,Target,1,FGuid::NewGuid()));
    const auto Running=F.State->ExportState().Quests[First];
    TestTrue(TEXT("Second acceptance succeeds"),F.Quests->AcceptQuest(Second));
    auto World=F.State->ExportState();
    TestTrue(TEXT("Existing HUD quest keeps running"),World.Quests[First].bStarted);
    TestEqual(TEXT("Accepting another quest preserves existing progress"),World.Quests[First].Objectives.FindRef(TEXT("OBJ_RING")),1);
    TestEqual(TEXT("Accepting another quest preserves its start time"),World.Quests[First].AcceptedAtGameMinute,Running.AcceptedAtGameMinute);
    TestTrue(TEXT("New quest remains accepted in the list"),World.Quests[Second].bAccepted&&!World.Quests[Second].bStarted);
    TestTrue(TEXT("Further events still advance the HUD quest"),F.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat,Target,1,FGuid::NewGuid()));
    World=F.State->ExportState();TestEqual(TEXT("Current quest continues from its old progress"),World.Quests[First].Objectives.FindRef(TEXT("OBJ_RING")),2);
    TestTrue(TEXT("A waiting quest does not collect objective progress"),World.Quests[Second].Objectives.IsEmpty());
    TestTrue(TEXT("Abandon the running quest"),F.Quests->AbandonQuest(First));
    TestFalse(TEXT("Waiting quests are not started implicitly"),F.State->ExportState().Quests[Second].bStarted);
    TestTrue(TEXT("Accept a new quest with an empty HUD and an existing waiting quest"),F.Quests->AcceptQuest(Third));
    World=F.State->ExportState();
    TestTrue(TEXT("The newly accepted quest becomes the HUD quest"),World.Quests[Third].bStarted);
    TestTrue(TEXT("Previous waiting quest stays in the list"),World.Quests[Second].bAccepted&&!World.Quests[Second].bStarted);
    TestFalse(TEXT("Duplicate acceptance is rejected"),F.Quests->AcceptQuest(Second));
    TestTrue(TEXT("Manual Start still switches to an accepted waiting quest"),F.Quests->StartQuest(Second));
    World=F.State->ExportState();TestTrue(TEXT("Exactly the manually selected quest is running"),World.Quests[Second].bStarted&&!World.Quests[Third].bStarted);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestSingleRunCheck,"IC.Runtime.QuestSingleRun",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICQuestSingleRunCheck::RunTest(const FString&)
{
    FICQuestFixture F;const FName First=TEXT("QST_DEFEAT_998"),Second=TEXT("QST_DELIVER_998"),Target=TEXT("MON_TEST_998");
    auto* Table=F.Catalog->Tables.Last().Table.LoadSynchronous();
    for(FName ID:{First,Second})
    {FICQuestRow Definition;F.Definitions->GetQuest(ID,Definition);Definition.bAllowAbandon=true;Definition.ObjectiveGroups[0].Objectives[0].RequiredCount=3;Table->AddRow(ID,Definition);}
    auto Waiting=F.State->ExportState();for(auto& Pair:Waiting.Quests)Pair.Value.bStarted=false;
    TestTrue(TEXT("Restore accepted waiting quests for explicit Start checks"),F.State->RestoreState(Waiting));
    TestFalse(TEXT("Waiting quests ignore combat events"),F.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat,Target,1,FGuid::NewGuid()));
    TestTrue(TEXT("Start first quest"),F.Quests->StartQuest(First));const FGuid FirstEvent=FGuid::NewGuid();
    TestTrue(TEXT("Started quest advances"),F.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat,Target,1,FirstEvent));
    TestFalse(TEXT("Invalid start leaves current quest intact"),F.Quests->StartQuest(TEXT("QST_MISSING")));
    TestTrue(TEXT("Starting current quest is idempotent"),F.Quests->StartQuest(First));
    TestEqual(TEXT("Repeated Start preserves current progress"),F.State->ExportState().Quests[First].Objectives.FindRef(TEXT("OBJ_RING")),1);
    TestTrue(TEXT("Switch quest"),F.Quests->StartQuest(Second));auto World=F.State->ExportState();
    TestTrue(TEXT("Stopped quest remains in the journal"),World.Quests[First].bAccepted&&!World.Quests[First].bStarted);
    TestEqual(TEXT("Switch clears previous progress"),World.Quests[First].Objectives.Num(),0);
    TestTrue(TEXT("Exactly the new quest starts"),World.Quests[Second].bStarted);
    TestFalse(TEXT("Stopped quest ignores further events"),F.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat,Target,1,FGuid::NewGuid()));
    TestTrue(TEXT("Restart former quest"),F.Quests->StartQuest(First));
    TestFalse(TEXT("Old event cannot be replayed after restart"),F.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat,Target,1,FirstEvent));
    TestTrue(TEXT("New event advances restarted quest"),F.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Defeat,Target,1,FGuid::NewGuid()));
    TestTrue(TEXT("Abandon running quest"),F.Quests->AbandonQuest(First));F.Quests->RefreshAvailableQuests();World=F.State->ExportState();
    TestTrue(TEXT("Abandoned auto quest stays removed"),!World.Quests[First].bAccepted&&!World.Quests[First].bStarted&&World.Quests[First].bAbandoned);
    TestEqual(TEXT("Abandon clears progress"),World.Quests[First].Objectives.Num(),0);
    TestTrue(TEXT("Explicit acceptance can reacquire abandoned quest"),F.Quests->AcceptQuest(First));
    TestTrue(TEXT("Reaccepted quest starts when no other quest is running"),F.State->ExportState().Quests[First].bStarted);
    TestFalse(TEXT("Protected quest cannot be abandoned"),F.Quests->AbandonQuest(TEXT("QST_RING_998")));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestReacceptGrantCheck, "IC.Runtime.QuestReacceptGrant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestReacceptGrantCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture F;
    FICQuestRow Definition;
    F.Definitions->GetQuest(TEXT("QST_DELIVER_998"), Definition);
    Definition.ID = TEXT("QST_REACCEPT_998");
    Definition.IsAutoAccept = false;
    Definition.bAllowAbandon = true;
    Definition.RewardID = NAME_None;
    Definition.ObjectiveGroups[0].Objectives[0].RequiredCount = 100;
    FICItemAmount Grant;
    Grant.ItemID = TEXT("ITM_QST_998");
    Grant.Count = 1;
    Definition.AcceptanceQuestItems = {Grant, Grant};
    F.Catalog->Tables.Last().Table.LoadSynchronous()->AddRow(Definition.ID, Definition);
    auto OwnedQuestItems=[&](){int32 Count=0;for(const auto& Item:F.Inventory->GetSnapshot().QuestItems)if(Item.ItemID==Grant.ItemID)Count+=Item.Count;return Count;};
    TestTrue(TEXT("First acceptance grants the configured quest items"), F.Quests->AcceptQuest(Definition.ID));
    TestEqual(TEXT("Duplicate grant rows aggregate to two items"), OwnedQuestItems(), 2);
    TestTrue(TEXT("Granted quest can be abandoned"), F.Quests->AbandonQuest(Definition.ID));
    TestEqual(TEXT("Abandon preserves the quest items"), OwnedQuestItems(), 2);
    TestTrue(TEXT("Abandoned quest can be accepted again"), F.Quests->AcceptQuest(Definition.ID));
    TestEqual(TEXT("Reacceptance does not duplicate retained quest items"), OwnedQuestItems(), 2);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestObjectiveDialogueCheck, "IC.Runtime.QuestObjectiveDialogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICQuestObjectiveDialogueCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    FICQuestRow Quest;
    Quest.ID = TEXT("QST_TALK_998");
    Quest.CompletionMode = EICQuestCompletionMode::NPCInteraction;
    Quest.TurnInNPCID = TEXT("NPC_R01_998");
    FICQuestObjectiveDefinition Objective;
    Objective.ObjectiveID = TEXT("OBJ_TALK");
    Objective.Type = EICQuestObjectiveType::Talk;
    Objective.Target.Type = EICDefinitionType::NPC;
    Objective.Target.ID = Quest.TurnInNPCID;
    Objective.DialogueID = TEXT("DLG_TALK_998");
    FICQuestObjectiveGroup Group;
    Group.Objectives.Add(Objective);
    Quest.ObjectiveGroups.Add(Group);
    Fixture.Catalog->Tables.RemoveAll([](const FICDefinitionBinding& Binding)
        { return Binding.Type == EICDefinitionType::Quest; });
    Fixture.AddTable<FICQuestRow>(EICDefinitionType::Quest, {Quest});
    FICDialogueRow Dialogue;
    Dialogue.ID = Objective.DialogueID;
    Dialogue.NPCID = Objective.Target.ID;
    Dialogue.EntryNodeID = TEXT("START");
    FICDialogueNode Node;
    Node.NodeID = Dialogue.EntryNodeID;
    Node.TextKey = TEXT("DLG_TALK_998.START");
    Dialogue.Nodes.Add(Node);
    Fixture.AddTable<FICDialogueRow>(EICDefinitionType::Dialogue, {Dialogue});
    Fixture.Definitions->ConfigureCatalog(Fixture.Catalog);

    FICWorldState World = Fixture.State->ExportState();
    World.Quests.Reset();
    FICQuestState& Progress = World.Quests.Add(Quest.ID);
    Progress.bAccepted = true;
    Fixture.State->RestoreState(World);
    TestTrue(TEXT("Accepted but unstarted quest uses ordinary dialogue"),
        Fixture.Quests->GetNPCObjectiveDialogue(Dialogue.NPCID).IsNone());
    Progress.bStarted = true;
    Fixture.State->RestoreState(World);
    TestEqual(TEXT("Started objective selects its dedicated dialogue"),
        Fixture.Quests->GetNPCObjectiveDialogue(Dialogue.NPCID), Dialogue.ID);
    TestTrue(TEXT("Other NPCs use ordinary dialogue"),
        Fixture.Quests->GetNPCObjectiveDialogue(TEXT("NPC_R01_997")).IsNone());
    TestFalse(TEXT("A generic conversation cannot satisfy the dedicated objective"),
        Fixture.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Talk, Dialogue.NPCID, 1, FGuid::NewGuid()));
    TestFalse(TEXT("Another dialogue cannot satisfy it"),
        Fixture.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Talk, Dialogue.NPCID, 1,
            FGuid::NewGuid(), EICLifeActivity::Gathering, NAME_None, TEXT("DLG_OTHER_998")));
    TestEqual(TEXT("Selection alone does not advance the objective"),
        Fixture.State->ExportState().Quests.FindChecked(Quest.ID).Objectives.FindRef(Objective.ObjectiveID), 0);
    TestTrue(TEXT("Completing the selected dialogue advances the objective"),
        Fixture.Quests->ReportObjectiveEvent(EICQuestObjectiveType::Talk, Dialogue.NPCID, 1,
            FGuid::NewGuid(), EICLifeActivity::Gathering, NAME_None, Dialogue.ID));
    TestTrue(TEXT("Satisfied objective no longer overrides ordinary dialogue"),
        Fixture.Quests->GetNPCObjectiveDialogue(Dialogue.NPCID).IsNone());
    Progress.bAbandoned = true;
    Fixture.State->RestoreState(World);
    TestTrue(TEXT("Abandoned quest cannot select its dialogue"),
        Fixture.Quests->GetNPCObjectiveDialogue(Dialogue.NPCID).IsNone());
    Progress.bAbandoned = false;
    Progress.bCompleted = true;
    Fixture.State->RestoreState(World);
    TestTrue(TEXT("Completed quest cannot select its dialogue"),
        Fixture.Quests->GetNPCObjectiveDialogue(Dialogue.NPCID).IsNone());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestCappedEventCheck, "IC.Runtime.QuestCappedEvent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICQuestCappedEventCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    FICQuestRow Definition;
    TestTrue(TEXT("Defeat definition exists"),
        Fixture.Definitions->GetQuest(TEXT("QST_DEFEAT_998"), Definition).bSuccess);
    Definition.ID = TEXT("QST_CAPPED_998");
    Definition.IsAutoAccept = false;
    Definition.CompletionMode = EICQuestCompletionMode::NPCInteraction;
    Definition.TurnInNPCID = TEXT("NPC_TEST_998");
    Definition.RewardID = NAME_None;
    Fixture.Catalog->Tables.Last().Table.LoadSynchronous()->AddRow(Definition.ID, Definition);

    FICWorldState World = Fixture.State->ExportState();
    World.Quests.Reset();
    FICQuestState& Progress = World.Quests.Add(Definition.ID);
    Progress.bAccepted = true;
    Progress.bStarted = true;
    TestTrue(TEXT("Restore started quest"), Fixture.State->RestoreState(World));

    const FName Target = Definition.ObjectiveGroups[0].Objectives[0].Target.ID;
    TestTrue(TEXT("First event reaches required count"), Fixture.Quests->ReportObjectiveEvent(
        EICQuestObjectiveType::Defeat, Target, 1, FGuid::NewGuid()));
    const int32 ReceiptCount = Fixture.State->ExportState().ProcessedEventIDs.Num();
    TestFalse(TEXT("New event cannot advance a capped objective"), Fixture.Quests->ReportObjectiveEvent(
        EICQuestObjectiveType::Defeat, Target, 1, FGuid::NewGuid()));
    TestEqual(TEXT("No event receipt is stored for unchanged progress"),
        Fixture.State->ExportState().ProcessedEventIDs.Num(), ReceiptCount);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestInventoryTransitionCheck, "IC.Runtime.QuestInventoryTransition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICQuestInventoryTransitionCheck::RunTest(const FString& Parameters)
{
    FICQuestFixture Fixture;
    FICQuestRow Definition;
    TestTrue(TEXT("Defeat definition exists"),
        Fixture.Definitions->GetQuest(TEXT("QST_DEFEAT_998"), Definition).bSuccess);
    Definition.ID = TEXT("QST_INVENTORY_998");
    Definition.IsAutoAccept = false;
    Definition.RewardID = NAME_None;
    FICQuestObjectiveDefinition& Collect = Definition.ObjectiveGroups[0].Objectives[0];
    Collect.Type = EICQuestObjectiveType::Collect;
    Collect.Target.Type = EICDefinitionType::Item;
    Collect.Target.ID = TEXT("ITM_QST_998");
    FICQuestObjectiveDefinition Possess = Collect;
    Possess.ObjectiveID = TEXT("OBJ_POSSESS");
    Possess.Type = EICQuestObjectiveType::PossessItem;
    Definition.ObjectiveGroups[0].Objectives.Add(Possess);
    Fixture.Catalog->Tables.Last().Table.LoadSynchronous()->AddRow(Definition.ID, Definition);

    FICWorldState World = Fixture.State->ExportState();
    World.Quests.Reset();
    FICQuestState& Progress = World.Quests.Add(Definition.ID);
    Progress.bAccepted = true;
    Progress.bStarted = true;
    TestTrue(TEXT("Restore active inventory quest"), Fixture.State->RestoreState(World));
    TestTrue(TEXT("Picking up the item succeeds"), Fixture.Inventory->AddItem(Collect.Target.ID, 1, false));
    const FICWorldState Result = Fixture.State->ExportState();
    const FICQuestState& Updated = Result.Quests.FindChecked(Definition.ID);
    TestEqual(TEXT("Acquisition counts once"), Updated.Objectives.FindRef(Collect.ObjectiveID), 1);
    TestTrue(TEXT("Acquisition and possession complete in the same inventory update"), Updated.bCompleted);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestAnyDeliveryCheck, "IC.Runtime.QuestAnyDelivery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICQuestAnyDeliveryCheck::RunTest(const FString& Parameters)
{
    const FName RingID(TEXT("ITM_QST_998"));
    const FName FoodID(TEXT("ITM_FOOD_998"));
    for (int32 Scenario = 0; Scenario < 3; ++Scenario)
    {
        FICQuestFixture Fixture;
        FICQuestRow Definition;
        Definition.ID = TEXT("QST_ANY_DELIVERY_998");
        Definition.CompletionMode = EICQuestCompletionMode::NPCInteraction;
        Definition.TurnInNPCID = TEXT("NPC_R01_101");

        FICQuestObjectiveDefinition Ring;
        Ring.ObjectiveID = TEXT("OBJ_RING");
        Ring.Type = EICQuestObjectiveType::DeliverItem;
        Ring.Target.Type = EICDefinitionType::Item;
        Ring.Target.ID = RingID;
        FICQuestObjectiveDefinition Food = Ring;
        Food.ObjectiveID = TEXT("OBJ_FOOD");
        Food.Target.ID = FoodID;
        if (Scenario == 2)
        {
            Definition.ObjectiveGroupPolicy = EICConditionPolicy::Any;
            FICQuestObjectiveGroup RingGroup;
            RingGroup.Objectives.Add(Ring);
            FICQuestObjectiveGroup FoodGroup;
            FoodGroup.Objectives.Add(Food);
            Definition.ObjectiveGroups = {RingGroup, FoodGroup};
        }
        else
        {
            FICQuestObjectiveGroup Group;
            Group.Policy = EICConditionPolicy::Any;
            Group.Objectives = {Ring, Food};
            Definition.ObjectiveGroups.Add(Group);
        }
        Fixture.Catalog->Tables.Last().Table.LoadSynchronous()->AddRow(Definition.ID, Definition);

        FICWorldState World = Fixture.State->ExportState();
        World.Quests.Reset();
        FICQuestState& Progress = World.Quests.Add(Definition.ID);
        Progress.bAccepted = true;
        Progress.bStarted = true;
        TestTrue(TEXT("Restore active delivery quest"), Fixture.State->RestoreState(World));
        if (Scenario != 1)
            TestTrue(TEXT("Add ring"), Fixture.Inventory->AddItem(RingID, 1, false));
        TestTrue(TEXT("Add food"), Fixture.Inventory->AddItem(FoodID, 1, false));

        TestTrue(TEXT("One available alternative is enough"), Fixture.Quests->AreQuestObjectivesReady(Definition.ID));
        TestEqual(TEXT("Readiness preview does not consume food"), Fixture.Inventory->CountItem(FoodID), 1);
        TestTrue(TEXT("Turn in the selected alternative"),
            Fixture.Quests->TurnInQuest(Definition.ID, Definition.TurnInNPCID));
        const FICWorldState Result = Fixture.State->ExportState();
        const FICQuestState& Updated = Result.Quests.FindChecked(Definition.ID);
        TestTrue(TEXT("Quest completes"), Updated.bCompleted);
        TestEqual(TEXT("Only the chosen ring is delivered"), Updated.Objectives.FindRef(Ring.ObjectiveID), Scenario == 1 ? 0 : 1);
        TestEqual(TEXT("Food is delivered only when ring is unavailable"), Updated.Objectives.FindRef(Food.ObjectiveID), Scenario == 1 ? 1 : 0);
        TestEqual(TEXT("Unchosen ring remains in inventory"),
            Fixture.Inventory->CountItem(RingID), 0);
        TestEqual(TEXT("Unchosen food remains in inventory"),
            Fixture.Inventory->CountItem(FoodID), Scenario == 1 ? 0 : 1);
    }

    // An All group must never commit a successful partial delivery.
    {
        FICQuestFixture Fixture;
        FICQuestRow Definition;
        Definition.ID = TEXT("QST_ALL_DELIVERY_998");
        Definition.CompletionMode = EICQuestCompletionMode::NPCInteraction;
        Definition.TurnInNPCID = TEXT("NPC_R01_101");
        FICQuestObjectiveGroup Group;
        for (const FName ItemID : {RingID, FoodID})
        {
            FICQuestObjectiveDefinition Objective;
            Objective.ObjectiveID = ItemID;
            Objective.Type = EICQuestObjectiveType::DeliverItem;
            Objective.Target.Type = EICDefinitionType::Item;
            Objective.Target.ID = ItemID;
            Group.Objectives.Add(Objective);
        }
        Definition.ObjectiveGroups.Add(Group);
        Fixture.Catalog->Tables.Last().Table.LoadSynchronous()->AddRow(Definition.ID, Definition);
        FICWorldState World = Fixture.State->ExportState();
        World.Quests.Reset();
        FICQuestState& Progress = World.Quests.Add(Definition.ID);
        Progress.bAccepted = true;
        Progress.bStarted = true;
        TestTrue(TEXT("Restore All delivery quest"), Fixture.State->RestoreState(World));
        TestTrue(TEXT("Add only the first required item"), Fixture.Inventory->AddItem(RingID, 1, false));
        TestFalse(TEXT("All group is not ready with one item"), Fixture.Quests->AreQuestObjectivesReady(Definition.ID));
        TestFalse(TEXT("Incomplete delivery is rejected"),
            Fixture.Quests->TurnInQuest(Definition.ID, Definition.TurnInNPCID));
        TestEqual(TEXT("Rejected transaction keeps the first item"), Fixture.Inventory->CountItem(RingID), 1);
        TestEqual(TEXT("Rejected transaction keeps objective progress"),
            Fixture.State->ExportState().Quests.FindChecked(Definition.ID).Objectives.Num(), 0);
    }
    return !HasAnyErrors();
}
#endif
