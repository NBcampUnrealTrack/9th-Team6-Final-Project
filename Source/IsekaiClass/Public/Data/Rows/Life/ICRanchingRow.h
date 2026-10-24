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
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> Products;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) double ProductionIntervalGameMinutes=1440;
};
