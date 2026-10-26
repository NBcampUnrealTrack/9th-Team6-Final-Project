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
#endif
