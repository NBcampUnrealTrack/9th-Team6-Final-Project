#pragma once
#include "CoreMinimal.h"
#include "ICCombatStateTypes.generated.h"


USTRUCT(BlueprintType)
struct FICActiveEffectState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid InstanceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName EffectID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName SourceEntityID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Stacks = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bInfinite = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bHarmful = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double ExpiresAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double NextTickPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double RemainingShield = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, double> CapturedMagnitudes;
};

USTRUCT(BlueprintType)
struct FICCombatState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FICActiveEffectState> Effects;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, double> SkillReadyAtPlaySecond;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double LastCapturedPlaySecond = 0;
};
