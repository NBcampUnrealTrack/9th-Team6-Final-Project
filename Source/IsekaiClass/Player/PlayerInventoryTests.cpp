#include "PlayerInventory.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "PlayerSaveGame.h"
#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerInventoryRulesTest,"Isekai.Player.InventoryRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayerInventoryRulesTest::RunTest(const FString& Parameters)
{
    auto* Instance=NewObject<UGameInstance>();auto* State=NewObject<UICWorldStateSubsystem>(Instance);auto* I=NewObject<UPlayerInventory>();I->BindWorldState(State);
    TestEqual(TEXT("Initial capacity"),I->GetSnapshot().Bag.Slots.Num(),20);
    I->AddItem(TEXT("Wood"),98); I->AddItem(TEXT("Wood"),3);
    TestEqual(TEXT("First stack reaches 99"),I->GetSnapshot().Bag.Slots[0].Count,99);
    TestEqual(TEXT("Overflow creates 2"),I->GetSnapshot().Bag.Slots[1].Count,2);
    I->ResetInventory(); I->AddItem(TEXT("Sword"),20,false);
    TestFalse(TEXT("Full bag rejects atomic add"),I->AddItem(TEXT("Wood"),1,false));
    TestFalse(TEXT("Full partial add returns failure"),I->AddItem(TEXT("Wood"),3,true));
    TestEqual(TEXT("TryAddItem returns full remainder"),I->TryAddItem(TEXT("Wood"),3),3);
    TestEqual(TEXT("Overflow does not enter bag"),I->CountItem(TEXT("Wood")),0);
    TestTrue(TEXT("Quest items bypass full bag"),I->AddItem(TEXT("QuestRing"),1));
    TestEqual(TEXT("Separate quest bag"),I->GetSnapshot().QuestItems.Num(),1);
    TestEqual(TEXT("Quest does not expand bag"),I->GetSnapshot().Bag.Slots.Num(),20);
    TestFalse(TEXT("Equipment cannot split"),I->SplitStack(0,1));
    auto Modified=I->GetSnapshot();FGuid ID=Modified.Bag.Slots[0].InstanceID;Modified.Bag.Slots[0].Upgrade=7;Modified.Bag.Slots[0].Options.Add(TEXT("Attack"),12);TestTrue(TEXT("Commit upgrade snapshot"),State->CommitInventoryState(Modified));
    I->Equip(0); TestEqual(TEXT("Equip preserves identity"),I->GetSnapshot().Equipment[0].InstanceID,ID);
    TestTrue(TEXT("Unequip uses freed slot"),I->Unequip(EPlayerEquipSlot::Weapon));
    TestEqual(TEXT("Upgrade preserved"),I->GetSnapshot().Bag.Slots[0].Upgrade,7);
    I->EnsureStorage(TEXT("Other")); TestTrue(TEXT("Deposit"),I->Transfer(TEXT("Other"),0,true));
    TestEqual(TEXT("Stored identity"),I->GetSnapshot().Storage[TEXT("Other")].Slots[0].InstanceID,ID);
    TestTrue(TEXT("Withdraw"),I->Transfer(TEXT("Other"),0,false));
    TestTrue(TEXT("Valid persistent state"),I->ValidateState());
    I->ResetInventory(); I->AddItem(TEXT("Ticket"),10,false);
    for(int N=0;N<8;++N)TestTrue(TEXT("Expand 10"),I->ExpandUsingTicket());
    TestEqual(TEXT("Capacity cap"),I->GetSnapshot().Bag.Slots.Num(),100);TestFalse(TEXT("No expansion past cap"),I->ExpandUsingTicket());
    TestEqual(TEXT("Ticket retained at cap"),I->CountItem(TEXT("Ticket")),2);
    I->ResetInventory(); I->AddItem(TEXT("Wood"),10);I->BindQuickSlot(0,0);
    TestTrue(TEXT("Split stack"),I->SplitStack(0,4));TestEqual(TEXT("Split conserves total"),I->CountItem(TEXT("Wood")),10);
    I->ToggleLock(0);TestFalse(TEXT("Locked cannot discard"),I->Discard(0));TestFalse(TEXT("Locked cannot move"),I->MoveSlot(0,3));
    I->ToggleLock(0);I->AddItem(TEXT("PotionHP"),2);I->BindQuickSlot(0,2);I->SortBag();
    TestEqual(TEXT("Quickslot references item not position"),I->GetSnapshot().QuickSlots[0],FName(TEXT("PotionHP")));
    I->ResetInventory();I->AddItem(TEXT("Wood"),99);I->AddItem(TEXT("Sword"),19);
    TMap<FName,int32> Costs;Costs.Add(TEXT("Wood"),1);
    TestFalse(TEXT("Recipe space failure atomic"),I->ExchangeItems(Costs,TEXT("PotionHP"),1));
    TestEqual(TEXT("Materials retained"),I->CountItem(TEXT("Wood")),99);
    Costs[TEXT("Wood")]=99;TestTrue(TEXT("Freed ingredient slot usable"),I->ExchangeItems(Costs,TEXT("PotionHP"),1));
    auto* Save=NewObject<UPlayerPrototypeSave>();Save->Bag=I->GetSnapshot().Bag;Save->Equipment=I->GetSnapshot().Equipment;Save->QuestItems=I->GetSnapshot().QuestItems;Save->QuickSlots=I->GetSnapshot().QuickSlots;Save->Storage=I->GetSnapshot().Storage;
    TArray<uint8> Bytes;TestTrue(TEXT("Serialize save"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    auto* Loaded=Cast<UPlayerPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));TestNotNull(TEXT("Deserialize save"),Loaded);
    if(Loaded){TestEqual(TEXT("Saved instance ID"),Loaded->Bag.Slots[1].InstanceID,I->GetSnapshot().Bag.Slots[1].InstanceID);TestEqual(TEXT("Saved quantity"),Loaded->Bag.Slots[0].Count,1);}
    return true;
}
#endif
