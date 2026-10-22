#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ICGameplayAbility.generated.h"

class UAbilityTask_WaitGameplayEvent;

UCLASS()
class ISEKAICLASS_API UICGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UICGameplayAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    void Finish(bool Cancelled);
private:
    UPROPERTY(Transient) TObjectPtr<UAbilityTask_WaitGameplayEvent> CombatEventTask;
    UFUNCTION() void OnCombatGameplayEvent(FGameplayEventData Payload);
};

UCLASS()
class ISEKAICLASS_API UICSkillGameplayAbility : public UICGameplayAbility
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Skill") FName SkillID;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Skill") FGameplayTag CooldownTag;
    virtual const FGameplayTagContainer* GetCooldownTags() const override;
private:
    mutable FGameplayTagContainer CachedCooldownTags;
};
