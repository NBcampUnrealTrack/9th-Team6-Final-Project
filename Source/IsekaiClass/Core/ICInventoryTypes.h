#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ICInventoryTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerItemKind : uint8 { Consumable, Material, Equipment, Quest, Expansion };
UENUM(BlueprintType)
enum class EPlayerEquipSlot : uint8 { Weapon, Clothes, Ring, Necklace, None };

USTRUCT(BlueprintType)
struct FPlayerItemDefinition : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerItemKind Kind = EPlayerItemKind::Material;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerEquipSlot EquipSlot = EPlayerEquipSlot::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StackLimit = 99;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanDiscard = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanSell = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EffectID;
};

USTRUCT(BlueprintType)
struct FPlayerItemInstance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid InstanceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Count = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Grade = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Upgrade = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, float> Options;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName OwnerID = TEXT("Player");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bLocked = false;
    bool IsEmpty() const { return ItemID.IsNone() || Count <= 0; }
};

USTRUCT(BlueprintType)
struct FPlayerContainer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FPlayerItemInstance> Slots;
};
