#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "ICPlayerSurvival.generated.h"

USTRUCT(BlueprintType)
struct FPlayerSurvivalState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float HP = 60.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxHP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxMP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float SP = 70.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxSP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double SPDrainElapsedPlaySeconds = 0;
    static constexpr double SPDrainIntervalPlaySeconds = 10;

    bool IsValid() const;
    bool IsExhausted() const { IC_TRACE_FUNCTION(); return SP <= 0.f; }
    float RestoreHP(float Amount);
    float RestoreSP(float Amount);
    float ConsumeSP(float Amount);
    float AdvanceNaturalSP(double PlaySeconds);
    int32 CalculateSleepMinutes(float RecoveryPerGameMinute) const;
    void CompleteSleep();
};
