#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Player/Stats/ICPlayerStats.h"
#include "ICInventoryTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerItemKind : uint8 { Consumable, Material, Equipment, Quest, Expansion };
UENUM(BlueprintType)
enum class EPlayerEquipSlot : uint8 { Weapon, Clothes, Ring, Necklace, None };
UENUM(BlueprintType)
enum class EPlayerItemTab : uint8 { Normal, Quest };
UENUM(BlueprintType)
enum class EPlayerItemCategory : uint8 { Consumable, Food, Book, Material, Gathering, Fish, Crafted, Quest, Ticket, Equipment, Revival };
UENUM(BlueprintType)
enum class EPlayerItemGrade : uint8 { Common, Uncommon, Rare, Epic, Legendary };

class UStaticMesh;

USTRUCT(BlueprintType)
struct FPlayerItemDefinition : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText DisplayName;
    // Existing Data Tables and V1 saves use the original property.
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText Name;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerItemTab Tab=EPlayerItemTab::Normal;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerItemCategory Category=EPlayerItemCategory::Material;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerItemKind Kind=EPlayerItemKind::Material;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerEquipSlot EquipSlot=EPlayerEquipSlot::None;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 StackLimit=99;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bCanDiscard=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bCanSell=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bCanGift=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bDeprecated=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 BuyPrice=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 SellPrice=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName EffectID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 RequiredClass=INDEX_NONE;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerMainStat GrowthStat=EPlayerMainStat::None;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 GrowthAmount=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float RestoreHP=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float RestoreSP=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerItemGrade DefaultGrade=EPlayerItemGrade::Common;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxEnhanceLevel=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxRandomOptions=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName OptionPoolID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseAttack=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseDefense=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float EnhanceAttackPerLevel=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float EnhanceDefensePerLevel=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 EnhanceBaseCost=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float EnhanceBaseSuccessRate=1.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> EquipmentMesh;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName AttachSocket;
    FText GetLabel() const { return DisplayName.IsEmpty()?Name:DisplayName; }
};

USTRUCT(BlueprintType)
struct FPlayerItemInstance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) FGuid InstanceID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Count=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Grade=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) int32 Upgrade=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) TMap<FName,float> Options;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) FName OwnerID=TEXT("Player");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) bool bLocked=false;
    bool IsEmpty() const { return ItemID.IsNone()||Count<=0; }
};

USTRUCT(BlueprintType)
struct FPlayerContainer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) TArray<FPlayerItemInstance> Slots;
};
