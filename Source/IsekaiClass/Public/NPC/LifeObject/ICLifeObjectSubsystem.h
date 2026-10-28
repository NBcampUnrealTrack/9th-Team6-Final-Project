#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SmartObjectRuntime.h"
#include "NPC/LifeObject/ICLifeObjectTypes.h"
#include "ICLifeObjectSubsystem.generated.h"

class AICLifeObjectActor;
class UICLifeObjectUserComponent;
class USmartObjectSubsystem;
class USmartObjectBehaviorDefinition;

/** 슬롯 하나의 현재 사용자 기록(디버그·주인 없는 예약 정리용). 저장하지 않는다. */
USTRUCT()
struct FICLifeSlotOccupant
{
    GENERATED_BODY()

    UPROPERTY() TWeakObjectPtr<AICLifeObjectActor> Object;
    UPROPERTY() TWeakObjectPtr<UICLifeObjectUserComponent> User;
    UPROPERTY() FName UserID;
    UPROPERTY() int32 SlotIndex = INDEX_NONE;
    FSmartObjectClaimHandle Claim;
    bool bOccupied = false;
    double SinceRealSeconds = 0.0;
};

/**
 * 현재 월드의 생활 오브젝트 목록과 슬롯 예약/점유/해제를 한곳에서 관리한다.
 *
 * Smart Object API 호출은 모두 이 클래스 안에서만 한다(엔진 버전이 바뀌면 여기만 고치면 된다).
 * 1초마다 "사용자가 사라졌는데 남아 있는 예약"을 찾아 강제로 해제하고 경고 로그를 남긴다.
 */
UCLASS()
class ISEKAICLASS_API UICLifeObjectSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableWhenPaused() const override { return true; }

    static UICLifeObjectSubsystem* Get(const UObject* WorldContext);

    void RegisterObject(AICLifeObjectActor* Object);
    void UnregisterObject(AICLifeObjectActor* Object);

    /** 일과에 맞는 후보를 점수순(낮을수록 우선)으로 모은다. 지금 예약할 수 없는 슬롯은 제외한다. */
    int32 GatherCandidates(const UICLifeObjectUserComponent& User, const FICLifeRoutine& Routine, float SearchRadiusCm, TArray<FICLifeObjectCandidate>& OutCandidates) const;

    /** 슬롯 예약. 성공하면 OutClaim이 유효하다. 동시에 두 NPC가 요청해도 한 명만 성공한다. */
    bool TryClaim(UICLifeObjectUserComponent& User, AICLifeObjectActor& Object, int32 SlotIndex, FSmartObjectClaimHandle& OutClaim);

    /** 예약을 점유로 바꾼다. Behavior Definition이 있으면 반환한다(없어도 예약은 유지). */
    const USmartObjectBehaviorDefinition* MarkOccupied(const FSmartObjectClaimHandle& Claim, bool& bOutMarked);

    /** 예약/점유 해제. 여러 번 불러도 안전하다. */
    void Release(const FSmartObjectClaimHandle& Claim, FName Reason, bool bFromInvalidation = false);

    bool GetSlotTransform(const FSmartObjectClaimHandle& Claim, FTransform& OutTransform) const;
    bool IsClaimAlive(const FSmartObjectClaimHandle& Claim) const;

    /** 플레이어 침대 사용. NPC가 쓰고 있으면 실패한다. 성공하면 거리가 멀어질 때까지 NPC 예약을 막는다. */
    bool TryLockForPlayer(AICLifeObjectActor& Object, AActor* Player, FText& OutFailure);
    void UnlockForPlayer(AICLifeObjectActor& Object);

    void GetOccupantsOf(const AICLifeObjectActor& Object, TArray<const FICLifeSlotOccupant*>& OutOccupants) const;
    int32 GetObjectCount() const;
    void DumpToLog() const;

    static bool IsDebugEnabled();

private:
    USmartObjectSubsystem* GetSmartObjects() const;
    void RunWatchdog();
    void RefreshDebugLabels();
    void UpdatePlayerLocks();
    void SetSlotsEnabled(AICLifeObjectActor& Object, bool bEnabled) const;

    TArray<TWeakObjectPtr<AICLifeObjectActor>> Objects;
    TMap<FSmartObjectSlotHandle, FICLifeSlotOccupant> Occupants;
    float WatchdogAccumulator = 0.f;
    float DebugAccumulator = 0.f;
    bool bDebugWasEnabled = false;
    int32 OrphanReleaseCount = 0;
};
