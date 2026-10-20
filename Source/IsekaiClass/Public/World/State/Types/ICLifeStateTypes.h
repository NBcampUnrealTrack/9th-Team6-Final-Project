#pragma once
#include "CoreMinimal.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICLifeStateTypes.generated.h"

UENUM(BlueprintType)
enum class EICProductionJobStatus : uint8 { Running , Ready , Completed , Cancelled  };

USTRUCT(BlueprintType)
struct FICMaterialReservationState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid ItemInstanceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Count = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICProductionMaterialTiming Timing = EICProductionMaterialTiming::ConsumeOnSuccess;
};


USTRUCT(BlueprintType)
struct FICProductionJobState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid JobID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName OwnerCharacterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ProductionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName RecipeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName WorkAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICProductionJobStatus Status = EICProductionJobStatus::Running;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double StartedAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double CompletesAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FICMaterialReservationState> Materials;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 ReservedGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double ReservedSP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bCostsCommitted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bResultsCommitted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid CompletionTransactionID;
};

USTRUCT(BlueprintType)
struct FICCropState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName CropID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName OwnerCharacterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double PlantedAtGameMinute = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double HarvestableAtGameMinute = 0;
};
