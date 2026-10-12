#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Debug/ICLog.h"
#include "ICAttributeSets.generated.h"

UCLASS(meta=(ToolTip="Combat GAS 실행 능력치 집합입니다. 지속 상태의 원본은 WorldState이며 로드·스폰 시 복원하고 저장·제거 전에 반영합니다."))
class ISEKAICLASS_API UICCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UICCombatAttributeSet();
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="현재 HP. 피해·회복은 GameplayEffect로 변경합니다."))
    FGameplayAttributeData Health;
    static FGameplayAttribute GetHealthAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, Health))); }
    float GetHealth() const { IC_TRACE_FUNCTION(); return Health.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="최대 HP. 최대값 증가로 현재 HP가 회복되지 않습니다."))
    FGameplayAttributeData MaxHealth;
    static FGameplayAttribute GetMaxHealthAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MaxHealth))); }
    float GetMaxHealth() const { IC_TRACE_FUNCTION(); return MaxHealth.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="현재 MP. 스킬 첫 효과 확정 시 비용을 차감합니다."))
    FGameplayAttributeData Mana;
    static FGameplayAttribute GetManaAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, Mana))); }
    float GetMana() const { IC_TRACE_FUNCTION(); return Mana.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="최대 MP. 현재값은 0부터 최대값 사이로 제한합니다."))
    FGameplayAttributeData MaxMana;
    static FGameplayAttribute GetMaxManaAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MaxMana))); }
    float GetMaxMana() const { IC_TRACE_FUNCTION(); return MaxMana.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="최종 물리 공격력. 장비 고정값은 한 번만 적용합니다."))
    FGameplayAttributeData PhysicalAttack;
    static FGameplayAttribute GetPhysicalAttackAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, PhysicalAttack))); }
    float GetPhysicalAttack() const { IC_TRACE_FUNCTION(); return PhysicalAttack.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="최종 마법 공격력과 치유 기반입니다."))
    FGameplayAttributeData MagicAttack;
    static FGameplayAttribute GetMagicAttackAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MagicAttack))); }
    float GetMagicAttack() const { IC_TRACE_FUNCTION(); return MagicAttack.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="최종 방어력. 피해 계산은 100/(100+방어력)을 사용합니다."))
    FGameplayAttributeData Defense;
    static FGameplayAttribute GetDefenseAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, Defense))); }
    float GetDefense() const { IC_TRACE_FUNCTION(); return Defense.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="공격속도 배율. 공격 모션·간격에만 적용합니다."))
    FGameplayAttributeData AttackSpeed;
    static FGameplayAttribute GetAttackSpeedAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, AttackSpeed))); }
    float GetAttackSpeed() const { IC_TRACE_FUNCTION(); return AttackSpeed.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="최종 달리기 속도. 단위는 cm/s입니다."))
    FGameplayAttributeData MoveSpeed;
    static FGameplayAttribute GetMoveSpeedAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, MoveSpeed))); }
    float GetMoveSpeed() const { IC_TRACE_FUNCTION(); return MoveSpeed.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="치명타 확률. 0~1 비율이며 최종 상한은 설정 DT를 따릅니다."))
    FGameplayAttributeData CriticalChance;
    static FGameplayAttribute GetCriticalChanceAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, CriticalChance))); }
    float GetCriticalChance() const { IC_TRACE_FUNCTION(); return CriticalChance.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="치명타 피해 배율입니다."))
    FGameplayAttributeData CriticalMultiplier;
    static FGameplayAttribute GetCriticalMultiplierAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, CriticalMultiplier))); }
    float GetCriticalMultiplier() const { IC_TRACE_FUNCTION(); return CriticalMultiplier.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Combat", meta=(ToolTip="자동 회피 확률. 성공해도 이동이나 추가 무적은 발생하지 않습니다."))
    FGameplayAttributeData AutoDodgeChance;
    static FGameplayAttribute GetAutoDodgeChanceAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICCombatAttributeSet, AutoDodgeChance))); }
    float GetAutoDodgeChance() const { IC_TRACE_FUNCTION(); return AutoDodgeChance.GetCurrentValue(); }
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
};

UCLASS(meta=(ToolTip="Primary GAS 실행 능력치 집합입니다. 지속 상태의 원본은 WorldState이며 로드·스폰 시 복원하고 저장·제거 전에 반영합니다."))
class ISEKAICLASS_API UICPrimaryAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UICPrimaryAttributeSet();
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary", meta=(ToolTip="유효 STR 주 스탯. 성장·해금용 정수 기본값은 WorldState에 유지하고 장비·효과 보정은 실행 값에만 반영합니다."))
    FGameplayAttributeData STR;
    static FGameplayAttribute GetSTRAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, STR))); }
    float GetSTR() const { IC_TRACE_FUNCTION(); return STR.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary", meta=(ToolTip="유효 DEX 주 스탯. 성장·해금용 정수 기본값은 WorldState에 유지하고 장비·효과 보정은 실행 값에만 반영합니다."))
    FGameplayAttributeData DEX;
    static FGameplayAttribute GetDEXAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, DEX))); }
    float GetDEX() const { IC_TRACE_FUNCTION(); return DEX.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary", meta=(ToolTip="유효 INT 주 스탯. 성장·해금용 정수 기본값은 WorldState에 유지하고 장비·효과 보정은 실행 값에만 반영합니다."))
    FGameplayAttributeData INT;
    static FGameplayAttribute GetINTAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, INT))); }
    float GetINT() const { IC_TRACE_FUNCTION(); return INT.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary", meta=(ToolTip="유효 CON 주 스탯. 성장·해금용 정수 기본값은 WorldState에 유지하고 장비·효과 보정은 실행 값에만 반영합니다."))
    FGameplayAttributeData CON;
    static FGameplayAttribute GetCONAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, CON))); }
    float GetCON() const { IC_TRACE_FUNCTION(); return CON.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary", meta=(ToolTip="유효 CHA 주 스탯. 성장·해금용 정수 기본값은 WorldState에 유지하고 장비·효과 보정은 실행 값에만 반영합니다."))
    FGameplayAttributeData CHA;
    static FGameplayAttribute GetCHAAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, CHA))); }
    float GetCHA() const { IC_TRACE_FUNCTION(); return CHA.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Primary", meta=(ToolTip="유효 LUK 주 스탯. 성장·해금용 정수 기본값은 WorldState에 유지하고 장비·효과 보정은 실행 값에만 반영합니다."))
    FGameplayAttributeData LUK;
    static FGameplayAttribute GetLUKAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICPrimaryAttributeSet, LUK))); }
    float GetLUK() const { IC_TRACE_FUNCTION(); return LUK.GetCurrentValue(); }
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
};

UCLASS(meta=(ToolTip="Survival GAS 실행 능력치 집합입니다. 지속 상태의 원본은 WorldState이며 로드·스폰 시 복원하고 저장·제거 전에 반영합니다."))
class ISEKAICLASS_API UICSurvivalAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UICSurvivalAttributeSet();
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Survival", meta=(ToolTip="플레이어 현재 SP. PlayClock 10초당 1 감소하며 화면은 정수로 표시합니다."))
    FGameplayAttributeData SP;
    static FGameplayAttribute GetSPAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICSurvivalAttributeSet, SP))); }
    float GetSP() const { IC_TRACE_FUNCTION(); return SP.GetCurrentValue(); }
    UPROPERTY(BlueprintReadOnly, Category="IC|GAS|Survival", meta=(ToolTip="플레이어 최대 SP. 영입 NPC·몬스터에는 생존 AttributeSet을 생성하지 않습니다."))
    FGameplayAttributeData MaxSP;
    static FGameplayAttribute GetMaxSPAttribute() { IC_TRACE_FUNCTION(); return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UICSurvivalAttributeSet, MaxSP))); }
    float GetMaxSP() const { IC_TRACE_FUNCTION(); return MaxSP.GetCurrentValue(); }
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
};

