#include "Inventory/Rewards/ICRewardService.h"
#include "UI/Framework/ICText.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Debug/ICLog.h"
bool FICRewardService::PrepareLifeLoot(FICWorldState& Candidate,FName DropTableID,const FGuid& Receipt,UICDefinitionSubsystem* D,const FVector& Position,double Now,TArray<FICItemAmount>& Received,FString& Reason)
{
    Received.Reset();Reason.Empty();if(!D||!Receipt.IsValid())return false;
    if(Candidate.CompletedTransactionIDs.Contains(Receipt))return true;
    FICDefinitionResult R;const auto* Drop=D->Find<FICDropTableRow>(DropTableID,R);
    if(!Drop||Drop->Rolls<1||Drop->MinimumGold<0||Drop->MaximumGold<Drop->MinimumGold){Reason=R.Reason;return false;}
    FICWorldState Next=Candidate;FRandomStream Random(GetTypeHash(Receipt));
    const auto* Player=Next.Characters.Find(UICWorldStateSubsystem::PlayerId());
    FICStatSettingsRow Defaults;const auto* Stats=D->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),R);if(!Stats)Stats=&Defaults;
    const double LUK=Player?Player->MainStats.LUK:1;
    const double Luck=1+Stats->LUKRewardBonus*(LUK-1)/(LUK-1+Stats->LUKCurveK);
    for(const auto& Entry:Drop->Entries)
        if(!D->Find<FICItemRow>(Entry.ItemID,R)||Entry.MinimumCount<1||Entry.MaximumCount<Entry.MinimumCount||!FMath::IsFinite(Entry.Probability)||Entry.Probability<0||Entry.Probability>1||!FMath::IsFinite(Entry.Weight)||Entry.Weight<0||Entry.bEquipmentGradeRoll)return false;
    auto* Bag=NewObject<UICInventoryComponent>();Bag->BeginStaging(D,Next.Job);if(!Bag->ApplySnapshot(Next.Inventory))return false;
    FICDropState Ground;Ground.ID=FGuid::NewGuid();Ground.Position=Position;Ground.CreatedPlaySecond=Now;
    auto Grant=[&](const FICDropEntry& Entry)
    {
        int32 Count=Random.RandRange(Entry.MinimumCount,Entry.MaximumCount);
        const auto* Item=D->Find<FICItemRow>(Entry.ItemID,R);
        while(Count>0)
        {
            auto* Builder=NewObject<UICInventoryComponent>();Builder->BeginStaging(D,Next.Job);
            const int32 Chunk=FMath::Min(Count,FMath::Max(1,Item->StackLimit));
            if(!Builder->AddItem(Entry.ItemID,Chunk,false))return false;
            auto Instances=Builder->GetSnapshot().Bag.Slots;Instances.Append(Builder->GetSnapshot().QuestItems);
            for(auto Instance:Instances)if(!Instance.IsEmpty())
            {
                const int32 Remainder=Drop->bAutoCollectItems?Bag->TryAddItemInstance(Instance):Instance.Count;
                const int32 Added=Instance.Count-Remainder;
                if(Added>0){auto* Existing=Received.FindByPredicate([&](const auto& Value){return Value.ItemID==Entry.ItemID;});if(Existing)Existing->Count+=Added;else{auto& Value=Received.AddDefaulted_GetRef();Value.ItemID=Entry.ItemID;Value.Count=Added;}}
                if(Remainder>0){if(Added>0)Instance.InstanceID=FGuid::NewGuid();Instance.Count=Remainder;Ground.Contents.Add(Instance);}
            }
            Count-=Chunk;
        }
        return true;
    };
    for(int32 Roll=0;Roll<Drop->Rolls;++Roll)
    {
        if(Drop->Selection==EICDropSelection::IndependentProbability)
        {for(const auto& Entry:Drop->Entries)if(Random.FRand()<FMath::Clamp(Entry.Probability*(Entry.bApplyPlayerLUKToProbability?Luck:1),0.,1.))if(!Grant(Entry))return false;}
        else
        {
            double Total=0;for(const auto& Entry:Drop->Entries)Total+=Entry.Weight*(Entry.bApplyPlayerLUKToProbability?Luck:1);
            if(Total<=0)return false;double Choice=Random.FRand()*Total;
            for(const auto& Entry:Drop->Entries){Choice-=Entry.Weight*(Entry.bApplyPlayerLUKToProbability?Luck:1);if(Choice<=0){if(!Grant(Entry))return false;break;}}
        }
    }
    const int64 Gold=static_cast<int64>(Next.Gold)+FMath::RoundToInt64(Random.RandRange(Drop->MinimumGold,Drop->MaximumGold)*(Drop->bApplyPlayerLUKToGold?Luck:1));
    if(Gold>MAX_int32||Gold<0)return false;
    Next.Gold=Gold;Next.Inventory=Bag->GetSnapshot();if(!Ground.Contents.IsEmpty())Next.Drops.Add(Ground);
    Next.CompletedTransactionIDs.Add(Receipt);if(!UICWorldStateSubsystem::ValidateState(Next))return false;
    Candidate=MoveTemp(Next);return true;
}
bool FICRewardService::PrepareMonsterDefeat(FICWorldState& Candidate,FName Key,const FICMonsterRow& Monster,UICDefinitionSubsystem* D,double Now,FString& Reason)
{
    IC_TRACE_FUNCTION();Reason.Empty();auto* Slot=Candidate.SpawnSlots.Find(Key);if(!D||!Slot||!Slot->IncarnationID.IsValid()){Reason=ICText::Get(TEXT("Native_8fdc440ab009")).ToString();return false;}
    if(Candidate.CompletedTransactionIDs.Contains(Slot->IncarnationID))return true;
    FICWorldState Next=Candidate;FRandomStream Random(GetTypeHash(Slot->IncarnationID));FICDefinitionResult R;const auto* Drop=Monster.DropTableID.IsNone()?nullptr:D->Find<FICDropTableRow>(Monster.DropTableID,R);
    if(!Monster.DropTableID.IsNone()&&!Drop){Reason=R.Reason;return false;}
    const FName FirstFlag=FName(*(TEXT("KILL_FIRST_")+Monster.ID.ToString()));const FName RewardID=Next.Flags.FindRef(FirstFlag)?Monster.RepeatKillRewardID:Monster.FirstKillRewardID;const auto* Reward=RewardID.IsNone()?nullptr:D->Find<FICRewardRow>(RewardID,R);
    if(!RewardID.IsNone()&&!Reward){Reason=R.Reason;return false;}
    const auto* Player=Next.Characters.Find(UICWorldStateSubsystem::PlayerId());FICStatSettingsRow Defaults;const auto* Stats=D->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),R);if(!Stats)Stats=&Defaults;const double LUK=Player?Player->MainStats.LUK:1;const double Luck=1+Stats->LUKRewardBonus*(LUK-1)/(LUK-1+Stats->LUKCurveK);
    FICDropState Ground;Ground.ID=FGuid::NewGuid();Ground.Position=Slot->Position;Ground.CreatedPlaySecond=Now;
    auto BuildGroundItem=[&](FName ID,int32 Count,const FICDropEntry* Entry)
    {
        IC_TRACE_FUNCTION();const auto* Item=D->Find<FICItemRow>(ID,R);if(!Item||Count<1)return false;
        while(Count>0){auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,Next.Job);const int32 Chunk=FMath::Min(Count,FMath::Max(1,Item->StackLimit));if(!Staging->AddItem(ID,Chunk,false))return false;
            auto Instances=Staging->GetSnapshot().Bag.Slots;Instances.Append(Staging->GetSnapshot().QuestItems);
            for(auto Instance:Instances)if(!Instance.IsEmpty())
            {if(Entry&&Entry->bEquipmentGradeRoll&&Drop&&!Drop->EquipmentGrades.IsEmpty())
                {double Total=0;for(const auto& G:Drop->EquipmentGrades)Total+=G.Weight*(G.bApplyPlayerLUK?Luck:1);double Roll=Random.FRand()*Total;for(const auto& G:Drop->EquipmentGrades){Roll-=G.Weight*(G.bApplyPlayerLUK?Luck:1);if(Roll<=0){Instance.Grade=static_cast<int32>(G.Grade);break;}}Instance.Options.Reset();if(!Staging->RollRandomOptions(Instance,*Item))return false;}
                Ground.Contents.Add(Instance);}Count-=Chunk;}return true;
    };
    int64 Gold=Next.Gold;
    if(Drop)
    {
        if(Drop->Rolls<1||Drop->MinimumGold<0||Drop->MaximumGold<Drop->MinimumGold){Reason=ICText::Get(TEXT("Native_6c036b297ff2")).ToString();return false;}
        for(const auto& Entry:Drop->Entries)if(!D->Find<FICItemRow>(Entry.ItemID,R)||Entry.MinimumCount<1||Entry.MaximumCount<Entry.MinimumCount||!FMath::IsFinite(Entry.Probability)||Entry.Probability<0||Entry.Probability>1||!FMath::IsFinite(Entry.Weight)||Entry.Weight<0){Reason=ICText::Get(TEXT("Native_c28eef04b65b")).ToString();return false;}
        for(int32 Roll=0;Roll<Drop->Rolls;++Roll)
        {
            if(Drop->Selection==EICDropSelection::IndependentProbability){for(const auto& Entry:Drop->Entries)if(Random.FRand()<FMath::Clamp(Entry.Probability*(Entry.bApplyPlayerLUKToProbability?Luck:1),0.,1.))if(!BuildGroundItem(Entry.ItemID,Random.RandRange(Entry.MinimumCount,Entry.MaximumCount),&Entry)){Reason=ICText::Get(TEXT("Native_141355b744dc")).ToString();return false;}}
            else{double Total=0;for(const auto& Entry:Drop->Entries)Total+=Entry.Weight*(Entry.bApplyPlayerLUKToProbability?Luck:1);double Choice=Random.FRand()*Total;for(const auto& Entry:Drop->Entries){Choice-=Entry.Weight*(Entry.bApplyPlayerLUKToProbability?Luck:1);if(Choice<=0){if(!BuildGroundItem(Entry.ItemID,Random.RandRange(Entry.MinimumCount,Entry.MaximumCount),&Entry)){Reason=ICText::Get(TEXT("Native_24b0dd0a7bdd")).ToString();return false;}break;}}}
        }
        Gold+=FMath::RoundToInt64(Random.RandRange(Drop->MinimumGold,Drop->MaximumGold)*(Drop->bApplyPlayerLUKToGold?Luck:1));
    }
    if(Reward)
    {
        Gold+=Reward->Gold;if(Reward->Gold<0){Reason=ICText::Get(TEXT("Native_a26a05cc9957")).ToString();return false;}
        auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,Next.Job);if(!Staging->ApplySnapshot(Next.Inventory)){Reason=ICText::Get(TEXT("Native_2623b82098a2")).ToString();return false;}
        for(const auto& Item:Reward->Items)if(!Staging->AddItem(Item.ItemID,Item.Count,false))
        {if(Reward->Overflow!=EICRewardOverflow::DropExcess||!BuildGroundItem(Item.ItemID,Item.Count,nullptr)){Reason=ICText::Get(TEXT("Native_48442fc5a9b1")).ToString();return false;}}
        Next.Inventory=Staging->GetSnapshot();for(const auto& Affinity:Reward->Affinity){auto* Character=Next.Characters.Find(Affinity.NPCID);if(!Character||Character->Kind==EICCharacterKind::Background){Reason=ICText::Get(TEXT("Native_e7fa81f84aab")).ToString();return false;}Character->Affinity=FMath::Clamp(Character->Affinity+Affinity.Amount,0,500);}
        for(FName ID:Reward->UnlockRecipeIDs){if(!D->Find<FICRecipeRow>(ID,R)){Reason=R.Reason;return false;}Next.UnlockedRecipeIDs.Add(ID);}
    }
    if(Gold>MAX_int32||Gold<0){Reason=ICText::Get(TEXT("Native_86bddc6005a7")).ToString();return false;}
    if(Drop&&Drop->bAutoCollectItems&&!Ground.Contents.IsEmpty())
    {
        auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,Next.Job);
        if(!Staging->ApplySnapshot(Next.Inventory)){Reason=TEXT("Cannot stage monster loot inventory");return false;}
        TArray<FPlayerItemInstance> Overflow;
        for(auto Item:Ground.Contents)
        {
            const int32 Remainder=Staging->TryAddItemInstance(Item);
            if(Remainder>0)
            {
                if(Remainder<Item.Count)Item.InstanceID=FGuid::NewGuid();
                Item.Count=Remainder;Overflow.Add(Item);
            }
        }
        Next.Inventory=Staging->GetSnapshot();Ground.Contents=MoveTemp(Overflow);
    }
    if(Monster.PetCodexIndex>=0)
    {
        int32& Essence=Next.Codex.PetEssence.FindOrAdd(Monster.PetCodexIndex);
        Essence=static_cast<int32>(FMath::Min<int64>(MAX_int32,static_cast<int64>(Essence)+Random.RandRange(1,10)));
    }
    Next.Gold=Gold;if(!Ground.Contents.IsEmpty())Next.Drops.Add(Ground);Next.Flags.Add(FirstFlag,true);Next.CompletedTransactionIDs.Add(Slot->IncarnationID);auto& Updated=Next.SpawnSlots.FindChecked(Key);Updated.bRewardPending=false;
    if(!UICWorldStateSubsystem::ValidateState(Next)){Reason=ICText::Get(TEXT("Native_8ea2e2c4ddba")).ToString();return false;}Candidate=MoveTemp(Next);return true;
}
