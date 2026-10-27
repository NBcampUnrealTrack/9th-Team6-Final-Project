#pragma once
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICGachaMachineRow.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICGachaItem
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) double Weight=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1")) int32 MinimumCount=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1")) int32 MaximumCount=1;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICGachaPool
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName PoolID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString NameKey;
    /** Pool chance is Weight divided by the sum of enabled pool weights. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) double Weight=50;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FICGachaItem> Items;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICGachaMachineRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) int32 CostGold=1000;
    /** Select one pool, then one item from that pool, for each paid interaction. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FICGachaPool> Pools;
};
