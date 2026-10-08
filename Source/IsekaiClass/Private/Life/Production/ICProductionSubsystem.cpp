#include "Life/Production/ICProductionSubsystem.h"
#include "Debug/ICLog.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/World/ICAnchorRow.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Subsystems/SubsystemCollection.h"
#include "Quest/ICQuestSubsystem.h"

void UICProductionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    IC_TRACE_FUNCTION();
    Super::Initialize(Collection);
    ConfigureServices(Collection.InitializeDependency<UICDefinitionSubsystem>(),Collection.InitializeDependency<UICWorldStateSubsystem>(),Collection.InitializeDependency<UICTimeSubsystem>());
}
void UICProductionSubsystem::ConfigureServices(UICDefinitionSubsystem* InDefinitions,UICWorldStateSubsystem* InState,UICTimeSubsystem* InClock)
{
    IC_TRACE_FUNCTION();
    if(Clock)Clock->OnPlaySecondsAdvanced.RemoveDynamic(this,&ThisClass::PlaySecondsAdvanced);
    Definitions=InDefinitions;State=InState;Clock=InClock;
    if(Clock)Clock->OnPlaySecondsAdvanced.AddUniqueDynamic(this,&ThisClass::PlaySecondsAdvanced);
    UE_LOG(LogICProduction, Verbose, TEXT("[Production.Configure] Definitions=%s State=%s Clock=%s"),*GetNameSafe(Definitions),*GetNameSafe(State),*GetNameSafe(Clock));
}
void UICProductionSubsystem::Deinitialize()
{ IC_TRACE_FUNCTION(); ConfigureServices(nullptr,nullptr,nullptr);RuntimeInventory.Reset();Super::Deinitialize(); }

bool UICProductionSubsystem::StartRecipe(FName RecipeID,FName WorkAnchorID,UICInventoryComponent* Inventory,FGuid& JobID,FString& Reason)
{
    IC_TRACE_FUNCTION();
    JobID.Invalidate();Reason.Reset();
    auto Fail=[&](const TCHAR* Text){ IC_TRACE_FUNCTION();Reason=Text;UE_LOG(LogICProduction, Verbose, TEXT("[Production.Start.Rejected] Recipe=%s Anchor=%s Reason=%s"),*RecipeID.ToString(),*WorkAnchorID.ToString(),Text);return false;};FICDefinitionResult R;
    if(!Definitions||!State||!Clock||!Inventory)return Fail(TEXT("생산 시스템이 준비되지 않았습니다."));
    const auto* Recipe=Definitions->Find<FICRecipeRow>(RecipeID,R);
    const auto* Production=Recipe?Definitions->Find<FICProductionRow>(Recipe->ProductionID,R):nullptr;
    const auto* Anchor=Definitions->Find<FICAnchorRow>(WorkAnchorID,R);
    if(!Recipe||!Production||!Anchor)return Fail(TEXT("레시피·생산·작업 앵커 정의가 누락되었습니다."));
    if(Recipe->ResultKind!=EICProductionResultKind::BagItems || Production->ResultKind!=EICProductionResultKind::BagItems)
        return Fail(TEXT("이 작업은 설치·채집 등 전용 생산 경로가 필요합니다."));
    if(!Recipe->UnlockConditions.IsEmpty()||!Production->StartConditions.IsEmpty())
        return Fail(TEXT("생산 조건 판정기가 연결되지 않았습니다. 변경하지 않았습니다."));
    if(!Production->WorkAnchorIDs.IsEmpty()&&!Production->WorkAnchorIDs.Contains(WorkAnchorID))return Fail(TEXT("이 작업대에서 수행할 수 없는 생산입니다."));
    if(Production->DurationPlaySeconds!=Recipe->WorkDurationPlaySeconds || Production->SPCost!=Recipe->SPCost)
        return Fail(TEXT("레시피와 생산 정의의 시간·비용이 일치하지 않습니다."));
    if(!FMath::IsFinite(Recipe->WorkDurationPlaySeconds)||Recipe->WorkDurationPlaySeconds<0||!FMath::IsFinite(Recipe->SPCost)||Recipe->SPCost<0)
        return Fail(TEXT("제작 시간·SP 비용 설정이 올바르지 않습니다."));
    if(!State->FlushParticipants())return Fail(TEXT("현재 상태를 반영할 수 없습니다."));
    FICWorldState Next=State->ExportState();
    if(!Next.bPlayerCreated)return Fail(TEXT("플레이어가 생성되지 않았습니다."));
    if(!Recipe->bUnlockedByDefault&&!Next.UnlockedRecipeIDs.Contains(RecipeID))return Fail(TEXT("아직 해금되지 않은 레시피입니다."));
    for(const auto& Pair:Next.ProductionJobs)
        if(Pair.Value.OwnerCharacterID==UICWorldStateSubsystem::PlayerId()&&(Pair.Value.Status==EICProductionJobStatus::Running||Pair.Value.Status==EICProductionJobStatus::Ready))
            return Fail(TEXT("진행 중인 작업을 완료하거나 취소하세요."));
    auto* Player=Next.Characters.Find(UICWorldStateSubsystem::PlayerId());
    if(!Player||Player->HP<=0||Player->IsDowned()||Player->SP<Recipe->SPCost)return Fail(TEXT("제작할 수 없는 상태이거나 SP가 부족합니다."));
    for(const auto& Tool:Recipe->Tools)if(!Definitions->Find<FICItemRow>(Tool.ItemID,R)||Tool.Count<=0||Inventory->CountItem(Tool.ItemID)<Tool.Count)
        return Fail(TEXT("필수 도구가 없거나 도구 정의가 누락되었습니다."));
    for(const auto& Tool:Production->Tools)if(!Definitions->Find<FICItemRow>(Tool.ItemID,R)||Tool.Count<=0||Inventory->CountItem(Tool.ItemID)<Tool.Count)
        return Fail(TEXT("필수 생산 도구가 없습니다."));
    // Validate every result and enough bag space before reserving costs.
    if(Recipe->Ingredients.IsEmpty()||Recipe->Results.IsEmpty())return Fail(TEXT("재료·결과 정의가 비어 있습니다."));
    TMap<FName,int64> Costs;
    for(const auto& Cost:Recipe->Ingredients)
    {
        if(Cost.Count<=0||!Definitions->Find<FICItemRow>(Cost.ItemID,R))return Fail(TEXT("재료 정의 또는 수량이 올바르지 않습니다."));
        Costs.FindOrAdd(Cost.ItemID)+=Cost.Count;if(Costs[Cost.ItemID]>MAX_int32)return Fail(TEXT("재료 수량이 범위를 초과했습니다."));
    }
    FICInventoryState Preview=Next.Inventory;
    for(const auto& Cost:Costs)if(!Inventory->Remove(Preview.Bag,Cost.Key,Cost.Value))return Fail(TEXT("재료가 부족하거나 잠겨 있습니다."));
    for(const auto& Result:Recipe->Results)
    {
        const auto* Item=Definitions->Find<FICItemRow>(Result.ItemID,R);
        if(!Item||Item->Kind==EPlayerItemKind::Quest||Result.Count<=0||Inventory->Insert(Preview.Bag,Result.ItemID,Result.Count)!=Result.Count)
            return Fail(TEXT("결과 정의가 누락되었거나 결과물을 보관할 공간이 없습니다."));
    }
    FICProductionJobState Job;Job.JobID=FGuid::NewGuid();Job.OwnerCharacterID=Player->CharacterId;
    Job.RecipeID=RecipeID;Job.ProductionID=Recipe->ProductionID;Job.WorkAnchorID=WorkAnchorID;
    Job.StartedAtPlaySecond=Clock->GetClock().PlaySecond;Job.CompletesAtPlaySecond=Job.StartedAtPlaySecond+Recipe->WorkDurationPlaySeconds;
    // SP is the action's start cost; materials stay reserved until successful completion.
    Job.ReservedSP=Recipe->SPCost;Job.bCostsCommitted=true;Player->SP-=Recipe->SPCost;
    for(const auto& Cost:Costs)
    {
        int32 Remaining=Cost.Value;
        for(const auto& Item:Next.Inventory.Bag.Slots)if(Item.ItemID==Cost.Key&&!Item.bLocked&&Remaining>0)
        {
            FICMaterialReservationState Reservation;Reservation.ItemInstanceID=Item.InstanceID;Reservation.ItemID=Item.ItemID;
            Reservation.Count=FMath::Min(Remaining,Item.Count);Job.Materials.Add(Reservation);Remaining-=Reservation.Count;
        }
        if(Remaining)return Fail(TEXT("재료 예약 재검사에 실패했습니다."));
    }
    Next.ProductionJobs.Add(Job.JobID,Job);
    if(!Inventory->ValidateSnapshot(Next.Inventory)||!State->CommitGameplayTransaction(Next,Job.JobID))return Fail(TEXT("생산 시작 검증에 실패했습니다. 변경하지 않았습니다."));
    RuntimeInventory=Inventory;JobID=Job.JobID;UE_LOG(LogICProduction, Log, TEXT("[Production.Started] Job=%s Recipe=%s Anchor=%s CompleteAt=%.2f SPCost=%.2f"),*JobID.ToString(),*RecipeID.ToString(),*WorkAnchorID.ToString(),Job.CompletesAtPlaySecond,Job.ReservedSP);OnProductionChanged.Broadcast(JobID,TEXT("제작 시작"));return true;
}

bool UICProductionSubsystem::CompleteJob(FGuid JobID,UICInventoryComponent* Inventory,FString& Reason)
{
    IC_TRACE_FUNCTION();
    Reason.Reset();auto Fail=[&](const TCHAR* Text){ IC_TRACE_FUNCTION();Reason=Text;UE_LOG(LogICProduction, Verbose, TEXT("[Production.Complete.Rejected] Job=%s Reason=%s"),*JobID.ToString(),Text);return false;};FICDefinitionResult R;
    if(!State||!Clock||!Definitions||!Inventory)return Fail(TEXT("생산 시스템이 준비되지 않았습니다."));
    if(!State->FlushParticipants())return Fail(TEXT("현재 상태를 반영할 수 없습니다."));
    FICWorldState Next=State->ExportState();auto* Job=Next.ProductionJobs.Find(JobID);
    if(!Job || Job->Status==EICProductionJobStatus::Cancelled || Job->Status==EICProductionJobStatus::Completed)return Fail(TEXT("이미 완료·취소된 작업이거나 존재하지 않습니다."));
    if(Clock->GetClock().PlaySecond<Job->CompletesAtPlaySecond)return Fail(TEXT("아직 제작 시간이 남아 있습니다."));
    const auto* Recipe=Definitions->Find<FICRecipeRow>(Job->RecipeID,R);
    const auto* Production=Definitions->Find<FICProductionRow>(Job->ProductionID,R);
    if(!Recipe||!Production||Recipe->ProductionID!=Job->ProductionID||Recipe->ResultKind!=EICProductionResultKind::BagItems||Recipe->Results.IsEmpty())
        return Fail(TEXT("완료에 필요한 정의가 누락되거나 변경되었습니다."));
    if(Job->OwnerCharacterID!=UICWorldStateSubsystem::PlayerId())return Fail(TEXT("이 작업은 NPC 생산 경로에서 처리해야 합니다."));
    if(!Job->bCostsCommitted)
    {
        auto* Owner=Next.Characters.Find(Job->OwnerCharacterID);
        if(!Owner||Owner->SP<Job->ReservedSP||Next.Gold<Job->ReservedGold)return Fail(TEXT("예약 비용을 확정할 수 없습니다."));
        Owner->SP-=Job->ReservedSP;Next.Gold-=Job->ReservedGold;
    }
    // Consume the exact reserved instances, then insert all outputs into the same candidate.
    for(const auto& Reserved:Job->Materials)
    {
        if(Reserved.Timing==EICProductionMaterialTiming::ConsumeAtStart&&Job->bCostsCommitted)continue;
        auto* Item=Next.Inventory.Bag.Slots.FindByPredicate([&](const auto& I){ IC_TRACE_FUNCTION();return I.InstanceID==Reserved.ItemInstanceID;});
        if(!Item||Item->ItemID!=Reserved.ItemID||Item->bLocked||Item->Count<Reserved.Count||!Inventory->Find(Item->ItemID))return Fail(TEXT("예약 재료를 확인할 수 없습니다."));
        Item->Count-=Reserved.Count;if(Item->Count==0)*Item=FPlayerItemInstance();
    }
    for(const auto& Result:Recipe->Results)
        if(Result.Count<=0||Inventory->Insert(Next.Inventory.Bag,Result.ItemID,Result.Count)!=Result.Count)
            return Fail(TEXT("결과 정의가 누락되었거나 가방 공간이 부족합니다. 변경하지 않았습니다."));
    Job->Status=EICProductionJobStatus::Completed;Job->bResultsCommitted=Job->bCostsCommitted=true;Job->CompletionTransactionID=FGuid::NewGuid();
    const FGuid Transaction=Job->CompletionTransactionID;
    if(!Inventory->ValidateSnapshot(Next.Inventory)||!State->CommitGameplayTransaction(Next,Transaction))return Fail(TEXT("생산 완료 거래 검증에 실패했습니다."));
    UE_LOG(LogICProduction, Log, TEXT("[Production.Completed] Job=%s Transaction=%s"),*JobID.ToString(),*Transaction.ToString());
    if (UICQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICQuestSubsystem>() : nullptr)
    {
        Quests->ReportObjectiveEvent(EICQuestObjectiveType::Craft, Job->RecipeID, 1, JobID,
            Production->Activity, Recipe->Results[0].ItemID);
    }
    OnProductionChanged.Broadcast(JobID,TEXT("제작 완료"));return true;
}

bool UICProductionSubsystem::CancelJob(FGuid JobID)
{
    IC_TRACE_FUNCTION();
    if(!State)return false;FICWorldState Next=State->ExportState();auto* Job=Next.ProductionJobs.Find(JobID);
    if(!Job||Job->Status==EICProductionJobStatus::Completed||Job->Status==EICProductionJobStatus::Cancelled)return false;
    Job->Status=EICProductionJobStatus::Cancelled;
    if(!State->CommitGameplayTransaction(Next,FGuid::NewGuid()))return false;
    UE_LOG(LogICProduction, Log, TEXT("[Production.Cancelled] Job=%s: 예약 재료 해제"),*JobID.ToString());OnProductionChanged.Broadcast(JobID,TEXT("작업 취소. 예약 재료 해제"));return true;
}

void UICProductionSubsystem::PlaySecondsAdvanced(double Seconds)
{
    IC_TRACE_FUNCTION();
    if(!State||!Clock)return;
    UICInventoryComponent* Inventory=RuntimeInventory.Get();
    if(!Inventory&&GetWorld())if(auto* PC=Cast<AICPlayerController>(GetWorld()->GetFirstPlayerController()))Inventory=PC->GetInventory();
    if(!Inventory)return;
    const FICWorldState Snapshot=State->ExportState();
    for(const auto& Pair:Snapshot.ProductionJobs)
    {
        if(Pair.Value.OwnerCharacterID!=UICWorldStateSubsystem::PlayerId())continue;
        if(Pair.Value.Status!=EICProductionJobStatus::Running||Clock->GetClock().PlaySecond<Pair.Value.CompletesAtPlaySecond)continue;
        FString Reason;
        if(!CompleteJob(Pair.Key,Inventory,Reason))
        {
            FICWorldState Next=State->ExportState();auto* Job=Next.ProductionJobs.Find(Pair.Key);
            if(Job&&Job->Status==EICProductionJobStatus::Running)
            {Job->Status=EICProductionJobStatus::Ready;if(State->CommitGameplayTransaction(Next,FGuid::NewGuid())){UE_LOG(LogICProduction, Warning, TEXT("[Production.Ready] Job=%s 완료 보류: %s"),*Pair.Key.ToString(),*Reason);OnProductionChanged.Broadcast(Pair.Key,Reason);}}
        }
    }
}
