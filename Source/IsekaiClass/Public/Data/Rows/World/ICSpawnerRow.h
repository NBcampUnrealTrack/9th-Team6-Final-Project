#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICSpawnerRow.generated.h"

UENUM(BlueprintType)
enum class EICLeashShape : uint8 { Radius , Box  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSpawnSlotDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ClampMin="0")) int32 SpawnSlotIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") FName MonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") FName OriginAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") FICConditionGroup Availability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leash") EICLeashShape LeashShape = EICLeashShape::Radius;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leash", meta=(ClampMin="0", Units="cm")) double LeashRadiusCm = 3000.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leash", meta=(Units="cm")) FBox OriginRelativeLeashBoundsCm = FBox(ForceInit);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Respawn") bool bOverrideRespawnTime = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Respawn", meta=(ClampMin="0", Units="s")) double RespawnPlaySeconds = 90.0;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSpawnerRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn") TArray<FICSpawnSlotDefinition> Slots;
};
