#pragma once
#include "CoreMinimal.h"
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

    bool IsValid() const;
    bool IsExhausted() const { return SP <= 0.f; }
    float RestoreHP(float Amount);
    float RestoreSP(float Amount);
    float ConsumeSP(float Amount);
    int32 CalculateSleepMinutes(float RecoveryPerGameMinute) const;
    void CompleteSleep();
};
