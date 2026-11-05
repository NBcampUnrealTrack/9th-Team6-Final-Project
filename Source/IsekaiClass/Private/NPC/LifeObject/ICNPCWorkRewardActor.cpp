#include "NPC/LifeObject/ICNPCWorkRewardActor.h"

#include "NPC/Character/ICNPCBase.h"
#include "NPC/World/ICNPCContainerActor.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Debug/ICLog.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

void AICNPCWorkRewardActor::OnNPCUseStarted(AICNPCBase* NPC)
{
    if (!NPC || Rewards.IsEmpty() || FMath::FRand() > FMath::Clamp(RewardChance, 0.f, 1.f)) return;
    float TotalWeight = 0.f;
    for (const FICNPCWorkReward& Reward : Rewards) if (!Reward.ItemID.IsNone() && Reward.Count > 0) TotalWeight += FMath::Max(0.f, Reward.Weight);
    if (TotalWeight <= 0.f) return;

    float Pick = FMath::FRandRange(0.f, TotalWeight);
    for (const FICNPCWorkReward& Reward : Rewards)
    {
        Pick -= FMath::Max(0.f, Reward.Weight);
        if (Pick <= 0.f)
        {
            GiveRewardToNPC(*NPC, Reward.ItemID, Reward.Count);
            return;
        }
    }
}

bool AICNPCWorkRewardActor::GiveRewardToNPC(AICNPCBase& NPC, FName RewardItemID, int32 Count) const
{
    UGameInstance* GI = GetGameInstance();
    UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    UICDefinitionSubsystem* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    if (!State || !Definitions || !State->FlushParticipants()) return false;

    const FName NPCStorageID = FindPersonalContainerID(NPC);
    if (NPCStorageID.IsNone())
    {
        UE_LOG(LogICLifeObject, Verbose, TEXT("[NPCWorkReward.NoContainer] NPC=%s Object=%s"), *NPC.GetCharacterID().ToString(), *GetDebugID().ToString());
        return false;
    }

    FICWorldState Next = State->ExportState();
    UICInventoryComponent* Inventory = NewObject<UICInventoryComponent>();
    Inventory->BeginStaging(Definitions, Next.Job);
    if (!Inventory->AddItemToStorage(Next.Inventory, NPCStorageID, RewardItemID, Count)) return false;
    const bool bCommitted = State->CommitGameplayTransaction(Next, FGuid::NewGuid());
    UE_LOG(LogICLifeObject, Verbose, TEXT("[NPCWorkReward] NPC=%s Container=%s Item=%s Count=%d Success=%d"),
        *NPC.GetCharacterID().ToString(), *NPCStorageID.ToString(), *RewardItemID.ToString(), Count, bCommitted ? 1 : 0);
    return bCommitted;
}

FName AICNPCWorkRewardActor::FindPersonalContainerID(const AICNPCBase& NPC) const
{
    for (TActorIterator<AICNPCContainerActor> It(GetWorld()); It; ++It)
    {
        if (It->OwnerNPCID == NPC.GetCharacterID() && !It->ContainerID.IsNone() && !It->bPickupOnly) return It->ContainerID;
    }
    return NAME_None;
}

