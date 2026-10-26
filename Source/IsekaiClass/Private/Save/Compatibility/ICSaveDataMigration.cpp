#include "Save/Compatibility/ICSaveDataMigration.h"
#include "Debug/ICLog.h"

void ICSaveDataMigration::NormalizeInventoryIDs(FICWorldState& State)
{
    IC_TRACE_FUNCTION();
    static const TMap<FName,FName> ItemIDs={
        {TEXT("PotionHP"),TEXT("ITM_CONS_001")},{TEXT("Food"),TEXT("ITM_FOOD_001")},{TEXT("BookSTR"),TEXT("ITM_BOOK_001")},
        {TEXT("Wood"),TEXT("ITM_MAT_001")},{TEXT("Ticket"),TEXT("ITM_TKT_001")},{TEXT("QuestRing"),TEXT("ITM_QST_001")},
        {TEXT("Sword"),TEXT("EQP_WPN_SWORD_001")},{TEXT("Bow"),TEXT("EQP_WPN_BOW_001")},{TEXT("Staff"),TEXT("EQP_WPN_STAFF_001")},
        {TEXT("HealBook"),TEXT("EQP_WPN_WAND_001")},{TEXT("Clothes"),TEXT("EQP_ARM_001")},{TEXT("Ring"),TEXT("EQP_RNG_001")},{TEXT("Necklace"),TEXT("EQP_NCK_001")}};
    static const TMap<FName,FName> OptionIDs={
        {TEXT("Attack"),TEXT("OPT_ATTACK")},{TEXT("Defense"),TEXT("OPT_DEFENSE")},{TEXT("MaxHP"),TEXT("OPT_MAXHP")},
        {TEXT("MaxMP"),TEXT("OPT_MAXMP")},{TEXT("MaxSP"),TEXT("OPT_MAXSP")},{TEXT("AttackSpeed"),TEXT("OPT_ATTACK_SPEED")},
        {TEXT("MoveSpeed"),TEXT("OPT_MOVE_SPEED")},{TEXT("CriticalRate"),TEXT("OPT_CRITICAL")}};
    auto ID=[&](FName& Value){ IC_TRACE_FUNCTION();if(const auto* New=ItemIDs.Find(Value)){UE_LOG(LogICSave, Verbose, TEXT("[Save.Migrate.ItemID] Before=%s After=%s"),*Value.ToString(),*New->ToString());Value=*New;}};
    auto Item=[&](FPlayerItemInstance& Value)
    {
        IC_TRACE_FUNCTION();
        ID(Value.ItemID);if(Value.OwnerID==TEXT("Player"))Value.OwnerID=TEXT("CHR_PLAYER");
        TMap<FName,float> Options;
        for(const auto& Pair:Value.Options)
        {
            if(const auto* New=OptionIDs.Find(Pair.Key))
            {
                const bool Ratio=Pair.Key==TEXT("AttackSpeed")||Pair.Key==TEXT("MoveSpeed")||Pair.Key==TEXT("CriticalRate");
                UE_LOG(LogICSave, Verbose, TEXT("[Save.Migrate.Option] Before=%s After=%s Value=%.4f Converted=%.4f"),*Pair.Key.ToString(),*New->ToString(),Pair.Value,Ratio?Pair.Value/100.f:Pair.Value);
                Options.FindOrAdd(*New)+=Ratio?Pair.Value/100.f:Pair.Value;
            }
            else Options.FindOrAdd(Pair.Key)+=Pair.Value;
        }
        Value.Options=MoveTemp(Options);
    };
    for(auto& Value:State.Inventory.Bag.Slots)Item(Value);
    for(auto& Value:State.Inventory.QuestItems)Item(Value);
    for(auto& Value:State.Inventory.Equipment)Item(Value);
    for(auto& Value:State.Inventory.WeaponSlots)Item(Value);
    for(auto& Container:State.Inventory.Storage)for(auto& Value:Container.Value.Slots)Item(Value);
    for(auto& Value:State.Inventory.QuickSlots)ID(Value);
    for(auto& Character:State.Characters)for(auto& Value:Character.Value.Equipment)Item(Value);
    for(auto& Drop:State.Drops)for(auto& Value:Drop.Contents)Item(Value);
    for(auto& Job:State.ProductionJobs)for(auto& Value:Job.Value.Materials)ID(Value.ItemID);
    NormalizeWeaponOnly(State);
}

void ICSaveDataMigration::NormalizeWeaponOnly(FICWorldState& State)
{
    auto Retired=[](FName ID){const auto Name=ID.ToString();return Name.StartsWith(TEXT("EQP_ARM_"))||Name.StartsWith(TEXT("EQP_RNG_"))||Name.StartsWith(TEXT("EQP_NCK_"));};
    auto Clean=[&](TArray<FPlayerItemInstance>& Items){for(auto& Item:Items)if(Retired(Item.ItemID))Item={};};
    Clean(State.Inventory.Bag.Slots);Clean(State.Inventory.QuestItems);Clean(State.Inventory.Equipment);Clean(State.Inventory.WeaponSlots);
    if(State.Inventory.Equipment.Num()==4)State.Inventory.Equipment.SetNum(1);
    for(auto& Pair:State.Inventory.Storage)Clean(Pair.Value.Slots);
    for(auto& ID:State.Inventory.QuickSlots)if(Retired(ID))ID=NAME_None;
    for(auto& Pair:State.Characters){Clean(Pair.Value.Equipment);if(Pair.Value.Equipment.Num()==4)Pair.Value.Equipment.SetNum(1);}
    for(auto& Drop:State.Drops){Clean(Drop.Contents);Drop.Contents.RemoveAll([](const auto& Item){return Item.IsEmpty();});}
    State.Drops.RemoveAll([](const auto& Drop){return Drop.Contents.IsEmpty();});
    for(auto It=State.Codex.AcquiredItemIDs.CreateIterator();It;++It)if(Retired(*It))It.RemoveCurrent();
}
