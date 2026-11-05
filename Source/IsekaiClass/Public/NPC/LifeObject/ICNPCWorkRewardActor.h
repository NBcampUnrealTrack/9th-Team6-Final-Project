#pragma once

#include "CoreMinimal.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "ICNPCWorkRewardActor.generated.h"

/** One weighted result for a successful NPC workstation interaction. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNPCWorkReward
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reward") FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reward", meta=(ClampMin="1")) int32 Count = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reward", meta=(ClampMin="0.0")) float Weight = 1.f;
};

/**
 * Base class for NPC-operated counters, ovens, and workbenches which may create
 * one item for the NPC that actually uses the object.
 */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICNPCWorkRewardActor : public AICLifeObjectActor
{
    GENERATED_BODY()
public:
    /** Roll once when an NPC begins using this workstation. 1.0 means always. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC Reward", meta=(ClampMin="0.0", ClampMax="1.0")) float RewardChance = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC Reward") TArray<FICNPCWorkReward> Rewards;

    virtual void OnNPCUseStarted(AICNPCBase* NPC) override;

protected:
    bool GiveRewardToNPC(AICNPCBase& NPC, FName RewardItemID, int32 Count) const;
    FName FindPersonalContainerID(const AICNPCBase& NPC) const;
};
