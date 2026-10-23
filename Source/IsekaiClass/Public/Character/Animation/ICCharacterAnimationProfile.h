#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICCharacterAnimationProfile.generated.h"

class UAnimMontage;
class UAnimSequence;
class UBlendSpace;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCharacterAnimationSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UBlendSpace> Locomotion;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Jump;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Attack;
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
