#include "Inventory/Components/ICInventoryComponent.h"
#include "Debug/ICLog.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

bool UICInventoryComponent::RollRandomOptions(FPlayerItemInstance& Item,const FICItemRow& Definition) const
{
    IC_TRACE_FUNCTION();
    auto* Service=GetDefinitions();FICDefinitionResult R;
    const auto* Gear=Service?Service->Find<FICEquipmentRow>(Definition.EquipmentID,R):nullptr;
    if(!Gear || Gear->ItemID!=Definition.ID || Gear->Slot!=Definition.EquipSlot)return false;
    const auto* Enhancement=Service->Find<FICEnhancementRow>(Gear->EnhancementID,R);
    if(!Enhancement)return false;
    Item.Grade=static_cast<int32>(Gear->DefaultGrade);Item.Options.Reset();
    if(Gear->OptionPoolID.IsNone())return true;
    const auto* Pool=Service->Find<FICOptionPoolRow>(Gear->OptionPoolID,R);if(!Pool||Pool->bAllowDuplicateOptions)return false;
    const auto* Grade=Pool->Grades.FindByPredicate([&](const auto& G){ IC_TRACE_FUNCTION();return G.Grade==Gear->DefaultGrade;});
    if(!Grade || Grade->OptionCount<0 || Grade->OptionCount>4)return false;
    TArray<FICWeightedOption> Options=Grade->Options;TSet<FName> Seen;
    // Resolve every referenced option before rolling anything.
    for(const auto& Entry:Options)
    {
        const auto* Rule=Service->Find<FICOptionRow>(Entry.OptionID,R);
        if(!Rule || Seen.Contains(Entry.OptionID) || !FMath::IsFinite(Entry.Weight) || Entry.Weight<0
            || Rule->Stat==EICStat::None || !FMath::IsFinite(Rule->MinimumValue) || !FMath::IsFinite(Rule->MaximumValue)
            || Rule->MinimumValue>Rule->MaximumValue
            || (!Rule->AllowedSlots.IsEmpty()&&!Rule->AllowedSlots.Contains(Gear->Slot))
            || (!Rule->AllowedWeaponFamilies.IsEmpty()&&!Rule->AllowedWeaponFamilies.Contains(Gear->WeaponFamily)))return false;
        Seen.Add(Entry.OptionID);
    }
    for(int32 Index=0;Index<Grade->OptionCount;++Index)
    {
        double Total=0;for(const auto& Entry:Options)Total+=Entry.Weight;
        if(!FMath::IsFinite(Total)||Total<=0)return false;
        double Roll=FMath::FRand()*Total;int32 Pick=INDEX_NONE;
        for(int32 N=0;N<Options.Num();++N)if(Options[N].Weight>0){Pick=N;Roll-=Options[N].Weight;if(Roll<0)break;}
        if(Pick==INDEX_NONE)return false;
        const auto* Rule=Service->Find<FICOptionRow>(Options[Pick].OptionID,R);if(!Rule)return false;
        Item.Options.Add(Rule->ID,FMath::Lerp(Rule->MinimumValue,Rule->MaximumValue,FMath::FRand()));Options.RemoveAt(Pick);
    }
    UE_LOG(LogICInventory, VeryVerbose, TEXT("[Equipment.Options] Item=%s Instance=%s Options=%d"),*Item.ItemID.ToString(),*Item.InstanceID.ToString(),Item.Options.Num());
    return true;
}

bool UICInventoryComponent::EnhanceEquipment(int32 BagIndex,int32& Gold)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICInventory, Verbose, TEXT("[Equipment.Enhance.Request] BagSlot=%d"),BagIndex);
    if(!WorldState || !WorldState->FlushParticipants())return Fail(TEXT("플레이어 상태를 반영할 수 없습니다."));
    FICWorldState Next=WorldState->ExportState();
    if(!Next.Inventory.Bag.Slots.IsValidIndex(BagIndex))return false;
    auto& Item=Next.Inventory.Bag.Slots[BagIndex];const auto* D=Find(Item.ItemID);
    auto* Service=GetDefinitions();FICDefinitionResult R;
    const auto* Gear=D&&Service?Service->Find<FICEquipmentRow>(D->EquipmentID,R):nullptr;
    const auto* Rules=Gear?Service->Find<FICEnhancementRow>(Gear->EnhancementID,R):nullptr;
    if(!D || !Gear || !Rules || D->Kind!=EPlayerItemKind::Equipment || Item.bLocked)return Fail(TEXT("강화 정의가 없거나 강화할 수 없는 아이템입니다."));
    if(Item.Upgrade>=Rules->MaximumLevel)return Fail(TEXT("이미 최대 강화 단계입니다."));
    const auto* Step=Rules->Steps.FindByPredicate([&](const auto& S){ IC_TRACE_FUNCTION();return S.TargetLevel==Item.Upgrade+1;});
    if(!Step || Step->GoldCost<0 || !FMath::IsFinite(Step->SuccessProbability) || Step->SuccessProbability<0 || Step->SuccessProbability>1)
        return Fail(TEXT("다음 강화 단계의 설정이 올바르지 않습니다."));
    if(Next.Gold<Step->GoldCost)return Fail(TEXT("강화 비용이 부족합니다."));
    const bool Success=FMath::FRand()<Step->SuccessProbability;
    if(Success)++Item.Upgrade;Next.Gold-=Step->GoldCost;
    if(!ValidateSnapshot(Next.Inventory) || !WorldState->CommitGameplayTransaction(Next,FGuid::NewGuid()))
        return Fail(TEXT("강화 거래 검증에 실패했습니다. 아이템과 골드는 변경하지 않았습니다."));
    Gold=Next.Gold;
    UE_LOG(LogICInventory, Log, TEXT("[Equipment.Enhanced] Item=%s Instance=%s Success=%d Level=%d GoldCost=%d RemainingGold=%d"),*Item.ItemID.ToString(),*Item.InstanceID.ToString(),Success,Item.Upgrade,Step->GoldCost,Gold);
    const FString Message=Success?FString::Printf(TEXT("강화 성공: +%d, %d 골드 소모"),Item.Upgrade,Step->GoldCost):FString::Printf(TEXT("강화 실패: +%d 유지, %d 골드 소모"),Item.Upgrade,Step->GoldCost);
    OnMessage.Broadcast(FText::FromString(Message));
    return Success;
}

FPlayerEquipmentStats UICInventoryComponent::GetEquipmentStats() const
{
    IC_TRACE_FUNCTION();
    FPlayerEquipmentStats Result;auto* Service=GetDefinitions();if(!Service)return Result;
    auto Add=[&](EICStat Stat,double Value)
    {
        IC_TRACE_FUNCTION();
        switch(Stat)
        {
            case EICStat::PhysicalAttack:case EICStat::MagicAttack:Result.Attack+=Value;break;
            case EICStat::Defense:Result.Defense+=Value;break;
            case EICStat::MaxHP:Result.MaxHP+=Value;break;case EICStat::MaxMP:Result.MaxMP+=Value;break;case EICStat::MaxSP:Result.MaxSP+=Value;break;
            case EICStat::AttackSpeed:Result.AttackSpeed+=Value;break;case EICStat::MoveSpeed:Result.MoveSpeed+=Value;break;case EICStat::CriticalChance:Result.CriticalRate+=Value;break;
            default:break;
        }
    };
    TMap<EICStat,double> Flat,Percent;FICDefinitionResult R;
    for(const auto& Item:GetSnapshot().Equipment)
    {
        if(Item.IsEmpty())continue;const auto* D=Find(Item.ItemID);
        const auto* Gear=D?Service->Find<FICEquipmentRow>(D->EquipmentID,R):nullptr;if(!Gear)return FPlayerEquipmentStats();
        const auto* Rules=Service->Find<FICEnhancementRow>(Gear->EnhancementID,R);if(!Rules)return FPlayerEquipmentStats();
        double Bonus=0;
        for(int32 Level=1;Level<=Item.Upgrade;++Level)
        {
            const auto* Step=Rules->Steps.FindByPredicate([&](const auto& S){ IC_TRACE_FUNCTION();return S.TargetLevel==Level;});if(!Step)return FPlayerEquipmentStats();
            Bonus+=Step->OriginalBaseBonusRatio;
        }
        for(const auto& M:Gear->BaseModifiers)
        {Flat.FindOrAdd(M.Stat)+=M.Flat*(Rules->EnhancedStats.Contains(M.Stat)?1+Bonus:1);Percent.FindOrAdd(M.Stat)+=M.Percent;}
        for(const auto& O:Item.Options)
        {
            const auto* Rule=Service->Find<FICOptionRow>(O.Key,R);if(!Rule)return FPlayerEquipmentStats();
            (Rule->bPercentage?Percent:Flat).FindOrAdd(Rule->Stat)+=O.Value;
        }
    }
    TSet<EICStat> Stats;
    for(const auto& Entry:Flat)Stats.Add(Entry.Key);
    for(const auto& Entry:Percent)Stats.Add(Entry.Key);
    for(EICStat Stat:Stats)
    {
        FICStatModifier Modifier;Modifier.Stat=Stat;Modifier.Flat=Flat.FindRef(Stat);Modifier.Percent=Percent.FindRef(Stat);
        Result.Modifiers.Add(Modifier);
        const bool RatioStat=Stat==EICStat::AttackSpeed||Stat==EICStat::MoveSpeed||Stat==EICStat::CriticalChance;
        Add(Stat,RatioStat?Modifier.Flat+Modifier.Percent:Modifier.Flat);
    }
    Result.Modifiers.Sort([](const auto& A,const auto& B){ IC_TRACE_FUNCTION();return static_cast<uint8>(A.Stat)<static_cast<uint8>(B.Stat);});
    return Result;
}
