#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICScheduleRow.generated.h"

UENUM(BlueprintType)
enum class EICScheduleActivity : uint8 { Idle , Move , Work , Sleep  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICScheduleSegment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule") FName SegmentID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="0", ClampMax="1439", Units="min")) int32 StartGameMinute = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="1", ClampMax="1440", Units="min")) int32 EndGameMinute = 1440;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule") EICScheduleActivity Activity = EICScheduleActivity::Idle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule") FName DepartureAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule") FName DestinationAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule") FGameplayTag ActivityTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule") TArray<FName> ServiceIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FName ActivityResourceID;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICScheduleRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule") TArray<FICScheduleSegment> Segments;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="0", Units="s")) double PathFailureRecoveryPlaySeconds = 5.0;

};
