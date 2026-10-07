#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ICGameplayAbility.generated.h"

UCLASS(meta=(ToolTip="DT와 입력 요청을 실행하는 공통 GAS 능력입니다. ASC의 실행 ID·첫 효과 확정·타격 중복 검사 경로를 사용하며 종료와 취소에서 상태 태그·무적·남은 타격을 정리합니다."))
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
