#include "Combat/GAS/Abilities/ICGameplayAbility.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
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
    if (!ASC) { EndAbility(Handle, ActorInfo, ActivationInfo, false, true); return; }
    CombatEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ICTags::CombatEvent, nullptr, false, false);
    CombatEventTask->EventReceived.AddDynamic(this, &ThisClass::OnCombatGameplayEvent);
    CombatEventTask->ReadyForActivation();
    if (!ASC->BeginRequestedAction(this)) EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
}
void UICGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    IC_TRACE_FUNCTION();
    if (ActorInfo) if (auto* ASC = Cast<UICAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())) ASC->EndCurrentAction(bWasCancelled);
    CombatEventTask = nullptr;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
void UICGameplayAbility::Finish(bool Cancelled)
{ IC_TRACE_FUNCTION(); if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, Cancelled); }

void UICGameplayAbility::OnCombatGameplayEvent(FGameplayEventData Payload)
{
    auto* ASC = CurrentActorInfo ? Cast<UICAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr;
    if (!ASC || !IsActive()) return;
    FName Event;
    if (Payload.EventTag == ICTags::EffectCommitEvent) Event = TEXT("AN_EffectCommit");
    else if (Payload.EventTag == ICTags::HitStartEvent) Event = TEXT("AN_HitStart");
    else if (Payload.EventTag == ICTags::HitEndEvent) Event = TEXT("AN_HitEnd");
    else if (Payload.EventTag == ICTags::ProjectileFireEvent) Event = TEXT("AN_ProjectileFire");
    else if (Payload.EventTag == ICTags::CancelWindowStartEvent) Event = TEXT("AN_CancelWindowStart");
    else if (Payload.EventTag == ICTags::CancelWindowEndEvent) Event = TEXT("AN_CancelWindowEnd");
    else if (Payload.EventTag == ICTags::IFrameStartEvent) Event = TEXT("AN_IFrameStart");
    else if (Payload.EventTag == ICTags::IFrameEndEvent) Event = TEXT("AN_IFrameEnd");
    if (!Event.IsNone()) ASC->NotifyCombatEvent(ASC->GetActionID(), Event, FMath::RoundToInt(Payload.EventMagnitude));
}

const FGameplayTagContainer* UICSkillGameplayAbility::GetCooldownTags() const
{
    CachedCooldownTags.Reset();
    if (CooldownTag.IsValid()) CachedCooldownTags.AddTag(CooldownTag);
    return CooldownTag.IsValid() ? &CachedCooldownTags : Super::GetCooldownTags();
}
