#pragma once

#include "CoreMinimal.h"
#include "Interaction/ICInteractionActor.h"
#include "SmartObjectTypes.h"
#include "NPC/LifeObject/ICLifeObjectTypes.h"
#include "ICLifeObjectActor.generated.h"

class USmartObjectComponent;
class UTextRenderComponent;
class USceneComponent;
class AICNPCBase;
class AICPlayerController;

/**
 * NPC 마을 생활용 공용 상호작용 오브젝트(침대·의자·카운터·작업대·주방·농장·여가 지점)의 기반 액터.
 *
 * - 슬롯 수와 슬롯 위치는 SmartObject 컴포넌트에 지정한 Smart Object Definition 에셋에서 정한다.
 * - 예약/점유는 Smart Object 시스템의 Claim을 그대로 쓰므로 한 슬롯을 두 NPC가 동시에 쓸 수 없다.
 * - 이 액터는 "무엇을 누가 쓸 수 있는가"만 담당하고, 실제 예약·해제는 UICLifeObjectSubsystem이 맡는다.
 * - 실제 생산·소비·거래 기능은 없다(연출 전용).
 *
 * 블루프린트에서 이 클래스를 부모로 BP_IC_LO_Bed 같은 종류별 블루프린트를 만들어 메시와 Definition만 바꿔 쓰면 된다.
 */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICLifeObjectActor : public AICInteractionActor
{
    GENERATED_BODY()

public:
    AICLifeObjectActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool TryInteract_Implementation(AICPlayerController* Controller) override;

    /**
     * 배치 기준점. Shape를 이 컴포넌트의 자식으로 두어, 자식 BP에서 메시의 상대 위치/회전을 편집할 수 있다.
     * 액터 자체의 월드 위치/회전은 이 루트를 움직이면 된다.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|LifeObject")
    TObjectPtr<USceneComponent> PlacementRoot;

    /** Smart Object 연결. Details에서 Definition Asset을 지정해야 슬롯이 생긴다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|LifeObject")
    TObjectPtr<USmartObjectComponent> SmartObject;

    /** 개발용 점유 상태 표시. ic.LifeObject.Debug 1 일 때만 보인다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|LifeObject")
    TObjectPtr<UTextRenderComponent> DebugLabel;

    // ---------- 식별 ----------

    /** 오브젝트 고유 ID. 팀 ID 규칙(영문 대문자·숫자·밑줄, 40자 이하)에 맞춰 직접 접두사를 정해 입력한다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|LifeObject|ID")
    FName ObjectID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|ID")
    EICLifeObjectType ObjectType = EICLifeObjectType::Seat;

    /** R01~R05. 비어 있으면 모든 지역 NPC가 후보로 본다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|LifeObject|ID")
    FName RegionID;

    /** 구버전 단일 연결 앵커입니다. 기존 배치 액터 호환용으로 유지되며 LinkedAnchorIDs와 함께 사용됩니다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|LifeObject|ID")
    FName LinkedAnchorID;

    /** 여러 NPC가 같은 생활 오브젝트를 교대로 쓰도록 연결할 시간표 목적지 앵커들입니다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|LifeObject|ID")
    TArray<FName> LinkedAnchorIDs;

    // ---------- 사용 조건 ----------

    /** 설정하면 이 NPC만 사용할 수 있다(개인 침대, 상점 카운터). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|LifeObject|Users")
    FName OwnerNPCID;

    /** 이 오브젝트가 제공하는 활동. 비어 있으면 ObjectType 기본값(침대=Sleep, 의자=Sit/Rest ...)을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Users", meta=(Categories="IC.Life.Activity"))
    FGameplayTagContainer ActivityTags;

    /** 사용 가능한 직업 태그(NPC DT의 OccupationTag와 비교). 비어 있으면 직업 제한 없음. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Users")
    FGameplayTagContainer AllowedOccupationTags;

    /** 사용 가능한 NPC ID 목록. 비어 있으면 ID 제한 없음. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|LifeObject|Users")
    TArray<FName> AllowedNPCIDs;

    // ---------- 연출 ----------

    /** 도착 후 NPC를 슬롯 위치·방향으로 맞춘다(높이는 유지). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Presentation")
    bool bSnapUserToSlot = true;

    /** 슬롯별 개별 연출. 인덱스가 슬롯 번호다. 비어 있으면 DefaultPresentation → Behavior Definition 순서로 찾는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Presentation")
    TArray<FICLifeSlotPresentation> SlotPresentations;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Presentation")
    FICLifeSlotPresentation DefaultPresentation;

    // ---------- 플레이어 (침대) ----------

    /** 플레이어가 F로 잘 수 있는 침대인지. 침대 종류에서만 의미가 있다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Player")
    bool bPlayerCanSleep = true;

    /** OwnerNPCID가 있는 침대를 플레이어가 쓰기 위한 호감도(기획: 200). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Player", meta=(ClampMin="0", ClampMax="500"))
    int32 PlayerRequiredAffinity = 200;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|LifeObject|Player") FTransform PlayerSleepPoseOffset;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|LifeObject|Player") FVector PlayerWakeOffset=FVector(160,0,0);
    virtual FText GetInteractionText() const override;

    /** 플레이어가 잔 뒤 이 거리 이상 떨어질 때까지 NPC가 침대를 못 쓴다(그동안 NPC는 바닥에서 잔다). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|LifeObject|Player", meta=(ClampMin="0", Units="cm"))
    float PlayerLockReleaseDistanceCm = 300.f;

    // ---------- 조회 ----------

    UFUNCTION(BlueprintPure, Category="IC|LifeObject") FName GetDebugID() const;
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") FGameplayTagContainer GetEffectiveActivityTags() const;
    /** Legacy LinkedAnchorID와 다중 LinkedAnchorIDs를 모두 포함해 앵커 연결 여부를 확인합니다. */
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") bool IsLinkedToAnchor(FName AnchorID) const;
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") int32 GetSlotCount() const;
    /** Smart Object 시스템에 등록되어 슬롯을 쓸 수 있는 상태인지. */
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") bool IsSmartObjectReady() const;
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") bool IsOccupiedByAnyNPC() const;
    /** 상점 카운터: 주인이 지금 이 카운터를 쓰고 있는지(근무 중인지) 확인할 때 사용. */
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") bool IsUsedByNPC(FName NPCID) const;
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") TArray<FName> GetUserIDs() const;
    UFUNCTION(BlueprintPure, Category="IC|LifeObject") bool IsPlayerLocked() const { return bPlayerLocked; }

    bool GetSlotHandle(int32 SlotIndex, FSmartObjectSlotHandle& OutSlot) const;
    bool GetSlotTransform(int32 SlotIndex, FTransform& OutTransform) const;
    const FICLifeSlotPresentation* FindActorPresentation(int32 SlotIndex) const;
    /** 소유자·허용 ID·직업·지역 조건 검사. 슬롯 비어 있음 여부는 보지 않는다. */
    bool CanBeUsedBy(FName NPCID, const FGameplayTag& OccupationTag, FName NPCRegionID, FString* OutReason = nullptr) const;

    /** 서브시스템 전용: 플레이어 잠금 상태 갱신. */
    void SetPlayerLockedInternal(bool bLocked, AActor* Player);
    AActor* GetLockingPlayer() const { return LockingPlayer.Get(); }
    void SetDebugText(const FString& Text, bool bVisible);

private:
    bool bPlayerLocked = false;
    TWeakObjectPtr<AActor> LockingPlayer;
};
