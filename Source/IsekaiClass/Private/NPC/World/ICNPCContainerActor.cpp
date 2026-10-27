#include "NPC/World/ICNPCContainerActor.h"

#include "Core/Controller/ICPlayerController.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateSubsystem.h"

AICNPCContainerActor::AICNPCContainerActor()
{
    IC_TRACE_FUNCTION();
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
    Controller->OpenContainer(ContainerID, false, true);
    UE_LOG(LogICInteraction, Log, TEXT("[NPCContainer.Opened] Object=%s Owner=%s Container=%s"), *GetName(), *OwnerNPCID.ToString(), *ContainerID.ToString());
    return true;
}
