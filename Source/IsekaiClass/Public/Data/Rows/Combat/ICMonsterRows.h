#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICMonsterRows.generated.h"

class APawn;
class UAnimInstance;
class UBehaviorTree;
class UBlackboardData;
class USkeletalMesh;
class UAnimSequence;


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICMonsterCombatStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="1")) int32 MaxHP = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0")) int32 MaxMP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0")) int32 PhysicalAttack = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0")) int32 MagicAttack = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0")) int32 Defense = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0")) double MoveSpeedCmPerSecond = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0")) double AttackSpeedMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ClampMax="1")) double CriticalChance = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="1")) double CriticalMultiplier = 1.0;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICMonsterSkillChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") FName SkillID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) double Weight = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") FICConditionGroup Conditions;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICBossPatternChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") FName PatternID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0")) double Weight = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") FICConditionGroup Conditions;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICBossPhaseDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ClampMin="1")) int32 PhaseIndex = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") FString NameKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") bool bUseHPThreshold = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ClampMin="0", ClampMax="1")) double EnterAtOrBelowHPRatio = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") FICConditionGroup EntryConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") TArray<FICBossPatternChoice> Patterns;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") TArray<FName> EntryStatusEffectIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") TSoftObjectPtr<UAnimMontage> TransitionMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") TSoftObjectPtr<USoundBase> BGM;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICBossPatternStep
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step") FName SkillID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ClampMin="0")) double StartOffsetPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ClampMin="1")) int32 RepeatCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ClampMin="0")) double RepeatIntervalPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step") FName MontageSection;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICPatternInvulnerabilityWindow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Window", meta=(ClampMin="0")) double StartOffsetPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Window", meta=(ClampMin="0")) double EndOffsetPlaySeconds = 0.0;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICBossPatternRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") FName MonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") FICConditionGroup SelectionConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0")) double MinimumRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0")) double MaximumRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0")) double TelegraphPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0")) double DurationPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0")) double CooldownPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") bool bUnblockable = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") bool bCancelOnPhaseChange = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") TArray<FICBossPatternStep> Steps;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern") TArray<FICPatternInvulnerabilityWindow> InvulnerabilityWindows;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TSoftObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FString WarningKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UTexture2D> WarningIcon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UNiagaraSystem> TelegraphVFX;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<USoundBase> TelegraphSound;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICMonsterRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster") EICMonsterKind Kind = EICMonsterKind::Normal;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster") FICMonsterCombatStats CombatStats;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0")) double DetectionRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0", ClampMax="360")) double SightAngleDegrees = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI") bool bRequiresLineOfSight = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0")) double AttackRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0")) double AttackIntervalPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0")) double PathFailurePlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0")) double ReturnAcceptanceRadiusCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI") TArray<FICMonsterSkillChoice> Skills;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI") TSoftObjectPtr<UBehaviorTree> BehaviorTree;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI") TSoftObjectPtr<UBlackboardData> Blackboard;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="0")) double RespawnPlaySeconds = 90.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reward") FName DropTableID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reward") FName FirstKillRewardID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reward") FName RepeatKillRewardID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") bool bImmuneToOrdinaryHitStagger = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") FICConditionGroup BossEntryConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") FICConditionGroup BossExitConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") TArray<FICBossPhaseDefinition> BossPhases;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftClassPtr<APawn> PawnClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftClassPtr<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UAnimSequence> MoveAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UAnimSequence> AttackAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UAnimSequence> HitAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UAnimSequence> DeathAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<USoundBase> HitSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UNiagaraSystem> HitVFX;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pet", meta=(ClampMin="-1",ClampMax="54")) int32 PetCodexIndex=INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pet") FText PetName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pet", meta=(MultiLine=true)) FText PetBuffDescription;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pet") TArray<FICStatModifier> PetBuffs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<USoundBase> BattleBGM;
};
