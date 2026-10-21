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

    TestTrue(TEXT("Quest item delivery consumes dedicated inventory"), Fixture.Quests->DeliverItem(
        TEXT("QST_DELIVER_998"), TEXT("OBJ_RING")));
    World = Fixture.State->ExportState();
    TestTrue(TEXT("Delivery quest completed"), World.Quests.FindChecked(TEXT("QST_DELIVER_998")).bCompleted);
    TestEqual(TEXT("Delivered quest item removed"), World.Inventory.QuestItems.Num(), 0);

    const FGuid EventID = FGuid::NewGuid();
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
    TestEqual(TEXT("All active quests complete without objectives"), Fixture.Quests->ForceCompleteActiveQuests(), 3);

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

#endif
