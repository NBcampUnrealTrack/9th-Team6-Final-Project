#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ICPartyWarpComponent.generated.h"

/** Executes a paid warp for the player and every active, living party member. */
UCLASS(ClassGroup=(IC), meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICPartyWarpComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UICPartyWarpComponent();

    /** Registers one visited Warp Stone when the player is beside the supplied stone location. */
    UFUNCTION(BlueprintCallable, Category="IC|Warp")
    bool UnlockWarp(FName WarpID, FVector StoneLocation, FString& OutReason);

    UFUNCTION(BlueprintPure, Category="IC|Warp")
    bool IsWarpUnlocked(FName WarpID) const;

    UFUNCTION(BlueprintCallable, Category="IC|Warp")
    bool CanWarp(FName DestinationWarpID, int32 CostGold, FString& OutReason) const;

    /** Warps the owning player and active living companions. Gold is charged only on success. */
    UFUNCTION(BlueprintCallable, Category="IC|Warp")
    bool WarpParty(FName DestinationWarpID, int32 CostGold, FString& OutReason);

    /** A UI helper: all saved unlocked destination IDs. */
    UFUNCTION(BlueprintPure, Category="IC|Warp")
    TArray<FName> GetUnlockedWarpIDs() const;

private:
    class AICPlayerCharacter* GetPlayerCharacter() const;
    class UICWorldStateSubsystem* GetWorldState() const;
    bool ResolveWarp(FName WarpID, struct FICWarpRow& OutWarp, FString& OutReason) const;
};
