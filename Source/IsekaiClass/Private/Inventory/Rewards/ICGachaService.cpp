#include "Inventory/Rewards/ICGachaService.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

bool FICGachaService::PrepareDraw(FICWorldState& Candidate,const FICGachaMachineRow& Rule,UICDefinitionSubsystem* D,FRandomStream& Random,const FVector& Position,double Now,FICGachaDrawResult& Result)
{
    Result={};
    auto Invalid=[&](){Result.Failure=NSLOCTEXT("ICGacha","InvalidDefinition","뽑기 설정을 확인할 수 없습니다.");return false;};
    if(!D||!Candidate.bPlayerCreated||Rule.CostGold<0||Rule.Pools.IsEmpty()||!FMath::IsFinite(Now)||Now<0||Position.ContainsNaN())return Invalid();
    if(Candidate.Gold<Rule.CostGold){Result.Failure=NSLOCTEXT("ICGacha","NotEnoughGold","골드가 부족합니다.");return false;}
    double PoolTotal=0;FICDefinitionResult Lookup;
    for(const auto& Pool:Rule.Pools)
    {
        if(!FMath::IsFinite(Pool.Weight)||Pool.Weight<0)return Invalid();
        if(Pool.Weight==0)continue;
        double ItemTotal=0;
        for(const auto& Entry:Pool.Items)
        {
            if(!FMath::IsFinite(Entry.Weight)||Entry.Weight<0)return Invalid();
            if(Entry.Weight==0)continue;
            const auto* Item=D->Find<FICItemRow>(Entry.ItemID,Lookup);
            if(!Item||Item->Kind==EPlayerItemKind::Quest||Entry.MinimumCount<1||Entry.MaximumCount<Entry.MinimumCount||Entry.MaximumCount>100000)return Invalid();
            ItemTotal+=Entry.Weight;
        }
        if(!FMath::IsFinite(ItemTotal)||ItemTotal<=0)return Invalid();
        PoolTotal+=Pool.Weight;
    }
    if(!FMath::IsFinite(PoolTotal)||PoolTotal<=0)return Invalid();
    auto Pick=[&](const auto& Entries)
    {
        double Total=0;for(const auto& Entry:Entries)Total+=Entry.Weight;
        double Choice=Random.FRand()*Total;int32 Last=INDEX_NONE;
        for(int32 I=0;I<Entries.Num();++I)if(Entries[I].Weight>0)
        {Last=I;Choice-=Entries[I].Weight;if(Choice<0)return I;}
        return Last;
    };
    const auto& Pool=Rule.Pools[Pick(Rule.Pools)];const auto& Entry=Pool.Items[Pick(Pool.Items)];
    const int32 Count=Random.RandRange(Entry.MinimumCount,Entry.MaximumCount);
    const auto* Item=D->Find<FICItemRow>(Entry.ItemID,Lookup);if(!Item)return Invalid();
    FICWorldState Next=Candidate;auto* Bag=NewObject<UICInventoryComponent>();Bag->BeginStaging(D,Next.Job);
    if(!Bag->ApplySnapshot(Next.Inventory))return Invalid();
    FICDropState Ground;Ground.ID=FGuid::NewGuid();Ground.Position=Position;Ground.CreatedPlaySecond=Now;
    int32 Remaining=Count;
    while(Remaining>0)
    {
        const int32 Chunk=FMath::Min(Remaining,Item->Kind==EPlayerItemKind::Equipment?1:FMath::Max(1,Item->StackLimit));
        auto* Builder=NewObject<UICInventoryComponent>();Builder->BeginStaging(D,Next.Job);
        if(!Builder->AddItem(Entry.ItemID,Chunk,false))return Invalid();
        for(auto Instance:Builder->GetSnapshot().Bag.Slots)if(!Instance.IsEmpty())
        {
            const int32 Left=Bag->TryAddItemInstance(Instance);Result.ReceivedCount+=Instance.Count-Left;
            if(Left>0){if(Left<Instance.Count)Instance.InstanceID=FGuid::NewGuid();Instance.Count=Left;Ground.Contents.Add(Instance);Result.DroppedCount+=Left;}
        }
        Remaining-=Chunk;
    }
    Next.Gold-=Rule.CostGold;Next.Inventory=Bag->GetSnapshot();if(!Ground.Contents.IsEmpty())Next.Drops.Add(Ground);
    if(!UICWorldStateSubsystem::ValidateState(Next))return Invalid();
    Result.Prize.ItemID=Entry.ItemID;Result.Prize.Count=Count;Candidate=MoveTemp(Next);return true;
}
