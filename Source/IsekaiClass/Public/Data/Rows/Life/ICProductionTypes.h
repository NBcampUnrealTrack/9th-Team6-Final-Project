#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICProductionTypes.generated.h"


UENUM(BlueprintType)
enum class EICLifeActivity : uint8
{
    Crafting ,
    Cooking ,
    Planting ,
    Harvesting ,
    Fishing ,
    Woodcutting ,
    Mining ,
    Gathering
};

UENUM(BlueprintType)
enum class EICProductionResultKind : uint8
{
    BagItems ,
    InstallStorage ,
    PlantCrop ,
    HarvestCrop ,
    FishingCatch ,
    GatherResource
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICProductionToolRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tool")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tool", meta=(ClampMin="1"))
    int32 Count = 1;
};

UENUM(BlueprintType)
enum class EICProductionMaterialTiming : uint8
{

    ConsumeOnSuccess ,

    ConsumeAtStart
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICProductionMaterialRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
    FICItemAmount Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
    EICProductionMaterialTiming ConsumptionTiming = EICProductionMaterialTiming::ConsumeOnSuccess;
};
