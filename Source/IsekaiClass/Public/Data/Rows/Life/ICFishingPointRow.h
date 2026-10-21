#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICFishingPointRow.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICFishingSelectionEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish")
    FName FishID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ClampMin="0.0"))
    double Weight = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ClampMin="1"))
    int32 CatchCount = 1;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish")
    TArray<FName> AcceptedBaitItemIDs;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICFishingPointRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FName RegionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FName AnchorID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FName ProductionID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FName FishingRodItemID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    TArray<FName> AcceptedBaitItemIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.0", ClampMax="1.0"))
    double SuccessChance = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    TArray<FICFishingSelectionEntry> FishSelections;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftClassPtr<AActor> PointActorClass;
};
