#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ICNPCHomeBehaviorComponent.generated.h"

class AICAIController;
class AICNPCBase;
class UICLifeObjectUserComponent;

/** NPC가 집 앵커에 머무는 동안 수행할 가벼운 자율 행동. */
UENUM(BlueprintType)
enum class EICNPCHomeAction : uint8
{
    None,
    Wander,
    Idle,
    LookAround
};

/** 집 행동 하나의 출현 빈도와 지속 시간 설정. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNPCHomeActionConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home")
    EICNPCHomeAction Action = EICNPCHomeAction::Wander;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0"))
    float Weight = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0.1", Units="s"))
    float MinDurationSeconds = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0.1", Units="s"))
    float MaxDurationSeconds = 16.f;
};

/** 집 여가 시간에 순환할 생활 오브젝트 활동 하나. 같은 태그는 직후에 다시 선택하지 않는다. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNPCHomeObjectActivityConfig
{
    GENERATED_BODY()

    /** 생활 오브젝트의 Activity Tags 중 하나. 예: Sit, Work.Farm, Work.Cook, Leisure.View. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(Categories="IC.Life.Activity"))
    FGameplayTag ActivityTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0"))
    float Weight = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0.1", Units="s"))
    float MinUseSeconds = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0.1", Units="s"))
    float MaxUseSeconds = 20.f;
};

/**
 * StateTree/생활 오브젝트와 독립적인 "집에서 빈둥거리기" 레이어.
 *
 * 현재 일과의 목적지가 HomeAnchorID이고 Work 구간이 아닐 때만 일반 스케줄 이동을 대체한다.
 * 나중에 Wander/Idle/LookAround 중 하나를 LifeObject 상호작용으로 바꾸어도 선택·시간 관리 규칙은 유지된다.
 */
UCLASS(ClassGroup=(IC), meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICNPCHomeBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UICNPCHomeBehaviorComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 비어 있으면 NPC의 InitialAnchorID를 사용한다. 집 앵커 ID를 명시하는 편을 권장한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home")
    FName HomeAnchorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0", Units="cm"))
    float WanderRadiusCm = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="25", Units="cm"))
    float MoveAcceptanceRadiusCm = 80.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0.1", Units="s"))
    float DecisionIntervalSeconds = .5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home")
    TArray<FICNPCHomeActionConfig> ActionConfigs;

    /** 집 Idle 시간에 순환할 생활 오브젝트 활동 풀. 비어 있으면 현재 시간표 태그 탐색을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home")
    TArray<FICNPCHomeObjectActivityConfig> ObjectActivityPool;

    /**
     * 집 오브젝트 사용을 마친 뒤 같은 오브젝트를 다시 고르지 않는 시간(PlayClock 초).
     * 쓸 수 있는 오브젝트가 그것뿐이면 이 시간 동안 오브젝트 없이 앵커 대기/자율 행동을 한 뒤 다시 고른다. 0이면 끈다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC|Home", meta=(ClampMin="0", Units="s"))
    float ObjectReuseCooldownSeconds = 25.f;

    UFUNCTION(BlueprintPure, Category="IC|NPC|Home") bool IsHomeBehaviorActive() const { return bActive; }
    UFUNCTION(BlueprintPure, Category="IC|NPC|Home") EICNPCHomeAction GetCurrentAction() const { return CurrentAction; }
    UFUNCTION(BlueprintPure, Category="IC|NPC|Home") FName GetResolvedHomeAnchorID() const { return ResolvedHomeAnchorID; }

    /** 현재 시간표/상태에서 집 자율 행동을 실행할 수 있는지 확인한다. StateTree 조건용. */
    bool CanRunHomeBehavior();
    /** 현재가 집 Idle 구간인지. 이동/수면/근무 구간에는 false다. */
    bool ShouldUseHomeObjectPool();
    /** 가중치·직전 활동 회피를 적용해 사용 가능한 집 오브젝트 활동을 하나 고르고 후보를 만든다. */
    int32 FindNextHomeObjectCandidates(UICLifeObjectUserComponent& User);
    /** Occupy 진입 때 호출한다. 선택된 집 활동의 사용 시간을 시작한다. */
    void BeginHomeObjectUse();
    bool IsHomeObjectUseActive() const { return bHomeObjectUseActive; }
    bool IsHomeObjectUseFinished() const;
    void EndHomeObjectUse();
    /** HomeAutonomous에서 장시간 실제 이동 진척이 없었을 때 한 번 true가 된다. */
    bool ConsumeHomeObjectRetryRequest();
    /** AICAIController의 일반 스케줄 결정 루프에서 호출한다. */
    bool TickBehavior(AICAIController& Controller);
    void StopBehavior(AICAIController* Controller = nullptr);

private:
    AICNPCBase* GetNPC() const;
    bool CanRun(FTransform& OutHomeTransform);
    void StartNextAction(AICAIController& Controller, const FTransform& HomeTransform, double Now);
    EICNPCHomeAction ChooseAction() const;
    void StartIdleAction(AICAIController& Controller, const FTransform& HomeTransform);
    bool FindWanderGoal(const FVector& Center, FVector& OutGoal) const;
    const FICNPCHomeObjectActivityConfig* FindObjectActivity(FGameplayTag ActivityTag) const;
    double GetPlaySecond() const;
    double GetMaxHomeObjectUseSeconds() const;
    void UpdateStationaryWatch(double Now);

    FName ResolvedHomeAnchorID;
    EICNPCHomeAction CurrentAction = EICNPCHomeAction::None;
    EICNPCHomeAction PreviousAction = EICNPCHomeAction::None;
    double NextDecisionAt = 0.0;
    double HomeObjectUseEndsAt = 0.0;
    /** 마지막 집 오브젝트 사용이 끝난 시각. 재사용 쿨다운 계산용. */
    double LastHomeObjectEndedAt = -1.0;
    FGameplayTag CurrentHomeObjectActivity;
    FGameplayTag PreviousHomeObjectActivity;
    FVector LastObservedLocation = FVector::ZeroVector;
    double LastMovementAt = 0.0;
    bool bActive = false;
    bool bHomeObjectUseActive = false;
    bool bHomeObjectRetryRequested = false;
};
