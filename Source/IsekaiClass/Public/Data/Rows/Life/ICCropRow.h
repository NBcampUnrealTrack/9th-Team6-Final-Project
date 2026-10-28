#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCropRow.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCropStageDefinition
{
    GENERATED_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    FName StageID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ClampMin="0.0"))
    double DurationGameMinutes = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    bool bHarvestable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    FSoftObjectPath StageVFX;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCropRow : public FICDefinitionRow
{
    GENERATED_BODY()

    /** Wild/test-map crops start growing without consuming a seed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop") bool bGrowsNaturally=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop", meta=(ClampMin="0",Units="min")) double RegrowthGameMinutes=120;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop")
    FName SeedItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftClassPtr<AActor> CropActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop")
    FName PlantingProductionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop")
    FName HarvestProductionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop")
    TArray<FICCropStageDefinition> GrowthStages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop")
    TArray<FICItemAmount> HarvestYields;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop")
    TArray<FName> AllowedPlotAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UTexture2D> Icon;
};
