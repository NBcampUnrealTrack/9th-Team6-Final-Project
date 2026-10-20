#include "Inventory/Rewards/ICRewardService.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Debug/ICLog.h"
bool FICRewardService::PrepareMonsterDefeat(FICWorldState& Candidate,FName Key,const FICMonsterRow& Monster,UICDefinitionSubsystem* D,double Now,FString& Reason)
{
    IC_TRACE_FUNCTION();Reason.Empty();auto* Slot=Candidate.SpawnSlots.Find(Key);if(!D||!Slot||!Slot->IncarnationID.IsValid()){Reason=TEXT("처치 보상의 실행 키가 없습니다.");return false;}
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
        if(Drop->Rolls<1||Drop->MinimumGold<0||Drop->MaximumGold<Drop->MinimumGold){Reason=TEXT("드롭 횟수·골드 범위가 올바르지 않습니다.");return false;}
        for(const auto& Entry:Drop->Entries)if(!D->Find<FICItemRow>(Entry.ItemID,R)||Entry.MinimumCount<1||Entry.MaximumCount<Entry.MinimumCount||!FMath::IsFinite(Entry.Probability)||Entry.Probability<0||Entry.Probability>1||!FMath::IsFinite(Entry.Weight)||Entry.Weight<0){Reason=TEXT("드롭 아이템 참조·수량·확률이 올바르지 않습니다.");return false;}
        for(int32 Roll=0;Roll<Drop->Rolls;++Roll)
        {
            if(Drop->Selection==EICDropSelection::IndependentProbability){for(const auto& Entry:Drop->Entries)if(Random.FRand()<FMath::Clamp(Entry.Probability*(Entry.bApplyPlayerLUKToProbability?Luck:1),0.,1.))if(!BuildGroundItem(Entry.ItemID,Random.RandRange(Entry.MinimumCount,Entry.MaximumCount),&Entry)){Reason=TEXT("드롭 인스턴스를 만들 수 없습니다.");return false;}}
            else{double Total=0;for(const auto& Entry:Drop->Entries)Total+=Entry.Weight*(Entry.bApplyPlayerLUKToProbability?Luck:1);double Choice=Random.FRand()*Total;for(const auto& Entry:Drop->Entries){Choice-=Entry.Weight*(Entry.bApplyPlayerLUKToProbability?Luck:1);if(Choice<=0){if(!BuildGroundItem(Entry.ItemID,Random.RandRange(Entry.MinimumCount,Entry.MaximumCount),&Entry)){Reason=TEXT("선택 드롭을 만들 수 없습니다.");return false;}break;}}}
        }
        Gold+=FMath::RoundToInt64(Random.RandRange(Drop->MinimumGold,Drop->MaximumGold)*(Drop->bApplyPlayerLUKToGold?Luck:1));
    }
    if(Reward)
    {
        Gold+=Reward->Gold;if(Reward->Gold<0){Reason=TEXT("보상 골드가 음수입니다.");return false;}
        auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,Next.Job);if(!Staging->ApplySnapshot(Next.Inventory)){Reason=TEXT("보상 전 인벤토리가 올바르지 않습니다.");return false;}
        for(const auto& Item:Reward->Items)if(!Staging->AddItem(Item.ItemID,Item.Count,false))
        {if(Reward->Overflow!=EICRewardOverflow::DropExcess||!BuildGroundItem(Item.ItemID,Item.Count,nullptr)){Reason=TEXT("보상 아이템 참조 또는 가방 공간이 부족합니다.");return false;}}
        Next.Inventory=Staging->GetSnapshot();for(const auto& Affinity:Reward->Affinity){auto* Character=Next.Characters.Find(Affinity.NPCID);if(!Character||Character->Kind==EICCharacterKind::Background){Reason=TEXT("보상 관계 대상이 없습니다.");return false;}Character->Affinity=FMath::Clamp(Character->Affinity+Affinity.Amount,0,500);}
        for(FName ID:Reward->UnlockRecipeIDs){if(!D->Find<FICRecipeRow>(ID,R)){Reason=R.Reason;return false;}Next.UnlockedRecipeIDs.Add(ID);}
    }
    if(Gold>MAX_int32||Gold<0){Reason=TEXT("보상 후 골드 범위를 초과합니다.");return false;}
    Next.Gold=Gold;if(!Ground.Contents.IsEmpty())Next.Drops.Add(Ground);Next.Flags.Add(FirstFlag,true);Next.CompletedTransactionIDs.Add(Slot->IncarnationID);auto& Updated=Next.SpawnSlots.FindChecked(Key);Updated.bRewardPending=false;
    if(!UICWorldStateSubsystem::ValidateState(Next)){Reason=TEXT("처치 보상 후보 상태가 올바르지 않습니다.");return false;}Candidate=MoveTemp(Next);return true;
}
