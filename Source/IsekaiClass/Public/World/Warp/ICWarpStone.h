#pragma once

#include "CoreMinimal.h"
#include "Interaction/ICInteractionActor.h"
#include "ICWarpStone.generated.h"

class AICPlayerController;
class AICPlayerCharacter;

/** A world interaction actor that unlocks its WarpID when first used. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICWarpStone : public AICInteractionActor
{
    GENERATED_BODY()

public:
    AICWarpStone();

    /** Stable endpoint ID authored in DT_IC_Warp. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|Warp", meta=(ExposeOnSpawn="true")) FName WarpID;

    /** Global warp fee. Keep the same value on every stone while prices are region-independent. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|Warp", meta=(ClampMin="0")) int32 CostGold = 500;

    virtual bool TryInteract_Implementation(AICPlayerController* Controller) override;

    UFUNCTION(BlueprintPure, Category="IC|Warp")
    TArray<FName> GetUnlockedDestinations(AICPlayerCharacter* Player) const;

    UFUNCTION(BlueprintCallable, Category="IC|Warp")
    bool WarpToDestination(AICPlayerCharacter* Player, FName DestinationWarpID, FString& OutReason);
};
