#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Debug/ICLog.h"
#include "ICAttributeSets.generated.h"

UCLASS()
class ISEKAICLASS_API UICCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UICCombatAttributeSet();
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData Health;
    static FGameplayAttribute GetHealthAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, Health))); }
    float GetHealth() const { IC_TRACE_FUNCTION(); return Health.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData MaxHealth;
    static FGameplayAttribute GetMaxHealthAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MaxHealth))); }
    float GetMaxHealth() const { IC_TRACE_FUNCTION(); return MaxHealth.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData Mana;
    static FGameplayAttribute GetManaAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, Mana))); }
    float GetMana() const { IC_TRACE_FUNCTION(); return Mana.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData MaxMana;
    static FGameplayAttribute GetMaxManaAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MaxMana))); }
    float GetMaxMana() const { IC_TRACE_FUNCTION(); return MaxMana.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData PhysicalAttack;
    static FGameplayAttribute GetPhysicalAttackAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, PhysicalAttack))); }
    float GetPhysicalAttack() const { IC_TRACE_FUNCTION(); return PhysicalAttack.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData MagicAttack;
    static FGameplayAttribute GetMagicAttackAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MagicAttack))); }
    float GetMagicAttack() const { IC_TRACE_FUNCTION(); return MagicAttack.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData Defense;
    static FGameplayAttribute GetDefenseAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, Defense))); }
    float GetDefense() const { IC_TRACE_FUNCTION(); return Defense.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData AttackSpeed;
    static FGameplayAttribute GetAttackSpeedAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, AttackSpeed))); }
    float GetAttackSpeed() const { IC_TRACE_FUNCTION(); return AttackSpeed.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData MoveSpeed;
    static FGameplayAttribute GetMoveSpeedAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MoveSpeed))); }
    float GetMoveSpeed() const { IC_TRACE_FUNCTION(); return MoveSpeed.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData CriticalChance;
    static FGameplayAttribute GetCriticalChanceAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, CriticalChance))); }
    float GetCriticalChance() const { IC_TRACE_FUNCTION(); return CriticalChance.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData CriticalMultiplier;
    static FGameplayAttribute GetCriticalMultiplierAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, CriticalMultiplier))); }
    float GetCriticalMultiplier() const { IC_TRACE_FUNCTION(); return CriticalMultiplier.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat")
    FGameplayAttributeData AutoDodgeChance;
    static FGameplayAttribute GetAutoDodgeChanceAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, AutoDodgeChance))); }
    float GetAutoDodgeChance() const { IC_TRACE_FUNCTION(); return AutoDodgeChance.GetCurrentValue(); }
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
};

UCLASS()
class ISEKAICLASS_API UICPrimaryAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UICPrimaryAttributeSet();
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary")
    FGameplayAttributeData STR;
    static FGameplayAttribute GetSTRAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, STR))); }
    float GetSTR() const { IC_TRACE_FUNCTION(); return STR.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary")
    FGameplayAttributeData DEX;
    static FGameplayAttribute GetDEXAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, DEX))); }
    float GetDEX() const { IC_TRACE_FUNCTION(); return DEX.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary")
    FGameplayAttributeData INT;
    static FGameplayAttribute GetINTAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, INT))); }
    float GetINT() const { IC_TRACE_FUNCTION(); return INT.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary")
    FGameplayAttributeData CON;
    static FGameplayAttribute GetCONAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, CON))); }
    float GetCON() const { IC_TRACE_FUNCTION(); return CON.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary")
    FGameplayAttributeData CHA;
    static FGameplayAttribute GetCHAAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, CHA))); }
    float GetCHA() const { IC_TRACE_FUNCTION(); return CHA.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary")
    FGameplayAttributeData LUK;
    static FGameplayAttribute GetLUKAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, LUK))); }
    float GetLUK() const { IC_TRACE_FUNCTION(); return LUK.GetCurrentValue(); }
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
};

UCLASS()
class ISEKAICLASS_API UICSurvivalAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UICSurvivalAttributeSet();
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Survival")
    FGameplayAttributeData SP;
    static FGameplayAttribute GetSPAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICSurvivalAttributeSet, SP))); }
    float GetSP() const { IC_TRACE_FUNCTION(); return SP.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Survival")
    FGameplayAttributeData MaxSP;
    static FGameplayAttribute GetMaxSPAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICSurvivalAttributeSet, MaxSP))); }
    float GetMaxSP() const { IC_TRACE_FUNCTION(); return MaxSP.GetCurrentValue(); }
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
};

