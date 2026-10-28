#include "NPC/World/ICNPCContainerActor.h"

#include "Core/Controller/ICPlayerController.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Inventory/Actors/ICChestPresentationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"

AICNPCContainerActor::AICNPCContainerActor()
{
    IC_TRACE_FUNCTION();
    ChestPresentation=CreateDefaultSubobject<UICChestPresentationComponent>(TEXT("ChestPresentation"));ChestPresentation->SetupAttachment(Shape);
    FurnitureLootTable=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Data/DataTable/Life/DT_IC_FurnitureLoot.DT_IC_FurnitureLoot")));
    Action = TEXT("Storage");
    bHome = false;
    bAccessGranted = true;
    InteractionText = NSLOCTEXT("ICNPCContainer", "Interaction", "개인 보관함 열기");
    LockedMessage = NSLOCTEXT("ICNPCContainer", "Locked", "호감도 200 이상에서 사용할 수 있습니다.");
}

bool AICNPCContainerActor::CanAccessContainer(const AICPlayerController* Requester) const
{
    IC_TRACE_FUNCTION();
    if (!Requester || OwnerNPCID.IsNone() || ContainerID.IsNone()) return false;
    const UICWorldStateSubsystem* State = Requester->GetWorldState();
    if (!State) return false;
    const FICCharacterState OwnerState = State->GetSnapshot(OwnerNPCID);
    return OwnerState.CharacterId == OwnerNPCID && OwnerState.Affinity >= RequiredAffinity;
}

bool AICNPCContainerActor::TryInteract_Implementation(AICPlayerController* Controller)
{
    IC_TRACE_FUNCTION();
    if (!CanAccessContainer(Controller))
    {
        if (Controller) Controller->ShowLocalized(LockedMessage);
        UE_LOG(LogICInteraction, Verbose, TEXT("[NPCContainer.AccessDenied] Object=%s Owner=%s Container=%s"), *GetName(), *OwnerNPCID.ToString(), *ContainerID.ToString());
        return false;
    }

    // The generic inventory layer owns item movement and persistence. It receives
    // only this container's stable ID; NPC access policy stays in this actor.
    if(!bPickupOnly)return ChestPresentation->OpenForPlayer(Controller,ContainerID,false);
    if(!IsFurnitureLootAvailable()){Controller->ShowLocalized(NSLOCTEXT("ICFurniture","Empty","이미 확인한 가구입니다."));return true;}
    auto* Table=FurnitureLootTable.LoadSynchronous();const auto* Row=Table?Table->FindRow<FICFurnitureLootRow>(FurnitureLootRow,TEXT("Furniture"),false):nullptr;
    auto* State=Controller->GetWorldState();auto* GI=GetGameInstance();if(!Row||!State||!GI||!State->FlushParticipants())return false;
    auto Next=State->ExportState();const FGuid Receipt=FGuid::NewGuid();TArray<FICItemAmount> Received;FString Failure;
    auto* Clock=GI->GetSubsystem<UICTimeSubsystem>();
    if(!FICRewardService::PrepareLifeLoot(Next,Row->DropTableID,Receipt,GI->GetSubsystem<UICDefinitionSubsystem>(),GetActorLocation(),Clock->GetClock().PlaySecond,Received,Failure))return false;
    const FName Key=ContainerID.IsNone()?GetFName():ContainerID;
    Next.FurnitureLootReadyAt.Add(Key,Row->RefillGameMinutes>0?Clock->GetClock().GameMinute+Row->RefillGameMinutes:-1.);
    if(!State->CommitGameplayTransaction(Next,Receipt))return false;
    if(!Received.IsEmpty())State->OnItemsReceived.Broadcast(Received);return true;
}
bool AICNPCContainerActor::IsFurnitureLootAvailable() const
{
    if(!bPickupOnly||!GetGameInstance())return false;
    const auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();const auto Snapshot=State->ExportState();
    const auto* Ready=Snapshot.FurnitureLootReadyAt.Find(ContainerID.IsNone()?GetFName():ContainerID);
    return !Ready||(*Ready>=0&&GetGameInstance()->GetSubsystem<UICTimeSubsystem>()->GetClock().GameMinute>=*Ready);
}
FText AICNPCContainerActor::GetInteractionText() const
{
    if(bPickupOnly&&!IsFurnitureLootAvailable())return NSLOCTEXT("ICFurniture","EmptyHint","가구 · 비어 있음");
    return Super::GetInteractionText();
}
