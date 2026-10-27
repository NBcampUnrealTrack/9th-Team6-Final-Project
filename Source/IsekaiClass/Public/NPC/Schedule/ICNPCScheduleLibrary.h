#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "ICNPCScheduleLibrary.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNPCScheduleResolution
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") bool bFound = false;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") int32 GameMinuteOfDay = 0;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") FName SegmentID;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") EICScheduleActivity Activity = EICScheduleActivity::Idle;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") FName DepartureAnchorID;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") FName DestinationAnchorID;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") FGameplayTag ActivityTag;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") int32 StartGameMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="IC|NPC|Schedule") int32 EndGameMinute = 0;
};

/** Single source of truth for resolving an NPC's data-driven daily schedule. */
UCLASS()
class ISEKAICLASS_API UICNPCScheduleLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="IC|NPC|Schedule")
    static FICNPCScheduleResolution ResolveSchedule(const FICScheduleRow& Schedule, double GameMinute);
};
