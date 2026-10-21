#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICStatusEffectRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICStatusEffectRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") EICStatusKind Kind = EICStatusKind::StatModifier;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") bool bHarmful = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect", meta=(ClampMin="0")) double Strength = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") TArray<FICDefinitionRef> SourceDefinitions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") EICEffectTargetSide TargetSide = EICEffectTargetSide::Any;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") EICCombatTargetState TargetState = EICCombatTargetState::Living;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") FICConditionGroup ApplicationConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lifetime") bool bPermanent = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lifetime", meta=(ClampMin="0")) double DurationPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lifetime") FICConditionGroup PermanentEndConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lifetime", meta=(ClampMin="0")) double TickIntervalPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lifetime") bool bTickImmediatelyOnApplication = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacking") EICStackingPolicy StackingPolicy = EICStackingPolicy::KeepStrongerRefreshDuration;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stacking", meta=(ClampMin="1")) int32 MaximumStacks = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifiers") TArray<FICStatModifier> StatModifiers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifiers", meta=(ClampMin="0")) double DamageDealtMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifiers", meta=(ClampMin="0")) double DamageReceivedMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifiers", meta=(ClampMin="0")) double HealingReceivedMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Periodic") EICDamageType PeriodicDamageType = EICDamageType::Physical;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Periodic") FICCombatMagnitude PeriodicMagnitude;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Periodic") bool bPeriodicCanCritical = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Periodic") bool bPeriodicCanBeGuarded = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Periodic") bool bPeriodicCanBeAutoDodged = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Periodic") bool bPeriodicCancelsCasting = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shield") FICCombatMagnitude InitialShieldMagnitude;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control") FGameplayTagContainer GrantedStateTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Immunity") FGameplayTagContainer BlockedByTargetTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Immunity") TArray<FName> GrantedStatusImmunityIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Immunity") FGameplayTagContainer GrantedStatusImmunityTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Immunity") bool bBlockedByInvulnerability = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dispel") TArray<FName> AllowedCureItemIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dispel") TArray<FName> AllowedDispelSkillIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dispel") FGameplayTagContainer DispelTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dispel") bool bRemovedByLivingTargetSleep = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect") FGameplayTag CustomExecutionTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FICCombatPresentation Presentation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0")) double ExpiryWarningPlaySeconds = 5.0;
};
