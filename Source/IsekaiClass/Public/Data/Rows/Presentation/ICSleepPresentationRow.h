#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ICSleepPresentationRow.generated.h"
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSleepPresentationRow : public FTableRowBase
{
    GENERATED_BODY()
    /** Visible rest pose duration; the legacy field name preserves existing DataTable rows. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.1",DisplayName="Rest Pose Seconds")) float EnterSeconds=.8f;
    /** Each cut uses fade out -> black hold -> fade in, hiding bed/standing pose changes. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.1")) float FadeOutSeconds=.25f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float BlackHoldSeconds=.08f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.1")) float FadeInSeconds=.25f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector CameraOffset=FVector(420,450,300);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector CameraFocusOffset=FVector(0,0,70);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="30",ClampMax="100")) float FieldOfView=70.f;
};
