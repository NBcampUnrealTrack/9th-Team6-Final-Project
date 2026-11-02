#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ICNPCHomeBehaviorComponent.generated.h"

class AICAIController;
class AICNPCBase;

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

    UFUNCTION(BlueprintPure, Category="IC|NPC|Home") bool IsHomeBehaviorActive() const { return bActive; }
    UFUNCTION(BlueprintPure, Category="IC|NPC|Home") EICNPCHomeAction GetCurrentAction() const { return CurrentAction; }
    UFUNCTION(BlueprintPure, Category="IC|NPC|Home") FName GetResolvedHomeAnchorID() const { return ResolvedHomeAnchorID; }

    /** 현재 시간표/상태에서 집 자율 행동을 실행할 수 있는지 확인한다. StateTree 조건용. */
    bool CanRunHomeBehavior();
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

    FName ResolvedHomeAnchorID;
    EICNPCHomeAction CurrentAction = EICNPCHomeAction::None;
    EICNPCHomeAction PreviousAction = EICNPCHomeAction::None;
    double NextDecisionAt = 0.0;
    bool bActive = false;
};
