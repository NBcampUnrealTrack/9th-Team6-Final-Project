#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SmartObjectRuntime.h"
#include "NPC/LifeObject/ICLifeObjectTypes.h"
#include "ICLifeObjectUserComponent.generated.h"

class AICLifeObjectActor;
class AICNPCBase;
class AAIController;
class UAnimMontage;

/**
 * NPC 한 명의 생활 오브젝트 사용 상태를 가진 컴포넌트(AICNPCBase에 기본 부착).
 *
 * 예약 핸들은 이 컴포넌트 하나만 가진다. 대화·시간 점프·로드·영입·Downed·파괴·언로드 등
 * 어떤 경로로 행동이 끝나도 ReleaseAll()만 부르면 되고, 여러 번 불러도 안전하다.
 * StateTree 태스크는 이 컴포넌트의 함수를 얇게 감싼 것이다.
 */
UCLASS(ClassGroup=(IC), meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICLifeObjectUserComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UICLifeObjectUserComponent();
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ---------- 설정 ----------

    /** 목적지 앵커 기준 탐색 반경. 앵커에 연결된 오브젝트는 반경과 무관하게 후보가 된다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life", meta=(ClampMin="0", Units="cm"))
    float SearchRadiusCm = 2500.f;

    /** 슬롯 도착 판정 거리(수평). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life", meta=(ClampMin="10", Units="cm"))
    float ArrivalRadiusCm = 120.f;

    /** 대기 중 다시 오브젝트를 찾아보는 간격(PlayClock 초). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life", meta=(ClampMin="0.5", Units="s"))
    float RetryIntervalPlaySeconds = 5.f;

    /** 같은 슬롯으로 이동 실패가 이 횟수를 넘으면 그 슬롯을 포기하고 대체 행동으로 간다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life", meta=(ClampMin="1"))
    int32 MaxMoveFailures = 3;

    /** 화면에 Print String으로 생활 행동을 표시한다(개발 빌드 전용). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life|Debug")
    bool bPrintToScreen = true;

    // ---------- 일과 ----------

    /** 현재 GameClock으로 일과를 다시 계산한다. 구간이 바뀌었으면 true. bForce가 아니면 게임 분이 바뀔 때만 계산한다. */
    bool RefreshRoutine(bool bForce = false);
    UFUNCTION(BlueprintPure, Category="IC|Life") FICLifeRoutine GetRoutine() const { return Routine; }
    const FICLifeRoutine& GetRoutineRef() const { return Routine; }

    /** 대화·영입·Downed 등으로 생활 행동을 멈춰야 하는지. */
    UFUNCTION(BlueprintPure, Category="IC|Life") bool ShouldHoldLife() const;

    /** 외부에서 "일과를 처음부터 다시 계산하라"고 요청한다(대화 종료, 시간 점프 등). */
    UFUNCTION(BlueprintCallable, Category="IC|Life") void RequestReschedule(FName Reason);
    bool HasPendingReschedule() const { return bRescheduleRequested; }
    FName ConsumeRescheduleRequest();

    // ---------- 탐색·예약 ----------

    int32 FindCandidates();
    /**
     * 일과 태그를 건드리지 않고, 호출자가 지정한 활동 태그로 후보를 찾는다(집 여가 활동 풀용).
     * ExcludeObject를 주면 그 오브젝트의 슬롯은 대안이 없어도 후보에서 완전히 뺀다(재사용 쿨다운용).
     */
    int32 FindCandidatesForActivity(FGameplayTag ActivityTag, const AICLifeObjectActor* ExcludeObject = nullptr);
    const TArray<FICLifeObjectCandidate>& GetCandidates() const { return Candidates; }
    /** 후보를 점수순으로 돌며 첫 번째로 예약에 성공한 슬롯을 잡는다. */
    bool ClaimBestCandidate();
    UFUNCTION(BlueprintPure, Category="IC|Life") bool HasClaim() const;
    UFUNCTION(BlueprintPure, Category="IC|Life") AICLifeObjectActor* GetClaimedObject() const;
    /** 직전에 실제 사용을 시작한 오브젝트. 대안이 있으면 다음 후보에서 제외한다. */
    AICLifeObjectActor* GetLastUsedObject() const { return LastUsedObject.Get(); }
    int32 GetClaimedSlotIndex() const { return ClaimedSlotIndex; }
    const FSmartObjectClaimHandle& GetClaimHandle() const { return Claim; }
    bool GetClaimTransform(FTransform& OutTransform) const;
    bool IsAtClaimedSlot(float ExtraToleranceCm = 0.f) const;

    // ---------- 점유·연출 ----------

    /** 슬롯을 점유 상태로 바꾸고 위치를 맞춘다. */
    bool BeginUse();
    void EndUse();
    UFUNCTION(BlueprintPure, Category="IC|Life") bool IsUsing() const { return bUsing; }

    bool PlayPresentation();
    void StopPresentation();

    /** 수면 중인지(침대 사용 또는 바닥 수면). 수면 중 NPC는 대화 후보에서 제외한다. */
    UFUNCTION(BlueprintPure, Category="IC|Life") bool IsSleeping() const;
    void SetFloorSleeping(bool bSleeping) { bFloorSleeping = bSleeping; }

    // ---------- 해제 ----------

    /** 예약·점유·연출을 모두 정리한다. 여러 번 불러도 안전하다. */
    UFUNCTION(BlueprintCallable, Category="IC|Life") void ReleaseAll(FName Reason);

    /** Smart Object 시스템이 슬롯을 무효화했을 때(오브젝트 언로드 등). */
    void HandleSlotInvalidated(const FSmartObjectClaimHandle& InvalidatedClaim, ESmartObjectSlotState State);
    /** 서브시스템이 오브젝트 소실/플레이어 사용 등으로 예약을 빼앗을 때. */
    void NotifyClaimRevoked(FName Reason);
    bool OwnsClaim(const FSmartObjectClaimHandle& Handle) const { return Claim.IsValid() && Claim == Handle; }

    // ---------- 이동 실패 ----------

    int32 NoteMoveFailure() { return ++MoveFailureCount; }
    void ResetMoveFailures() { MoveFailureCount = 0; }
    int32 GetMoveFailureCount() const { return MoveFailureCount; }

    // ---------- 정보 ----------

    UFUNCTION(BlueprintPure, Category="IC|Life") FName GetUserID() const;
    FGameplayTag GetOccupationTag() const;
    FName GetRegionID() const;
    EICLifeFallback GetFallback() const;
    AICNPCBase* GetNPC() const;
    AAIController* GetAIController() const;
    bool ResolveAnchor(FName AnchorID, FTransform& OutTransform) const;
    double GetPlaySecond() const;
    void Say(const FString& Message, const FColor& Color = FColor::Cyan) const;

private:
    FICLifeRoutine Routine;
    TArray<FICLifeObjectCandidate> Candidates;
    FSmartObjectClaimHandle Claim;
    TWeakObjectPtr<AICLifeObjectActor> ClaimedObject;
    TWeakObjectPtr<AICLifeObjectActor> LastUsedObject;
    int32 ClaimedSlotIndex = INDEX_NONE;
    TWeakObjectPtr<UAnimMontage> PlayingMontage;
    FName PendingRescheduleReason;
    int32 MoveFailureCount = 0;
    int32 LastRoutineGameMinute = INDEX_NONE;
    bool bUsing = false;
    bool bRestoreOrientToMovement = false;
    bool bOccupiedMarked = false;
    bool bCollisionIgnored = false;
    bool bFloorSleeping = false;
    bool bRescheduleRequested = false;
    bool bReleasing = false;
};
