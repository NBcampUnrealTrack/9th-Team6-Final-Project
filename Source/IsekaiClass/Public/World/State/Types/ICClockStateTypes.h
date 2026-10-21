#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "ICClockStateTypes.generated.h"

USTRUCT(BlueprintType)
struct FICClockState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) double GameMinute = 480;
    UPROPERTY(BlueprintReadOnly, SaveGame) double PlaySecond = 0;
    bool IsValid() const { IC_TRACE_FUNCTION(); return FMath::IsFinite(GameMinute) && GameMinute >= 0 && FMath::IsFinite(PlaySecond) && PlaySecond >= 0; }
};
