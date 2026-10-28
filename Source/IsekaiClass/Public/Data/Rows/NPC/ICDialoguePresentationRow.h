#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ICDialoguePresentationRow.generated.h"

/** Default row, optionally overridden by an NPC definition ID. All distances are cm. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialoguePresentationRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bUseCloseUpCamera=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bAllowPlayerMovement=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="250")) float CloseDistanceCm=500.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName HeadBone=TEXT("J_Bip_C_Head");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="50")) float CameraDistanceCm=220.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float CameraHeightCm=10.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float CameraSideOffsetCm=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector FocusOffset=FVector(0,0,-12);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="15",ClampMax="100")) float FieldOfView=40.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float FollowSpeed=12.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float CameraCollisionRadiusCm=8.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bHidePlayerInCamera=true;
};
