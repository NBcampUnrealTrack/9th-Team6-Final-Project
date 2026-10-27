#pragma once
#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICRanchingRow.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICRanchingRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<AActor> AnimalActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<class UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UAnimationAsset> IdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform MeshTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float MaxHP=30;
    /** Hunting rewards use the same drop-table format as monsters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DropTableID;
    /** Zero leaves a defeated animal absent until a new game. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double RespawnGameMinutes=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> Products;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) double ProductionIntervalGameMinutes=1440;
};
