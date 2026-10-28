#include "Life/Production/ICProductionSubsystem.h"
#include "UI/Framework/ICText.h"
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
    if(!Definitions||!State||!Clock||!Inventory)return Fail(*ICText::Get(TEXT("Native_1e038cacd7e3")).ToString());
    const auto* Recipe=Definitions->Find<FICRecipeRow>(RecipeID,R);
    const auto* Production=Recipe?Definitions->Find<FICProductionRow>(Recipe->ProductionID,R):nullptr;
    const auto* Anchor=Definitions->Find<FICAnchorRow>(WorkAnchorID,R);
    if(!Recipe||!Production||!Anchor)return Fail(*ICText::Get(TEXT("Native_da3541bc00b0")).ToString());
    const bool Storage=Recipe->ResultKind==EICProductionResultKind::InstallStorage;
    if((!Storage&&Recipe->ResultKind!=EICProductionResultKind::BagItems)||Production->ResultKind!=Recipe->ResultKind)
        return Fail(*ICText::Get(TEXT("Native_ed13b90bdcba")).ToString());
    if(!Recipe->UnlockConditions.IsEmpty()||!Production->StartConditions.IsEmpty())
        return Fail(*ICText::Get(TEXT("Native_adde31bc12cb")).ToString());
    if(!Production->WorkAnchorIDs.IsEmpty()&&!Production->WorkAnchorIDs.Contains(WorkAnchorID))return Fail(*ICText::Get(TEXT("Native_fb46dc569135")).ToString());
    // Recipe DT owns the duration and SP cost of recipe jobs. Production rows
    // also serve other life activities and must not override these settings.
    if(!FMath::IsFinite(Recipe->WorkDurationPlaySeconds)||Recipe->WorkDurationPlaySeconds<0||!FMath::IsFinite(Recipe->SPCost)||Recipe->SPCost<0)
        return Fail(*ICText::Get(TEXT("Native_8420321993ee")).ToString());
    if(!State->FlushParticipants())return Fail(*ICText::Get(TEXT("Native_ed7cb0d52fdb")).ToString());
    FICWorldState Next=State->ExportState();
    if(Storage&&(Recipe->RequiredExtraStorage<0||Recipe->RequiredExtraStorage>=2||Next.ProgressionStages.FindRef(TEXT("PLAYER_STORAGE"))!=Recipe->RequiredExtraStorage||Recipe->InstalledContainerID.IsNone()||Next.Inventory.Storage.Contains(Recipe->InstalledContainerID)||Recipe->InstallationAnchorIDs.Num()!=1||Recipe->HomeID.IsNone()))return Fail(TEXT("현재 설치할 수 없는 창고입니다."));
    if(!Next.bPlayerCreated)return Fail(*ICText::Get(TEXT("Native_44dad2d7eb9d")).ToString());
    if(!Recipe->bUnlockedByDefault&&!Next.UnlockedRecipeIDs.Contains(RecipeID))return Fail(*ICText::Get(TEXT("Native_3186c7959639")).ToString());
    for(const auto& Pair:Next.ProductionJobs)
        if(Pair.Value.OwnerCharacterID==UICWorldStateSubsystem::PlayerId()&&(Pair.Value.Status==EICProductionJobStatus::Running||Pair.Value.Status==EICProductionJobStatus::Ready))
            return Fail(*ICText::Get(TEXT("Native_c6ffb67d4c4b")).ToString());
    auto* Player=Next.Characters.Find(UICWorldStateSubsystem::PlayerId());
    if(!Player||Player->HP<=0||Player->IsDowned()||Player->SP<Recipe->SPCost)return Fail(*ICText::Get(TEXT("Native_f17087bb7433")).ToString());
    if(Recipe->RequiredMainStatValue>0&&Player->MainStats.Get(Recipe->RequiredMainStat)<Recipe->RequiredMainStatValue)return Fail(TEXT("제작에 필요한 능력치가 부족합니다."));
    for(const auto& Tool:Recipe->Tools)if(!Definitions->Find<FICItemRow>(Tool.ItemID,R)||Tool.Count<=0||Inventory->CountItem(Tool.ItemID)<Tool.Count)
        return Fail(*ICText::Get(TEXT("Native_b6a433a1e2f6")).ToString());
    for(const auto& Tool:Production->Tools)if(!Definitions->Find<FICItemRow>(Tool.ItemID,R)||Tool.Count<=0||Inventory->CountItem(Tool.ItemID)<Tool.Count)
        return Fail(*ICText::Get(TEXT("Native_7cfdcdab4ecb")).ToString());

    if((Recipe->Ingredients.IsEmpty()&&Recipe->IngredientGroups.IsEmpty())||(!Storage&&Recipe->Results.IsEmpty()))return Fail(*ICText::Get(TEXT("Native_adb58b3f2cfa")).ToString());
    TMap<FName,int64> Costs;
    for(const auto& Cost:Recipe->Ingredients)
    {
        if(Cost.Count<=0||!Definitions->Find<FICItemRow>(Cost.ItemID,R))return Fail(*ICText::Get(TEXT("Native_4c18c41902f5")).ToString());
        Costs.FindOrAdd(Cost.ItemID)+=Cost.Count;if(Costs[Cost.ItemID]>MAX_int32)return Fail(*ICText::Get(TEXT("Native_be3592e134a1")).ToString());
    }
    FICInventoryState Preview=Next.Inventory;
    for(const auto& Cost:Costs)if(!Inventory->Remove(Preview.Bag,Cost.Key,Cost.Value))return Fail(*ICText::Get(TEXT("Native_1a362c4b1158")).ToString());
    for(const auto& Group:Recipe->IngredientGroups)
    {
        if(Group.Count<=0||Group.ItemIDs.IsEmpty())return Fail(TEXT("재료 그룹 설정을 확인하세요."));
        int32 Remaining=Group.Count;TSet<FName> Seen;
        for(FName ID:Group.ItemIDs)
        {
            if(Seen.Contains(ID)||!Definitions->Find<FICItemRow>(ID,R))return Fail(TEXT("재료 그룹의 아이템 설정을 확인하세요."));
            Seen.Add(ID);int64 Available=0;
            for(const auto& Item:Preview.Bag.Slots)if(!Item.IsEmpty()&&!Item.bLocked&&Item.ItemID==ID)Available+=Item.Count;
            const int32 Take=static_cast<int32>(FMath::Min<int64>(Remaining,Available));
            if(Take>0)
            {
                Costs.FindOrAdd(ID)+=Take;if(Costs[ID]>MAX_int32||!Inventory->Remove(Preview.Bag,ID,Take))return Fail(TEXT("재료 수량 설정을 확인하세요."));
                Remaining-=Take;
            }
        }
        if(Remaining>0)return Fail(*FText::Format(NSLOCTEXT("ICCraft","GroupMissing","{0} 재료가 부족합니다. 합계 {1}개가 필요합니다."),Group.DisplayName,FText::AsNumber(Group.Count)).ToString());
    }
    for(const auto& Result:Recipe->Results)
    {
        const auto* Item=Definitions->Find<FICItemRow>(Result.ItemID,R);
        if(!Item||Item->Kind==EPlayerItemKind::Quest||Result.Count<=0||Inventory->Insert(Preview.Bag,Result.ItemID,Result.Count)!=Result.Count)
            return Fail(*ICText::Get(TEXT("Native_d290b2e0fe80")).ToString());
    }
    FICProductionJobState Job;Job.JobID=FGuid::NewGuid();Job.OwnerCharacterID=Player->CharacterId;
    Job.RecipeID=RecipeID;Job.ProductionID=Recipe->ProductionID;Job.WorkAnchorID=WorkAnchorID;
    Job.StartedAtPlaySecond=Clock->GetClock().PlaySecond;Job.CompletesAtPlaySecond=Job.StartedAtPlaySecond+Recipe->WorkDurationPlaySeconds;

    Job.ReservedSP=Recipe->SPCost;Job.bCostsCommitted=true;Player->SP-=Recipe->SPCost;
    for(const auto& Cost:Costs)
    {
        int32 Remaining=Cost.Value;
        for(const auto& Item:Next.Inventory.Bag.Slots)if(Item.ItemID==Cost.Key&&!Item.bLocked&&Remaining>0)
        {
            FICMaterialReservationState Reservation;Reservation.ItemInstanceID=Item.InstanceID;Reservation.ItemID=Item.ItemID;
            Reservation.Count=FMath::Min(Remaining,Item.Count);Job.Materials.Add(Reservation);Remaining-=Reservation.Count;
        }
        if(Remaining)return Fail(*ICText::Get(TEXT("Native_7be88f327b17")).ToString());
    }
    Next.ProductionJobs.Add(Job.JobID,Job);
    if(!Inventory->ValidateSnapshot(Next.Inventory)||!State->CommitGameplayTransaction(Next,Job.JobID))return Fail(*ICText::Get(TEXT("Native_a03ef09e44cf")).ToString());
    RuntimeInventory=Inventory;JobID=Job.JobID;UE_LOG(LogICProduction, Log, TEXT("[Production.Started] Job=%s Recipe=%s Anchor=%s CompleteAt=%.2f SPCost=%.2f"),*JobID.ToString(),*RecipeID.ToString(),*WorkAnchorID.ToString(),Job.CompletesAtPlaySecond,Job.ReservedSP);OnProductionChanged.Broadcast(JobID,ICText::Get(TEXT("Production_Started")).ToString());return true;
}

bool UICProductionSubsystem::CompleteJob(FGuid JobID,UICInventoryComponent* Inventory,FString& Reason)
{
    IC_TRACE_FUNCTION();
    Reason.Reset();auto Fail=[&](const TCHAR* Text){ IC_TRACE_FUNCTION();Reason=Text;UE_LOG(LogICProduction, Verbose, TEXT("[Production.Complete.Rejected] Job=%s Reason=%s"),*JobID.ToString(),Text);return false;};FICDefinitionResult R;
    if(!State||!Clock||!Definitions||!Inventory)return Fail(*ICText::Get(TEXT("Native_1e038cacd7e3")).ToString());
    if(!State->FlushParticipants())return Fail(*ICText::Get(TEXT("Native_ed7cb0d52fdb")).ToString());
    FICWorldState Next=State->ExportState();auto* Job=Next.ProductionJobs.Find(JobID);
    if(!Job || Job->Status==EICProductionJobStatus::Cancelled || Job->Status==EICProductionJobStatus::Completed)return Fail(*ICText::Get(TEXT("Native_2003cd00007c")).ToString());
    if(Clock->GetClock().PlaySecond<Job->CompletesAtPlaySecond)return Fail(*ICText::Get(TEXT("Native_b7ee8700f83d")).ToString());
    const auto* Recipe=Definitions->Find<FICRecipeRow>(Job->RecipeID,R);
    const auto* Production=Definitions->Find<FICProductionRow>(Job->ProductionID,R);
    if(!Recipe||!Production||Recipe->ProductionID!=Job->ProductionID||(Recipe->ResultKind!=EICProductionResultKind::BagItems&&Recipe->ResultKind!=EICProductionResultKind::InstallStorage)||(Recipe->ResultKind==EICProductionResultKind::BagItems&&Recipe->Results.IsEmpty()))
        return Fail(*ICText::Get(TEXT("Native_9bd6a6be260b")).ToString());
    if(Job->OwnerCharacterID!=UICWorldStateSubsystem::PlayerId())return Fail(*ICText::Get(TEXT("Native_0b3335415e48")).ToString());
    if(Recipe->ResultKind==EICProductionResultKind::InstallStorage)
    {
        if(Recipe->RequiredExtraStorage<0||Recipe->RequiredExtraStorage>=2||Next.ProgressionStages.FindRef(TEXT("PLAYER_STORAGE"))!=Recipe->RequiredExtraStorage||Next.Inventory.Storage.Contains(Recipe->InstalledContainerID)||Recipe->InstallationAnchorIDs.Num()!=1||Recipe->StorageCapacityAdded!=40)return Fail(TEXT("창고 설치 상태가 변경되었습니다."));
        Next.Inventory.Storage.Add(Recipe->InstalledContainerID).Slots.SetNum(40);
        auto& Installed=Next.StorageInstallations.Add(Recipe->InstallationAnchorIDs[0]);Installed.HomeID=Recipe->HomeID;Installed.ContainerID=Recipe->InstalledContainerID;Installed.ProductionID=Recipe->ProductionID;
        Next.ProgressionStages.Add(TEXT("PLAYER_STORAGE"),Recipe->RequiredExtraStorage+1);
    }
    if(!Job->bCostsCommitted)
    {
        auto* Owner=Next.Characters.Find(Job->OwnerCharacterID);
        if(!Owner||Owner->SP<Job->ReservedSP||Next.Gold<Job->ReservedGold)return Fail(*ICText::Get(TEXT("Native_bf04650a49c0")).ToString());
        Owner->SP-=Job->ReservedSP;Next.Gold-=Job->ReservedGold;
    }

    for(const auto& Reserved:Job->Materials)
    {
        if(Reserved.Timing==EICProductionMaterialTiming::ConsumeAtStart&&Job->bCostsCommitted)continue;
        auto* Item=Next.Inventory.Bag.Slots.FindByPredicate([&](const auto& I){ IC_TRACE_FUNCTION();return I.InstanceID==Reserved.ItemInstanceID;});
        if(!Item||Item->ItemID!=Reserved.ItemID||Item->bLocked||Item->Count<Reserved.Count||!Inventory->Find(Item->ItemID))return Fail(*ICText::Get(TEXT("Native_b7bed5bf8b3c")).ToString());
        Item->Count-=Reserved.Count;if(Item->Count==0)*Item=FPlayerItemInstance();
    }
    for(const auto& Result:Recipe->Results)
        if(Result.Count<=0||Inventory->Insert(Next.Inventory.Bag,Result.ItemID,Result.Count)!=Result.Count)
            return Fail(*ICText::Get(TEXT("Native_58afefb41c27")).ToString());
    Job->Status=EICProductionJobStatus::Completed;Job->bResultsCommitted=Job->bCostsCommitted=true;Job->CompletionTransactionID=FGuid::NewGuid();
    const FGuid Transaction=Job->CompletionTransactionID;
    if(!Inventory->ValidateSnapshot(Next.Inventory)||!State->CommitGameplayTransaction(Next,Transaction))return Fail(*ICText::Get(TEXT("Native_92ba81004379")).ToString());
    UE_LOG(LogICProduction, Log, TEXT("[Production.Completed] Job=%s Transaction=%s"),*JobID.ToString(),*Transaction.ToString());
    if (UICQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICQuestSubsystem>() : nullptr)
    {
        Quests->ReportObjectiveEvent(EICQuestObjectiveType::Craft, Job->RecipeID, 1, JobID,
            Production->Activity, Recipe->Results.IsEmpty()?NAME_None:Recipe->Results[0].ItemID);
    }
    if(!Recipe->Results.IsEmpty())State->OnItemsReceived.Broadcast(Recipe->Results);
    OnProductionChanged.Broadcast(JobID,ICText::Get(TEXT("Production_Complete")).ToString());return true;
}

bool UICProductionSubsystem::CancelJob(FGuid JobID)
{
    IC_TRACE_FUNCTION();
    if(!State)return false;FICWorldState Next=State->ExportState();auto* Job=Next.ProductionJobs.Find(JobID);
    if(!Job||Job->Status==EICProductionJobStatus::Completed||Job->Status==EICProductionJobStatus::Cancelled)return false;
    Job->Status=EICProductionJobStatus::Cancelled;
    if(!State->CommitGameplayTransaction(Next,FGuid::NewGuid()))return false;
    UE_LOG(LogICProduction, Log, TEXT("[Production.Cancelled] Job=%s: 예약 재료 해제"),*JobID.ToString());OnProductionChanged.Broadcast(JobID,ICText::Get(TEXT("Production_Cancelled")).ToString());return true;
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
