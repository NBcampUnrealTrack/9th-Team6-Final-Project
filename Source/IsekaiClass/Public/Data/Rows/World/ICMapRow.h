#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICMapRow.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EICMapType : uint8 { Large , Town , Minimap  };

UENUM(BlueprintType)
enum class EICMapMarkerType : uint8 { Player , Companion , Enemy , Shop , Warp , Quest  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICMapLayerDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layer") FName InteriorFloorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layer") FString DisplayNameKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layer") TSoftObjectPtr<UTexture2D> MapTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm")) FVector2D WorldOriginCm = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection") FVector2D WorldAxisU = FVector2D(1.0, 0.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection") FVector2D WorldAxisV = FVector2D(0.0, 1.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection") FVector2D MapOriginUV = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm")) FVector2D WorldCmPerUV = FVector2D(100000.0, 100000.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm")) FVector2D WorldBoundsMinCm = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm")) FVector2D WorldBoundsMaxCm = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICMapRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map") EICMapType Type = EICMapType::Large;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map") TArray<FICMapLayerDefinition> Layers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map") TArray<EICMapMarkerType> VisibleMarkers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ClampMin="0.01")) double MinimumZoom = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ClampMin="0.01")) double MaximumZoom = 4.0;


};
