#include "PlayerInventory.h"

void UPlayerInventory::RollRandomOptions(FPlayerItemInstance& Item,const FPlayerItemDefinition& Definition) const
{
    if(Definition.Kind!=EPlayerItemKind::Equipment||Definition.MaxRandomOptions<=0||Definition.OptionPoolID.IsNone())return;
    TArray<FName> Keys;
    if(Definition.OptionPoolID==TEXT("OPT_WPN"))Keys={TEXT("Attack"),TEXT("AttackSpeed"),TEXT("CriticalRate")};
    else if(Definition.OptionPoolID==TEXT("OPT_ARM"))Keys={TEXT("Defense"),TEXT("MaxHP"),TEXT("MaxSP")};
    else Keys={TEXT("Attack"),TEXT("Defense"),TEXT("MaxHP"),TEXT("MaxMP"),TEXT("CriticalRate")};
    for(int32 Index=Keys.Num()-1;Index>0;--Index)Keys.Swap(Index,FMath::RandRange(0,Index));
    const int32 Count=FMath::Min(FMath::RandRange(1,2),FMath::Min(Definition.MaxRandomOptions,Keys.Num()));
    for(int32 Index=0;Index<Count;++Index)
    {
        const FName Key=Keys[Index];float Value=0.f;
        if(Key==TEXT("Attack")||Key==TEXT("Defense"))Value=FMath::RandRange(2,8);
        else if(Key==TEXT("MaxHP"))Value=FMath::RandRange(5,20);
        else if(Key==TEXT("MaxMP")||Key==TEXT("MaxSP"))Value=FMath::RandRange(3,15);
        else Value=FMath::RandRange(1,5);
        Item.Options.Add(Key,Value);
    }
}

bool UPlayerInventory::EnhanceEquipment(int32 BagIndex,int32& Gold)
{
    if(!Bag.Slots.IsValidIndex(BagIndex))return false;
    auto& Item=Bag.Slots[BagIndex];const auto* Definition=Find(Item.ItemID);
    if(!Definition||Definition->Kind!=EPlayerItemKind::Equipment||Item.bLocked)return Fail(TEXT("강화할 수 없는 아이템입니다."));
    if(Item.Upgrade>=Definition->MaxEnhanceLevel)return Fail(TEXT("이미 최대 강화 단계입니다."));
    const int32 Cost=Definition->EnhanceBaseCost*(Item.Upgrade+1);
    if(Cost<=0||Gold<Cost)return Fail(TEXT("강화 비용이 부족합니다."));
    Gold-=Cost;
    const float SuccessRate=FMath::Clamp(Definition->EnhanceBaseSuccessRate-Item.Upgrade*0.05f,0.5f,1.f);
    if(FMath::FRand()>SuccessRate)
    {
        OnMessage.Broadcast(FText::FromString(FString::Printf(TEXT("강화 실패: %d 골드 소모, 단계는 유지됩니다."),Cost)));
        return false;
    }
    ++Item.Upgrade;OnChanged.Broadcast();
    OnMessage.Broadcast(FText::FromString(FString::Printf(TEXT("강화 성공: +%d, %d 골드 소모"),Item.Upgrade,Cost)));
    return true;
}

FPlayerEquipmentStats UPlayerInventory::GetEquipmentStats() const
{
    FPlayerEquipmentStats Result;
    for(const auto& Item:Equipment)
    {
        if(Item.IsEmpty())continue;const auto* Definition=Find(Item.ItemID);if(!Definition)continue;
        Result.Attack+=Definition->BaseAttack+Definition->EnhanceAttackPerLevel*Item.Upgrade;
        Result.Defense+=Definition->BaseDefense+Definition->EnhanceDefensePerLevel*Item.Upgrade;
        Result.Attack+=Item.Options.FindRef(TEXT("Attack"));Result.Defense+=Item.Options.FindRef(TEXT("Defense"));
        Result.MaxHP+=Item.Options.FindRef(TEXT("MaxHP"));Result.MaxMP+=Item.Options.FindRef(TEXT("MaxMP"));Result.MaxSP+=Item.Options.FindRef(TEXT("MaxSP"));
        Result.AttackSpeed+=Item.Options.FindRef(TEXT("AttackSpeed"));Result.MoveSpeed+=Item.Options.FindRef(TEXT("MoveSpeed"));Result.CriticalRate+=Item.Options.FindRef(TEXT("CriticalRate"));
    }
    return Result;
}
