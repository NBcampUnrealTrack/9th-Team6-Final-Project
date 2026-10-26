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
    /** Additional equipped weapons, indexed by family - 1. The selected instance
     * remains in Equipment[Weapon] for combat/save compatibility; GetWeaponLoadout
     * combines both into the four permanent equipment slots without duplication. */
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FPlayerItemInstance> WeaponSlots;
    /** The selected weapon stays owned/equipped while put away. Old saves keep their drawn weapon. */
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bWeaponSheathed = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FName> QuickSlots;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FPlayerContainer> Storage;
    FICInventoryState()
    {
        IC_TRACE_FUNCTION();
        Bag.Slots.SetNum(20);
        Equipment.SetNum(1);
        WeaponSlots.SetNum(4);
        QuickSlots.SetNum(4);
        Storage.Add(TEXT("PlayerHome")).Slots.SetNum(40);
    }
};
