#include "PlayerInventory.h"
#include "PlayerSaveGame.h"
#include "PlayerSurvival.h"
#include "PlayerCrafting.h"
#include "PlayerWorldStateSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerInventoryRulesTest,"Isekai.Player.InventoryRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayerInventoryRulesTest::RunTest(const FString& Parameters)
{
    auto* I=NewObject<UPlayerInventory>();
    TestTrue(TEXT("Item IDs follow the planning convention"),I->ValidateDefinitions());
    const auto* SwordDefinition=I->Find(TEXT("EQP_WPN_SWORD_001"));TestNotNull(TEXT("Weapon definition exists"),SwordDefinition);if(SwordDefinition){TestEqual(TEXT("Weapon grade defaults to common"),SwordDefinition->DefaultGrade,EPlayerItemGrade::Common);TestEqual(TEXT("Weapon socket is configured"),SwordDefinition->AttachSocket,FName(TEXT("weapon_r")));TestEqual(TEXT("Weapon stack limit is one"),SwordDefinition->StackLimit,1);}
    FPlayerSurvivalState Survival;TestTrue(TEXT("Initial survival state is valid"),Survival.IsValid());TestEqual(TEXT("Sleep minutes use missing SP"),Survival.CalculateSleepMinutes(2.f),15);Survival.CompleteSleep();TestEqual(TEXT("Sleep restores HP"),Survival.HP,Survival.MaxHP);TestEqual(TEXT("Sleep restores MP"),Survival.MP,Survival.MaxMP);TestEqual(TEXT("Sleep restores SP"),Survival.SP,Survival.MaxSP);Survival.ConsumeSP(1000.f);TestTrue(TEXT("Zero SP causes exhaustion"),Survival.IsExhausted());Survival.RestoreSP(15.f);TestFalse(TEXT("Food clears exhaustion"),Survival.IsExhausted());
    FPlayerMainStats Stats;TestTrue(TEXT("Initial main stats are valid"),Stats.IsValid());TestEqual(TEXT("Intermediate book adds 3"),Stats.Add(EPlayerMainStat::Dexterity,3),3);Stats.Strength=9999;TestEqual(TEXT("Main stat clamps at 10000"),Stats.Add(EPlayerMainStat::Strength,5),1);
    const auto* HighBook=I->Find(TEXT("ITM_BOOK_018"));TestNotNull(TEXT("High luck book exists"),HighBook);if(HighBook){TestEqual(TEXT("High book adds 5"),HighBook->GrowthAmount,5);TestEqual(TEXT("High book targets luck"),HighBook->GrowthStat,EPlayerMainStat::Luck);}
    const auto* Potion=I->Find(TEXT("ITM_CONS_001"));TestNotNull(TEXT("Health potion exists"),Potion);if(Potion)TestEqual(TEXT("Potion restores configured HP"),Potion->RestoreHP,25.f);
    const auto* Bread=I->Find(TEXT("ITM_FOOD_002"));TestNotNull(TEXT("Bread exists"),Bread);if(Bread)TestEqual(TEXT("Bread restores configured SP"),Bread->RestoreSP,15.f);
    auto* Crafting=NewObject<UPlayerCrafting>();TestTrue(TEXT("Recipe definitions are valid"),Crafting->ValidateDefinitions());
    const auto* BreadRecipe=Crafting->Find(TEXT("RCP_COOK_001"));TestNotNull(TEXT("Bread recipe exists"),BreadRecipe);if(BreadRecipe){TestEqual(TEXT("Bread recipe result"),BreadRecipe->ResultItemID,FName(TEXT("ITM_FOOD_002")));TestEqual(TEXT("Bread recipe SP cost"),BreadRecipe->SPCost,5.f);}
    TestEqual(TEXT("Initial capacity"),I->Bag.Slots.Num(),20);
    I->AddItem(TEXT("ITM_MAT_001"),98); I->AddItem(TEXT("ITM_MAT_001"),3);
    TestEqual(TEXT("First stack reaches 99"),I->Bag.Slots[0].Count,99);
    TestEqual(TEXT("Overflow creates 2"),I->Bag.Slots[1].Count,2);
    I->ResetInventory(); I->AddItem(TEXT("EQP_WPN_SWORD_001"),20,false);
    TestTrue(TEXT("Equipment receives random options"),I->Bag.Slots[0].Options.Num()>=1&&I->Bag.Slots[0].Options.Num()<=2);
    TestFalse(TEXT("Full bag rejects atomic add"),I->AddItem(TEXT("ITM_MAT_001"),1,false));
    TestFalse(TEXT("Full loot overflows"),I->AddItem(TEXT("ITM_MAT_001"),3,true));
    TestEqual(TEXT("Overflow is not added"),I->CountItem(TEXT("ITM_MAT_001")),0);
    TestTrue(TEXT("Quest items bypass full bag"),I->AddItem(TEXT("ITM_QST_001"),1));
    TestEqual(TEXT("Separate quest bag"),I->QuestItems.Num(),1);
    TestEqual(TEXT("Quest does not expand bag"),I->Bag.Slots.Num(),20);
    TestFalse(TEXT("Equipment cannot split"),I->SplitStack(0,1));
    int32 Gold=1000;TestTrue(TEXT("First enhancement succeeds"),I->EnhanceEquipment(0,Gold));TestEqual(TEXT("Enhancement level increased"),I->Bag.Slots[0].Upgrade,1);TestEqual(TEXT("Enhancement consumes gold"),Gold,900);
    FGuid ID=I->Bag.Slots[0].InstanceID; I->Bag.Slots[0].Upgrade=7; I->Bag.Slots[0].Options.Add(TEXT("Attack"),12);
    I->Equip(0); TestEqual(TEXT("Equip preserves identity"),I->Equipment[0].InstanceID,ID);TestTrue(TEXT("Unified attack includes equipment"),I->GetEquipmentStats().Attack>=36.f);
    TestTrue(TEXT("Unequip uses freed slot"),I->Unequip(EPlayerEquipSlot::Weapon));
    TestEqual(TEXT("Upgrade preserved"),I->Bag.Slots[0].Upgrade,7);
    I->CurrentClass=1;TestFalse(TEXT("Class blocks another weapon"),I->Equip(0));I->CurrentClass=0;
    I->EnsureStorage(TEXT("Other")); TestTrue(TEXT("Deposit"),I->Transfer(TEXT("Other"),0,true));
    TestEqual(TEXT("Stored identity"),I->Storage[TEXT("Other")].Slots[0].InstanceID,ID);
    TestTrue(TEXT("Withdraw"),I->Transfer(TEXT("Other"),0,false));
    TestTrue(TEXT("Valid persistent state"),I->ValidateState());
    I->ResetInventory(); I->AddItem(TEXT("ITM_TKT_001"),10,false);
    for(int N=0;N<8;++N)TestTrue(TEXT("Expand 10"),I->ExpandUsingTicket());
    TestEqual(TEXT("Capacity cap"),I->Bag.Slots.Num(),100);TestFalse(TEXT("No expansion past cap"),I->ExpandUsingTicket());
    TestEqual(TEXT("Ticket retained at cap"),I->CountItem(TEXT("ITM_TKT_001")),2);
    I->ResetInventory(); I->AddItem(TEXT("ITM_MAT_001"),10);I->BindQuickSlot(0,0);
    TestTrue(TEXT("Split stack"),I->SplitStack(0,4));TestEqual(TEXT("Split conserves total"),I->CountItem(TEXT("ITM_MAT_001")),10);
    I->ToggleLock(0);TestFalse(TEXT("Locked cannot discard"),I->Discard(0));TestFalse(TEXT("Locked cannot move"),I->MoveSlot(0,3));
    I->ToggleLock(0);I->AddItem(TEXT("ITM_CONS_001"),2);I->BindQuickSlot(0,2);I->SortBag();
    TestEqual(TEXT("Quickslot references item not position"),I->QuickSlots[0],FName(TEXT("ITM_CONS_001")));
    I->ResetInventory();I->AddItem(TEXT("ITM_MAT_001"),99);I->AddItem(TEXT("EQP_WPN_SWORD_001"),19);
    TMap<FName,int32> Costs;Costs.Add(TEXT("ITM_MAT_001"),1);
    TestFalse(TEXT("Recipe space failure atomic"),I->ExchangeItems(Costs,TEXT("ITM_CONS_001"),1));
    TestEqual(TEXT("Materials retained"),I->CountItem(TEXT("ITM_MAT_001")),99);
    Costs[TEXT("ITM_MAT_001")]=99;TestTrue(TEXT("Freed ingredient slot usable"),I->ExchangeItems(Costs,TEXT("ITM_CONS_001"),1));
    I->ResetInventory();I->AddItem(TEXT("ITM_MAT_002"),2,false);TestTrue(TEXT("Craft bread from recipe"),Crafting->Craft(TEXT("RCP_COOK_001"),I));TestEqual(TEXT("Bread crafted"),I->CountItem(TEXT("ITM_FOOD_002")),1);TestEqual(TEXT("Flour consumed"),I->CountItem(TEXT("ITM_MAT_002")),0);
    auto* TestGameInstance=NewObject<UGameInstance>();auto* WorldState=NewObject<UPlayerWorldStateSubsystem>(TestGameInstance);FPlayerPersistentSnapshot Companion;Companion.CharacterID=TEXT("NPC_R01_001");WorldState->SetSnapshot(Companion);TestEqual(TEXT("Companion stat growth"),WorldState->AddMainStat(Companion.CharacterID,EPlayerMainStat::Strength,3),3);FPlayerPersistentSnapshot Result;TestTrue(TEXT("Companion snapshot exists"),WorldState->GetSnapshot(Companion.CharacterID,Result));TestEqual(TEXT("Companion strength stored"),Result.MainStats.Strength,4);
    auto* Save=NewObject<UPlayerPrototypeSave>();Save->Bag=I->Bag;Save->Equipment=I->Equipment;Save->QuestItems=I->QuestItems;Save->QuickSlots=I->QuickSlots;Save->Storage=I->Storage;
    TArray<uint8> Bytes;TestTrue(TEXT("Serialize save"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    auto* Loaded=Cast<UPlayerPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));TestNotNull(TEXT("Deserialize save"),Loaded);
    if(Loaded){TestEqual(TEXT("Saved instance ID"),Loaded->Bag.Slots[1].InstanceID,I->Bag.Slots[1].InstanceID);TestEqual(TEXT("Saved quantity"),Loaded->Bag.Slots[0].Count,1);}
    return true;
}
#endif
