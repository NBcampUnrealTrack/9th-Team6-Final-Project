#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICResourceRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICResourceRow : public FICDefinitionRow
{
    GENERATED_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource")
    EICLifeActivity Activity = EICLifeActivity::Gathering;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource")
    FName RegionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource")
    FName ProductionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource")
    TArray<FName> PlacementAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource")
    TArray<FICItemAmount> Yields;


    UPROPERTY(meta=(DeprecatedProperty,DeprecationMessage="Use RegenerationGameMinutes."))
    double RegenerationPlaySeconds = 300.0;

    /** Respawn uses the game calendar, including sleep and time skips. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ClampMin="0.0", Units="min"))
    double RegenerationGameMinutes = 120.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftClassPtr<AActor> ResourceActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UStaticMesh> AvailableMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UStaticMesh> DepletedMesh;

    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation") FTransform VisualTransform;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation") TSoftObjectPtr<class UGeometryCollection> OreCollection;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation") TSoftObjectPtr<class UMaterialInterface> MaterialOverride;
};
