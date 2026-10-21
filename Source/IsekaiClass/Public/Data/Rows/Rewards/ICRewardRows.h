#pragma once
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICRewardRows.generated.h"

UENUM(BlueprintType)
enum class EICRewardOverflow : uint8 { RejectAll , DropExcess , DeferUntilWorldReturn  };
UENUM(BlueprintType)
enum class EICDropSelection : uint8 { IndependentProbability , WeightedChoice  };

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICAffinityReward
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Amount = 0;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICRewardRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> Items;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 Gold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICAffinityReward> Affinity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> UnlockRecipeIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICRewardOverflow Overflow = EICRewardOverflow::RejectAll;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDropEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 MinimumCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 MaximumCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double Probability = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double Weight = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyPlayerLUKToProbability = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEquipmentGradeRoll = false;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEquipmentGradeWeight
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPlayerItemGrade Grade = EPlayerItemGrade::Common;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double Weight = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyPlayerLUK = false;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDropTableRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICDropSelection Selection = EICDropSelection::IndependentProbability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 Rolls = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICDropEntry> Entries;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICEquipmentGradeWeight> EquipmentGrades;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 MinimumGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 MaximumGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyPlayerLUKToGold = true;
};
