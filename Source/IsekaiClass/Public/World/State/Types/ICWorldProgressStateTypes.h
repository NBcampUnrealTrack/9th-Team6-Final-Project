#pragma once
#include "CoreMinimal.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "World/State/Types/ICCombatStateTypes.h"
#include "ICWorldProgressStateTypes.generated.h"

USTRUCT(BlueprintType)
struct FICSpawnSlotState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) FGuid IncarnationID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) bool bRewardPending=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName SpawnerID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 SlotIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName MonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bDefeated = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double RespawnReadyAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double RespawnReadyAtGameMinute = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float HP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxHP = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxMP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double LastSimPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FICCombatState Combat;
};

USTRUCT(BlueprintType)
struct FICResourcePointState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName DefinitionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 RemainingUses = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double ReadyAtPlaySecond = 0;
};

USTRUCT(BlueprintType)
struct FICDropState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FPlayerItemInstance> Contents;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double CreatedPlaySecond = 0;
    UPROPERTY(SaveGame) bool bStorageRecovery=false;
};
