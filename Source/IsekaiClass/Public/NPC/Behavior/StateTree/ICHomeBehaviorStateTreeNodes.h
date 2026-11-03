#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "ICHomeBehaviorStateTreeNodes.generated.h"

class AAIController;

USTRUCT()
struct FICHomeBehaviorNodeInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category=Context)
    TObjectPtr<AAIController> AIController = nullptr;
};

/** 현재 Scheduled NPC가 집 앵커를 목적지로 둔 비근무 시간인지 확인한다. */
USTRUCT(meta=(DisplayName="IC Home: Can Run Behavior (집 자율 행동 가능)", Category="IC|NPC|Home"))
struct ISEKAICLASS_API FICSTCondition_HomeBehaviorCanRun : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICHomeBehaviorNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

    UPROPERTY(EditAnywhere, Category=Parameter) bool bInvert = false;
};

/** Home Leisure 상태의 유일한 이동 태스크로 사용한다. */
USTRUCT(meta=(DisplayName="IC Home: Run Behavior (집 자율 행동 실행)", Category="IC|NPC|Home"))
struct ISEKAICLASS_API FICSTTask_HomeBehavior : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    FICSTTask_HomeBehavior();
    using FInstanceDataType = FICHomeBehaviorNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/** 집 Leisure 오브젝트를 랜덤 시간 동안 사용한 뒤 Succeeded를 반환해 Acquire로 다시 보낸다. */
USTRUCT(meta=(DisplayName="IC Home: Object Use Timer (집 오브젝트 사용 시간)", Category="IC|NPC|Home"))
struct ISEKAICLASS_API FICSTTask_HomeObjectUseTimer : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    FICSTTask_HomeObjectUseTimer();
    using FInstanceDataType = FICHomeBehaviorNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
