#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCombatTypes.generated.h"

class AActor;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UTexture2D;

UENUM(BlueprintType)
enum class EICDamageType : uint8 { Physical , Magical  };

UENUM(BlueprintType)
enum class EICSkillOwner : uint8 { CharacterClass , RecruitUnique , Monster  };

UENUM(BlueprintType)
enum class EICSkillPurpose : uint8 { Attack , Heal , Buff , Shield , Dispel , Revive , Utility  };

UENUM(BlueprintType)
enum class EICCombatTarget : uint8 { Self , Enemy , PartyMember , LivingParty , DownedCompanion , GroundPoint , Forward  };

UENUM(BlueprintType)
enum class EICCombatTargetState : uint8 { Living , Downed , LivingOrDowned  };

UENUM(BlueprintType)
enum class EICEffectTargetSide : uint8 { Friendly , Hostile , Any  };

UENUM(BlueprintType)
enum class EICHitShape : uint8 { Direct , WeaponTrace , Projectile , Sphere , Cone , Box , GroundArea  };

UENUM(BlueprintType)
enum class EICFirstEffectEvent : uint8 { ProjectileFire , FirstHit , FirstRecovery , EffectCommit  };

UENUM(BlueprintType)
enum class EICSkillEffectKind : uint8 { Damage , Heal , Shield , ApplyStatus , Dispel , Revive , Movement , Custom  };

UENUM(BlueprintType)
enum class EICAutoSkillType : uint8 { None , Attack , Heal , Dispel , Revive , Buff  };

UENUM(BlueprintType)
enum class EICStatusKind : uint8 { StatModifier , DamageOverTime , HealOverTime , Shield , Control , Custom  };

UENUM(BlueprintType)
enum class EICStackingPolicy : uint8 { KeepStrongerRefreshDuration , Replace , AddStacks , Independent  };

UENUM(BlueprintType)
enum class EICCompanionRole : uint8 { Unspecified , Frontline , RangedDamage , MagicDamage , Healer , Support  };

UENUM(BlueprintType)
enum class EICAttackDisposition : uint8 { Aggressive , Defensive , Wait  };

UENUM(BlueprintType)
enum class EICLowHPDisposition : uint8 { Attack , Wait  };

UENUM(BlueprintType)
enum class EICTargetPriority : uint8 { PlayerTargetFirst , NearbyEnemyFirst  };

UENUM(BlueprintType)
enum class EICMonsterKind : uint8 { Normal , RegionalBoss , FinalBoss  };

UENUM(BlueprintType)
enum class EICReviveMPPolicy : uint8 { Preserve , AtLeastMaxMPRatio , SetMaxMPRatio  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCombatPresentation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UNiagaraSystem> CastVFX;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UNiagaraSystem> ImpactVFX;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<USoundBase> CastSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<USoundBase> ImpactSound;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCombatMagnitude
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude") EICStat SourceStat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude", meta=(ClampMin="0")) double Coefficient = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude", meta=(ClampMin="0")) double FlatAmount = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magnitude") bool bApplyPersonalCHA = true;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSkillTargetRules
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target") EICCombatTarget Target = EICCombatTarget::Enemy;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target") EICCombatTargetState TargetState = EICCombatTargetState::Living;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target") bool bRequiresTarget = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target") bool bAllowCaster = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target") bool bRequireLineOfSight = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0")) double MinimumRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0")) double MaximumRangeCm = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target") bool bIgnoreRangeForDowned = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target") FICConditionGroup Conditions;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSkillCancelRules
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelOnUserMovement = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bSkillMovementExempt = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelOnGuard = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelOnDodge = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelOnEffectiveHit = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelBeforeFirstEffectOnTargetDeath = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelBeforeFirstEffectOnRangeExit = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelOnSuccessfulEquipmentChange = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cancel") bool bCancelRemainingHits = true;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSkillHitDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName TraceStartSocket=TEXT("weapon_r");
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName TraceEndSocket=TEXT("weapon_tip");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) double TraceRadiusCm=8;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) double TraceDurationPlaySeconds=.15;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") FName EffectID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0")) double StartOffsetPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="1")) int32 RepeatCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0")) double RepeatIntervalPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") EICHitShape Shape = EICHitShape::Direct;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0")) double RadiusCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0", ClampMax="360")) double ConeAngleDegrees = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") FVector BoxHalfExtentCm = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="1")) int32 MaximumTargets = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") bool bPierceTargets = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") bool bStopAtWorldCollision = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") bool bHitSameTargetOncePerRepeat = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0")) double ProjectileSpeedCmPerSecond = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta=(ClampMin="0")) double ProjectileLifetimePlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") TSoftClassPtr<AActor> ProjectileClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") FName MontageSection;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit") FName NotifyName;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICAutoSkillRules
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto") EICAutoSkillType Type = EICAutoSkillType::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto") FICConditionGroup Conditions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto", meta=(ClampMin="0", ClampMax="1")) double HealAtOrBelowHPRatio = 0.5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto") bool bRequiresMissingStatusEffect = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Auto") bool bRequiresDispellableStatusEffect = true;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICInitialEquipment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment") FName WeaponID;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNormalAttackStage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0")) double DamageCoefficient = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0")) double FixedDamage = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") FICSkillHitDefinition Hit;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") TSoftObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") FName MontageSection;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0")) double GuardDodgeCancelStartPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0")) double GuardDodgeCancelEndPlaySeconds = 0.0;
};
