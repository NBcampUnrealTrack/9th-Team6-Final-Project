#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICSkillRows.generated.h"

class UGameplayAbility;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSkillRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Owner") EICSkillOwner OwnerType = EICSkillOwner::CharacterClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Owner") FName ClassID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Owner") FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Owner") FName MonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Owner", meta=(ClampMin="0", ClampMax="4")) int32 ClassSlotIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Owner") EICWeaponFamily RequiredWeaponFamily = EICWeaponFamily::None;
    /** Optional fixed position, 0..14, within this weapon family's codex block. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ClampMin="-1",ClampMax="14")) int32 CodexIndex=INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Owner") bool bRequiresEquippedWeapon = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Unlock") bool bInitiallyUnlocked = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Unlock") FName PreviousSkillID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Unlock") FICConditionGroup UnlockConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") EICSkillPurpose Purpose = EICSkillPurpose::Attack;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") FICSkillTargetRules TargetRules;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) int32 MPCost = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) double CastPlaySeconds = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) double FirstEffectOffsetPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") EICFirstEffectEvent FirstEffectEvent = EICFirstEffectEvent::EffectCommit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) double RecoveryPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) double CooldownPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS") TSoftClassPtr<UGameplayAbility> AbilityClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS") TSoftClassPtr<UGameplayEffect> CooldownEffectClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS") FGameplayTag CooldownTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) double BeforeFirstEffectRetryPlaySeconds = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") FICSkillCancelRules CancelRules;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") TArray<FICSkillHitDefinition> Hits;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill") FICAutoSkillRules AutoRules;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TSoftObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") FName CastSection;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") FName RecoverySection;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FICCombatPresentation Presentation;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSkillEffectRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") EICSkillEffectKind Kind = EICSkillEffectKind::Damage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") EICDamageType DamageType = EICDamageType::Physical;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") FICCombatMagnitude Magnitude;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") bool bCanCritical = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") bool bCanBeGuarded = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") bool bCanBeAutoDodged = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") bool bCancelsCastingOnEffectiveHit = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") TArray<FName> AppliedStatusEffectIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dispel") TArray<FName> DispellableStatusEffectIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dispel") FGameplayTagContainer DispellableEffectTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dispel", meta=(ClampMin="0")) int32 MaximumDispelCount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Revive", meta=(ClampMin="0", ClampMax="1")) double ReviveHPRatio = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Revive") EICReviveMPPolicy ReviveMPPolicy = EICReviveMPPolicy::Preserve;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Revive", meta=(ClampMin="0", ClampMax="1")) double ReviveMPRatio = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0")) double MovementDistanceCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0")) double MovementPlaySeconds = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") FGameplayTag CustomExecutionTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") FICConditionGroup ApplicationConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FICCombatPresentation Presentation;
};
