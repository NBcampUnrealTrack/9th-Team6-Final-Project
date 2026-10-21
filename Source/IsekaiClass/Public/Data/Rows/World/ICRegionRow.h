#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICRegionRow.generated.h"

class USoundBase;
class UWorld;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICRegionRow : public FICDefinitionRow
{
    GENERATED_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") TSoftObjectPtr<UWorld> World;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(Units="cm")) FBox BoundsCm = FBox(ForceInit);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") TArray<FVector2D> BoundaryPolygonCm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") FName DefaultAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") TArray<FName> AnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") TArray<FName> SpawnerIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") TArray<FName> WarpIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") TArray<FName> NPCIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") FName RegionBossMonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") FName LandmarkID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region") FName TownMapID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ClampMin="0")) int32 DifficultyOrder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TSoftObjectPtr<USoundBase> ExplorationBGM;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TSoftObjectPtr<USoundBase> CombatBGM;
};
