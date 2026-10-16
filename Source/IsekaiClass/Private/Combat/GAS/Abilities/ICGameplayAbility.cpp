#include "Combat/GAS/Abilities/ICGameplayAbility.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Debug/ICLog.h"
UICGameplayAbility::UICGameplayAbility()
{
    IC_TRACE_FUNCTION();
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}
void UICGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    IC_TRACE_FUNCTION();
    auto* ASC = ActorInfo ? Cast<UICAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
    if (!ASC || !ASC->BeginRequestedAction(this)) EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
}
void UICGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    IC_TRACE_FUNCTION();
    if (ActorInfo) if (auto* ASC = Cast<UICAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())) ASC->EndCurrentAction(bWasCancelled);
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
void UICGameplayAbility::Finish(bool Cancelled)
{ IC_TRACE_FUNCTION(); if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, Cancelled); }
