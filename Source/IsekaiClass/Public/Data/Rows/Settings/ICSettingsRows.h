#pragma once
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICSettingsRows.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNewGameRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName StartAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double InitialGameMinute = 480;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FICBaseStats InitialStats;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 InitialGold = 200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> StartingItems;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> StartingWeapons;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> StartingSkills;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> StartingQuickSlots;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double InitialHP = 60;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double InitialMP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double InitialSP = 70;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double InitialMaxHP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double InitialMaxMP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double InitialMaxSP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> InitiallyUnlockedRecipeIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 MinimumNameLength = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 MaximumNameLength = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bStartWithFullResources = true;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICTimeSettingsRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) double RealSecondsPerGameMinute = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="10", ClampMax="10", Units="s")) double SPDrainIntervalPlaySeconds = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) double SleepRecoveryPerGameMinute = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="120")) int32 MinimumSleepGameMinutes = 120;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICStatSettingsRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 MainStatCap = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double BaseMaxHP = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double HPPerCON = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double BaseMaxMP = 80;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double MPPerINT = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double BaseMaxSP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double SPPerCON = 0.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double BaseAttack = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double AttackPerMainStat = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double BaseDefense = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double DefensePerCON = 1.2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) double DEXSpeedCurveK = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) double DEXDodgeCurveK = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) double DEXCriticalCurveK = 200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) double LUKCurveK = 200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) double CHACurveK = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double DEXAttackSpeedBonus = 0.20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double DEXMoveSpeedBonus = 0.10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double DEXDodgeBonus = 0.10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double BaseCriticalChance = 0.05;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double DEXCriticalBonus = 0.20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double BaseCriticalMultiplier = 1.50;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double LUKCriticalBonus = 0.50;
    UPROPERTY() double CHABonus = 0; // Retired, serialized for existing assets.
    UPROPERTY() TArray<EICStat> CompanionCHAAffectedStats; // Legacy asset compatibility; ignored.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double LUKRewardBonus = 0.30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double CriticalChanceCap = 0.50;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double CriticalMultiplierCap = 2.50;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double AutoDodgeChanceCap = 0.15;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) double AttackSpeedMultiplierCap = 1.50;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) double MoveSpeedMultiplierCap = 1.30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double ExhaustedDamageMultiplier = 0.80;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double ExhaustedAttackSpeedMultiplier = 0.80;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double ExhaustedMoveSpeedMultiplier = 0.90;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCombatSettingsRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double WalkSpeedCmPerSecond = 250;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double RunSpeedCmPerSecond = 450;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double InputBufferPlaySeconds = 0.2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double ComboWindowPlaySeconds = 0.8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="360")) double GuardArcDegrees = 120;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double GuardDamageMultiplier = 0.30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double GuardMoveSpeedMultiplier = 0.50;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double DodgeInvulnerabilityPlaySeconds = 0.25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double DodgeDistanceCm = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double DodgeCooldownPlaySeconds = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double HitStaggerPlaySeconds = 0.2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double HitStaggerIntervalPlaySeconds = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double CanceledCastRetryPlaySeconds = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double RecentCombatPlaySeconds = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double PlayerCombatGracePlaySeconds = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) double DefenseFormulaK = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double InteractionDistanceCm = 250;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double LockOnDistanceCm = 2500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double LockOnMaintainDistanceCm = 3000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double LockOnOcclusionPlaySeconds = 1;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICPartySettingsRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="3")) int32 MaximumCompanions = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="500")) int32 RequiredRecruitAffinity = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double LowHPRatio = 0.20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double AutoHealHPRatio = 0.50;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double SearchDistanceCm = 1200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double ChaseDistanceCm = 2000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double RecoveryDistanceCm = 3000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double PathFailurePlaySeconds = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector> FormationOffsetsCm;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICInventorySettingsRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 InitialSlots = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 ExpansionSlots = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 MaximumSlots = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 StackLimit = 99;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="4")) int32 QuickSlots = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="3")) int32 MaximumStorageInstallations = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 SlotsPerStorage = 40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double DropLifetimePlaySeconds = 600;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSaveSettingsRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="4", ClampMax="4")) int32 ManualSlotCount = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="5", ClampMax="5")) int32 ClearRecordSlot = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double AutoSaveIntervalPlaySeconds = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) double AutoSaveMinimumHPRatio = 0.25;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICMapSettingsRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001", Units="cm")) double DiscoveryRadiusCm = 5000;
};
