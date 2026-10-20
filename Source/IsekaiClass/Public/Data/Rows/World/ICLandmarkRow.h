#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICLandmarkRow.generated.h"

class UTexture2D;


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICLandmarkRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") FName AnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") FName CodexID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") TSoftObjectPtr<UTexture2D> Photo;
};
