#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICCharacterAnimationProfile.generated.h"

class UAnimMontage;
class UAnimSequence;
class UBlendSpace;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCharacterAnimationSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UBlendSpace> Locomotion;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UBlendSpace> CombatLocomotion;
    /** Clockwise: forward, forward-right, right, back-right, back, back-left, left, forward-left. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UAnimMontage>> DirectionalDodges;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Jump;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> JumpStart;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> FallLoop;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Land;
    /** Player jump height above the takeoff point. Zero keeps CharacterMovement defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump", meta=(ClampMin="0", Units="cm")) float JumpHeightCm=0.f;
    /** Time from takeoff back to the same height; used when JumpHeightCm is positive. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump", meta=(ClampMin="0.2", Units="s", EditCondition="JumpHeightCm > 0")) float JumpAirTimeSeconds=.75f;
    /** Fit the whole takeoff clip into this time. Zero preserves its existing timing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump", meta=(ClampMin="0", Units="s")) float JumpStartDurationSeconds=0.f;
    /** Fit landing recovery into this time. Zero uses the clip's own duration. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump", meta=(ClampMin="0", Units="s")) float LandingDurationSeconds=0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Sprint;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Attack;
    /** Three consecutive normal attacks, authored for this character's skeleton. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UAnimMontage>> AttackCombo;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Skill;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Hit;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Dodge;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Draw;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Sheathe;
    /** Flat-ground calibration for this mesh and animation family, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float GroundOffsetCm = 0.f;
};

/** Animation assets belong to the target skeleton; weapon families select the action set. */
UCLASS(BlueprintType)
class ISEKAICLASS_API UICCharacterAnimationProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<EICWeaponFamily, FICCharacterAnimationSet> WeaponSets;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FICCharacterAnimationSet Unarmed;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Sleep;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UAnimSequence> SleepEnter;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UAnimSequence> SleepExit;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<EICLifeActivity,TObjectPtr<UAnimMontage>> LifeActions;
    /** Mesh pose relative to a bed anchor (anchor origin is the standing capsule centre). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform SleepMeshTransform = FTransform(FRotator(0,-90,0), FVector(-35,-36,22));
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EICWeaponFamily DefaultFamily = EICWeaponFamily::Sword;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float WalkSpeed = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float RunSpeed = 600.f;

    const FICCharacterAnimationSet* FindSet(EICWeaponFamily Family) const
    {
        if (Family == EICWeaponFamily::None && Unarmed.Locomotion) return &Unarmed;
        if (const auto* Found = WeaponSets.Find(Family)) return Found;
        return WeaponSets.Find(DefaultFamily);
    }
};
