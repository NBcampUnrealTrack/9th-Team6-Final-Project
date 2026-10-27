#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICFishRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICFishRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftClassPtr<AActor> FishActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<class USkeletalMesh> SkeletalMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftClassPtr<class UAnimInstance> AnimationClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish")
    FName ItemID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish")
    FName RegionID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish")
    TArray<FName> AllowedFishingPointIDs;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish")
    TArray<FName> AcceptedBaitItemIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UStaticMesh> Mesh;
};
