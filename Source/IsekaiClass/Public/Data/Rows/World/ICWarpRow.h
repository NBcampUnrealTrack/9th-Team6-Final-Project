#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICWarpRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICWarpRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") FName ServiceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") FName VisitAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") FName ArrivalAnchorID;
    /** Home is available immediately; other destinations require stone interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") bool bUnlockedByDefault=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp", meta=(ClampMin="0", Units="cm")) double VisitRadiusCm = 250.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp") FICConditionGroup Availability;
};
