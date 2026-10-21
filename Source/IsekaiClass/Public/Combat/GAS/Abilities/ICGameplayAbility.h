#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ICGameplayAbility.generated.h"

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
};
