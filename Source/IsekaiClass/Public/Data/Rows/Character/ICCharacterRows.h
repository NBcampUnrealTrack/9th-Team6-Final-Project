#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICCharacterRows.generated.h"

class ACharacter;
class UAnimInstance;
class USkeletalMesh;
class UICCharacterAnimationProfile;

UENUM(BlueprintType)
enum class EICPlayerPresetGender : uint8 { Male , Female  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICClassRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") EICCharacterClass CharacterClass = EICCharacterClass::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") EICWeaponFamily WeaponFamily = EICWeaponFamily::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") EICDamageType NormalAttackDamageType = EICDamageType::Physical;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName SkillSlot1ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName SkillSlot2ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName SkillSlot3ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName SkillSlot4ID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName StartingWeaponID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") TArray<FICNormalAttackStage> NormalAttackStages;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName WeaponRightSocket = TEXT("weapon_r");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName WeaponLeftSocket = TEXT("weapon_l");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") FName MuzzleSocket = TEXT("muzzle");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Class") TSoftObjectPtr<UTexture2D> Icon;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICPlayerPresetRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") EICPlayerPresetGender Gender = EICPlayerPresetGender::Male;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") TSoftClassPtr<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") TSoftClassPtr<ACharacter> CharacterClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") FName AppearanceResourceID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") TSoftObjectPtr<UICCharacterAnimationProfile> AnimationProfile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preset") FTransform MeshTransform = FTransform(FRotator(0,-90,0),FVector(0,0,-88));
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICRecruitCombatRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") FName ClassID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") EICWeaponFamily WeaponFamily = EICWeaponFamily::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") EICCompanionRole Role = EICCompanionRole::Unspecified;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") FICBaseStats InitialBaseStats;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") FICInitialEquipment InitialEquipment;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") FName UniqueSkillID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") EICAttackDisposition InitialAttackDisposition = EICAttackDisposition::Defensive;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") EICLowHPDisposition InitialLowHPDisposition = EICLowHPDisposition::Wait;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") EICTargetPriority InitialTargetPriority = EICTargetPriority::PlayerTargetFirst;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recruit") bool bInitiallyAutoSkillEnabled = false;
};
