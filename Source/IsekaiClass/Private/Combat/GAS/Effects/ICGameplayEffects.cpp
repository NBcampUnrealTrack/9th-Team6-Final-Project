#include "Combat/GAS/Effects/ICGameplayEffects.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Debug/ICLog.h"

namespace
{
    void AddAmountModifier(UGameplayEffect& Effect, FGameplayAttribute Attribute)
    {
        IC_TRACE_FUNCTION();
        Effect.DurationPolicy = EGameplayEffectDurationType::Instant;
        FGameplayModifierInfo Modifier;
        Modifier.Attribute = Attribute;
        Modifier.ModifierOp = EGameplayModOp::Additive;
        FSetByCallerFloat Amount; Amount.DataTag = ICTags::DataAmount;
        Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Amount);
        Effect.Modifiers.Add(Modifier);
    }
}
UICHealthEffect::UICHealthEffect() { IC_TRACE_FUNCTION(); AddAmountModifier(*this, UICCombatAttributeSet::GetHealthAttribute()); }
UICManaEffect::UICManaEffect() { IC_TRACE_FUNCTION(); AddAmountModifier(*this, UICCombatAttributeSet::GetManaAttribute()); }
UICSPChangeEffect::UICSPChangeEffect() { IC_TRACE_FUNCTION(); AddAmountModifier(*this, UICSurvivalAttributeSet::GetSPAttribute()); }
UICStatusGameplayEffect::UICStatusGameplayEffect() { IC_TRACE_FUNCTION(); DurationPolicy = EGameplayEffectDurationType::Infinite; }
UICSkillCooldownEffect::UICSkillCooldownEffect() { DurationPolicy = EGameplayEffectDurationType::HasDuration; DurationMagnitude = FGameplayEffectModifierMagnitude(1.f); }
