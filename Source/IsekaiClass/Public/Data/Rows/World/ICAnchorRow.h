#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICAnchorRow.generated.h"

UENUM(BlueprintType)
enum class EICAnchorType : uint8
{
    PlayerStart , Home , Transit , Work , Bed , Kitchen , Crafting , FarmPlot ,
    StorageInstallation , WarpArrival , Landmark , FishingPoint , Resource , MonsterOrigin , Quest
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICAnchorRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor") EICAnchorType Type = EICAnchorType::Transit;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor") FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor") FName OwnerNPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor") FTransform WorldTransform = FTransform::Identity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor") FName InteriorFloorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor") bool bSafeForArrival = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ClampMin="0", Units="cm")) double ClearanceRadiusCm = 42.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ClampMin="0", Units="cm")) double ClearanceHalfHeightCm = 96.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SmartObject", meta=(AllowedClasses="/Script/SmartObjectsModule.SmartObjectDefinition")) TSoftObjectPtr<UObject> SmartObjectDefinition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SmartObject", meta=(ClampMin="0")) int32 SmartObjectSlotIndex = 0;
};
