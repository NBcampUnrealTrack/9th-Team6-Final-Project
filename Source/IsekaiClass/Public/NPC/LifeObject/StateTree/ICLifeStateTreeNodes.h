#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"
#include "ICLifeStateTreeNodes.generated.h"

class AAIController;

/*
 * NPC 생활 오브젝트용 StateTree 노드 모음.
 *
 * 모든 노드는 Schema가 "StateTree AI Component"인 StateTree에서 쓴다.
 * 인스턴스 데이터의 AIController(Category=Context)는 에디터가 자동으로 연결하므로 따로 바인딩할 필요가 없다.
 * 실제 로직은 NPC의 UICLifeObjectUserComponent에 있고, 노드는 그 함수를 호출하고 성공/실패만 돌려준다.
 *
 * 권장 트리 구조와 전환 설정은 Docs/NPC_LifeObject_Guide.md 참고.
 */

// ============================================================ 공통 인스턴스 데이터

USTRUCT()
struct FICLifeNodeInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category=Context)
    TObjectPtr<AAIController> AIController = nullptr;
};

// ============================================================ 조건

/** 대화·영입·Downed 등으로 생활 행동을 멈춰야 하면 true. */
USTRUCT(meta=(DisplayName="IC Life: Should Hold (생활 중단 필요)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTCondition_LifeShouldHold : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

    UPROPERTY(EditAnywhere, Category=Parameter) bool bInvert = false;
};

/** 현재 일과가 오브젝트를 필요로 하는 구간(수면·작업·앉기 등)이면 true. */
USTRUCT(meta=(DisplayName="IC Life: Routine Wants Object (오브젝트 필요 구간)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTCondition_LifeWantsObject : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

    UPROPERTY(EditAnywhere, Category=Parameter) bool bInvert = false;
};

/** 예약한 슬롯에 도착했으면 true. */
USTRUCT(meta=(DisplayName="IC Life: Is At Slot (도착 확인)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTCondition_LifeIsAtSlot : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

    /** 컴포넌트의 ArrivalRadiusCm에 더하는 여유 거리. */
    UPROPERTY(EditAnywhere, Category=Parameter, meta=(ClampMin="0", Units="cm")) float ExtraToleranceCm = 30.f;
    UPROPERTY(EditAnywhere, Category=Parameter) bool bInvert = false;
};

// ============================================================ 태스크

/** 생활 중단 상태. 들어올 때 점유를 모두 해제하고, 중단 사유가 사라지면 Succeeded. */
USTRUCT(meta=(DisplayName="IC Life: Hold (중단·대기)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeHold : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FICLifeWatchRoutineInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category=Context) TObjectPtr<AAIController> AIController = nullptr;
    UPROPERTY(EditAnywhere, Category=Output) FName SegmentID;
    UPROPERTY(EditAnywhere, Category=Output) FGameplayTag ActivityTag;
};

/**
 * 일과 감시. 상위 상태(Routine)에 둔다.
 * 시간표 구간이 바뀌거나, 외부에서 재계산을 요청하거나(대화 종료·시간 점프), 중단이 필요해지면 Succeeded.
 * → Routine 상태의 "On State Completed → Root" 전환으로 일과를 처음부터 다시 고른다.
 */
USTRUCT(meta=(DisplayName="IC Life: Watch Routine (일과 감시·중단)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeWatchRoutine : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeWatchRoutineInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FICLifeFindInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category=Context) TObjectPtr<AAIController> AIController = nullptr;
    UPROPERTY(EditAnywhere, Category=Output) int32 CandidateCount = 0;
};

/** 대상 Smart Object 탐색. 후보가 있으면 즉시 Succeeded, 없으면 Failed. */
USTRUCT(meta=(DisplayName="IC Life: Find Object (대상 탐색)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeFindObject : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    FICSTTask_LifeFindObject();
    using FInstanceDataType = FICLifeFindInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/**
 * 슬롯 예약 + 유지. 이동·점유 상태들의 "부모" 상태에 둔다.
 * 들어올 때 예약하고(실패 시 Failed), 예약이 살아 있는 동안 Running,
 * 이 상태를 벗어나면(어떤 이유든) 예약을 해제한다.
 */
USTRUCT(meta=(DisplayName="IC Life: Reserve Slot (슬롯 예약·유지)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeReserveSlot : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    FICSTTask_LifeReserveSlot();
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

    /** 상태를 벗어날 때 예약을 해제한다. 특별한 이유가 없으면 켜 둔다. */
    UPROPERTY(EditAnywhere, Category=Parameter) bool bReleaseOnExit = true;
};

USTRUCT()
struct FICLifeMoveInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category=Context) TObjectPtr<AAIController> AIController = nullptr;
    UPROPERTY() double StartPlaySecond = 0.0;
};

/** 예약한 슬롯 위치까지 NavMesh 이동. 도착하면 Succeeded, 경로 실패·시간 초과면 Failed. */
USTRUCT(meta=(DisplayName="IC Life: Move To Slot (NavMesh 이동)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeMoveToSlot : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeMoveInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

    UPROPERTY(EditAnywhere, Category=Parameter, meta=(ClampMin="0", Units="cm")) float AcceptanceRadiusCm = 50.f;
    /** 이 시간(PlayClock 초) 안에 도착하지 못하면 실패로 본다. */
    UPROPERTY(EditAnywhere, Category=Parameter, meta=(ClampMin="1", Units="s")) float TimeoutPlaySeconds = 30.f;
};

/** 슬롯 점유 시작(위치·방향 맞춤). 예약이 살아 있는 동안 Running, 벗어나면 점유를 끝낸다. */
USTRUCT(meta=(DisplayName="IC Life: Occupy Slot (점유 시작·유지)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeOccupySlot : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    FICSTTask_LifeOccupySlot();
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/** 지정 애니메이션 재생·유지. 애니메이션이 없으면 Print만 하고 계속 Running. 벗어나면 정지. */
USTRUCT(meta=(DisplayName="IC Life: Play Life Animation (애니 재생·유지)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifePlayAnimation : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    FICSTTask_LifePlayAnimation();
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/** 슬롯 점유 해제(즉시 Succeeded). 보통은 Reserve Slot이 자동으로 해제하므로 필요할 때만 쓴다. */
USTRUCT(meta=(DisplayName="IC Life: Release Slot (점유 해제)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeReleaseSlot : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    FICSTTask_LifeReleaseSlot();
    using FInstanceDataType = FICLifeNodeInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

    UPROPERTY(EditAnywhere, Category=Parameter) FName Reason = TEXT("StateTree");
};

USTRUCT()
struct FICLifeRecoverInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category=Context) TObjectPtr<AAIController> AIController = nullptr;
    UPROPERTY() double StartPlaySecond = 0.0;
};

/**
 * 이동 실패 복구.
 * - 화면 안: RepathDelay 후 Succeeded → 다시 Move To Slot(재탐색).
 * - 화면 밖: OffscreenRecovery(기획 5초) 후 목적지 앵커로 순간이동하고 Succeeded.
 * - 실패 횟수가 MaxMoveFailures를 넘으면 예약을 포기하고 Failed → 대체 행동.
 */
USTRUCT(meta=(DisplayName="IC Life: Recover Path (이동 실패 복구)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeRecoverPath : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeRecoverInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

    UPROPERTY(EditAnywhere, Category=Parameter, meta=(ClampMin="0", Units="s")) float RepathDelayPlaySeconds = 1.f;
    UPROPERTY(EditAnywhere, Category=Parameter, meta=(ClampMin="0", Units="s")) float OffscreenRecoveryPlaySeconds = 5.f;
};

USTRUCT()
struct FICLifeWaitInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category=Context) TObjectPtr<AAIController> AIController = nullptr;
    UPROPERTY() double NextRetryPlaySecond = 0.0;
    UPROPERTY() bool bArrived = false;
};

/**
 * 대체 행동: 목적지 앵커로 걸어가 대기한다(수면 구간이면 바닥 수면).
 * 오브젝트가 필요한 구간이면 RetryInterval마다 다시 찾아보고, 빈 슬롯이 생기면 Succeeded.
 * 오브젝트가 필요 없는 구간(Idle/Move)에서는 계속 Running(구간이 끝나면 Watch Routine이 끝낸다).
 */
USTRUCT(meta=(DisplayName="IC Life: Wait At Anchor (대체 행동·재탐색)", Category="IC|Life"))
struct ISEKAICLASS_API FICSTTask_LifeWaitAtAnchor : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FICLifeWaitInstanceData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

    UPROPERTY(EditAnywhere, Category=Parameter) bool bRetryObjects = true;
};
