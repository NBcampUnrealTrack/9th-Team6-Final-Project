#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Debug/ICLog.h"

UICCombatAttributeSet::UICCombatAttributeSet()
{
    IC_TRACE_FUNCTION();
    Health.SetBaseValue(60.f); Health.SetCurrentValue(60.f);
    MaxHealth.SetBaseValue(100.f); MaxHealth.SetCurrentValue(100.f);
    Mana.SetBaseValue(100.f); Mana.SetCurrentValue(100.f);
    MaxMana.SetBaseValue(100.f); MaxMana.SetCurrentValue(100.f);
    PhysicalAttack.SetBaseValue(10.f); PhysicalAttack.SetCurrentValue(10.f);
    MagicAttack.SetBaseValue(10.f); MagicAttack.SetCurrentValue(10.f);
    Defense.SetBaseValue(5.f); Defense.SetCurrentValue(5.f);
    AttackSpeed.SetBaseValue(1.f); AttackSpeed.SetCurrentValue(1.f);
    MoveSpeed.SetBaseValue(450.f); MoveSpeed.SetCurrentValue(450.f);
    CriticalChance.SetBaseValue(0.05f); CriticalChance.SetCurrentValue(0.05f);
    CriticalMultiplier.SetBaseValue(1.5f); CriticalMultiplier.SetCurrentValue(1.5f);
    AutoDodgeChance.SetBaseValue(0.f); AutoDodgeChance.SetCurrentValue(0.f);
}
void UICCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    IC_TRACE_FUNCTION();
    if (!FMath::IsFinite(NewValue)) NewValue = 0;
    if (Attribute == GetHealthAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    else if (Attribute == GetManaAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
    else if (Attribute == GetMaxHealthAttribute()) NewValue = FMath::Max(1.f, NewValue);
    else NewValue = FMath::Max(0.f, NewValue);
}
void UICCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    IC_TRACE_FUNCTION();
    PreAttributeBaseChange(Attribute, NewValue);
}
UICPrimaryAttributeSet::UICPrimaryAttributeSet()
{
    IC_TRACE_FUNCTION();
    STR.SetBaseValue(1.f); STR.SetCurrentValue(1.f);
    DEX.SetBaseValue(1.f); DEX.SetCurrentValue(1.f);
    INT.SetBaseValue(1.f); INT.SetCurrentValue(1.f);
    CON.SetBaseValue(1.f); CON.SetCurrentValue(1.f);
    CHA.SetBaseValue(1.f); CHA.SetCurrentValue(1.f);
    LUK.SetBaseValue(1.f); LUK.SetCurrentValue(1.f);
}
void UICPrimaryAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    IC_TRACE_FUNCTION();
    if (!FMath::IsFinite(NewValue)) NewValue = 0;
    NewValue = FMath::RoundToFloat(FMath::Clamp(NewValue, 1.f, 10000.f));
}
void UICPrimaryAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    IC_TRACE_FUNCTION();
    PreAttributeBaseChange(Attribute, NewValue);
}
UICSurvivalAttributeSet::UICSurvivalAttributeSet()
{
    IC_TRACE_FUNCTION();
    SP.SetBaseValue(70.f); SP.SetCurrentValue(70.f);
    MaxSP.SetBaseValue(100.f); MaxSP.SetCurrentValue(100.f);
}
void UICSurvivalAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    IC_TRACE_FUNCTION();
    if (!FMath::IsFinite(NewValue)) NewValue = 0;
    if (Attribute == GetSPAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxSP());
    else NewValue = FMath::Max(0.f, NewValue);
}
void UICSurvivalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    IC_TRACE_FUNCTION();
    PreAttributeBaseChange(Attribute, NewValue);
}
