#pragma once
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICItemRows.generated.h"

class UTexture2D;
class AICWeaponActor;

UENUM(BlueprintType)
enum class EICItemTarget : uint8 { None , PlayerOnly , LivingParty , DownedCompanion , NonCompanionRecruit  };
UENUM(BlueprintType)
enum class EICItemEffect : uint8 { None , Recovery , GrowthBook , Revival , Dispel , Expansion  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICItemRow : public FPlayerItemDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<UStaticMesh> WorldMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FVector WorldMeshScale = FVector(.25f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICItemTarget UseTarget = EICItemTarget::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowNonCompanionRecruitUse = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICItemEffect UseEffect = EICItemEffect::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> StatusEffectIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer DispelTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double RestoreMP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double RestoreHPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double RestoreMPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double RestoreSPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double RevivalHPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double RevivalMinimumMPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EquipmentID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CropID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer ToolTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bConsumeAtGrowthCap = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRejectWhenNoEffect = true;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEquipmentRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerEquipSlot Slot = EPlayerEquipSlot::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICWeaponFamily WeaponFamily = EICWeaponFamily::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICCharacterClass RequiredClass = EICCharacterClass::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICStatModifier> BaseModifiers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> WeaponMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") TSoftClassPtr<AICWeaponActor> WeaponActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AttachSocket;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform AttachmentTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EnhancementID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OptionPoolID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerItemGrade DefaultGrade = EPlayerItemGrade::Common;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEnhancementStep
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10")) int32 TargetLevel = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 GoldCost = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double SuccessProbability = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double OriginalBaseBonusRatio = 0.05;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEnhancementRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10")) int32 MaximumLevel = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EICStat> EnhancedStats;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICEnhancementStep> Steps;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICOptionRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICStat Stat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPercentage = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double MinimumValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double MaximumValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EPlayerEquipSlot> AllowedSlots;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EICWeaponFamily> AllowedWeaponFamilies;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICWeightedOption
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OptionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double Weight = 1;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICGradeOptionPool
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerItemGrade Grade = EPlayerItemGrade::Common;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="4")) int32 OptionCount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICWeightedOption> Options;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICOptionPoolRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICGradeOptionPool> Grades;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowDuplicateOptions = false;
};
