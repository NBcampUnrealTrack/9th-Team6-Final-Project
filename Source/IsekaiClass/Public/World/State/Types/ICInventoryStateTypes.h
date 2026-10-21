#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICInventoryStateTypes.generated.h"

USTRUCT(BlueprintType)
struct FICContainerOwnershipState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName OwnerCharacterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bPlayerOwned = false;
};

USTRUCT(BlueprintType)
struct FICStorageInstallationState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ContainerID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ProductionID;
};

USTRUCT(BlueprintType)
struct FICInventoryState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) FPlayerContainer Bag;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FPlayerItemInstance> QuestItems;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FName> QuickSlots;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FPlayerContainer> Storage;
    FICInventoryState()
    {
        IC_TRACE_FUNCTION();
        Bag.Slots.SetNum(20);
        Equipment.SetNum(4);
        QuickSlots.SetNum(4);
        Storage.Add(TEXT("PlayerHome")).Slots.SetNum(40);
    }
};
