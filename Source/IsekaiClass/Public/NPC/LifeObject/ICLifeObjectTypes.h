#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "ICLifeObjectTypes.generated.h"

class AICLifeObjectActor;
class UAnimMontage;

/**
 * NPC 생활 오브젝트 공용 타입.
 *
 * 생활 오브젝트는 연출 전용이다. 생산·소비·NPC 간 거래와 연결하지 않으며,
 * 예약/점유 정보는 런타임 전용이라 세이브에 저장하지 않는다.
 */
namespace ICLifeTags
{
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Activity);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sleep);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sit);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Rest);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Work);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorkShop);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorkCraft);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorkCook);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WorkFarm);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Leisure);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeisureWalk);
    ISEKAICLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeisureView);
}

/** 우선 구현 범위의 생활 오브젝트 종류. */
UENUM(BlueprintType)
enum class EICLifeObjectType : uint8
{
    Bed         UMETA(DisplayName="침대"),
    Seat        UMETA(DisplayName="의자/벤치"),
    ShopCounter UMETA(DisplayName="상점 카운터"),
    Workbench   UMETA(DisplayName="작업대"),
    Kitchen     UMETA(DisplayName="주방"),
    FarmSpot    UMETA(DisplayName="농장 작업 지점"),
    Leisure     UMETA(DisplayName="여가 지점")
};

/** 오브젝트를 하나도 잡지 못했을 때의 대체 행동. */
UENUM(BlueprintType)
enum class EICLifeFallback : uint8
{
    /** 시간표 목적지 앵커에서 대기한다. */
    WaitAtAnchor UMETA(DisplayName="앵커에서 대기"),
    /** 침대를 뺏긴 경우 앵커(바닥)에서 잔다. */
    SleepOnFloor UMETA(DisplayName="바닥에서 수면")
};

/** 현재 GameClock 기준으로 계산한 NPC의 일과 구간. 저장하지 않는다. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICLifeRoutine
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="IC|Life") bool bValid = false;
    UPROPERTY(BlueprintReadOnly, Category="IC|Life") FName SegmentID;
    UPROPERTY(BlueprintReadOnly, Category="IC|Life") EICScheduleActivity Activity = EICScheduleActivity::Idle;
    /** 오브젝트 탐색에 쓰는 활동 태그. 비어 있으면 오브젝트를 쓰지 않는 구간(이동/대기)이다. */
    UPROPERTY(BlueprintReadOnly, Category="IC|Life") FGameplayTag ActivityTag;
    UPROPERTY(BlueprintReadOnly, Category="IC|Life") FName DestinationAnchorID;
    UPROPERTY(BlueprintReadOnly, Category="IC|Life") int32 StartGameMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="IC|Life") int32 EndGameMinute = 0;

    bool WantsObject() const { return bValid && ActivityTag.IsValid(); }
    bool IsSameSegment(const FICLifeRoutine& Other) const
    {
        return bValid == Other.bValid && SegmentID == Other.SegmentID && ActivityTag == Other.ActivityTag
            && DestinationAnchorID == Other.DestinationAnchorID && StartGameMinute == Other.StartGameMinute;
    }
};

/** 탐색 결과 후보 하나(오브젝트 + 슬롯). 점수가 낮을수록 우선이다. */
USTRUCT()
struct ISEKAICLASS_API FICLifeObjectCandidate
{
    GENERATED_BODY()

    UPROPERTY() TWeakObjectPtr<AICLifeObjectActor> Object;
    UPROPERTY() int32 SlotIndex = INDEX_NONE;
    UPROPERTY() double Score = 0.0;
};

/** 슬롯 사용 중 재생할 연출. 애니메이션은 나중에 넣어도 되고, 비어 있으면 로그/Print만 한다. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICLifeSlotPresentation
{
    GENERATED_BODY()

    /** Idle/작업/수면 몽타주. 반복이 필요하면 몽타주 안에서 Loop 섹션을 만든다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|Life") TSoftObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|Life", meta=(ClampMin="0.01")) float PlayRate = 1.f;
    /** 비어 있으면 처음부터 재생한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|Life") FName StartSection;

    bool IsSet() const { return !Montage.IsNull(); }
};

namespace ICLife
{
    /** 오브젝트 종류의 기본 활동 태그. 액터의 ActivityTags가 비어 있을 때 사용한다. */
    ISEKAICLASS_API FGameplayTagContainer GetDefaultActivityTags(EICLifeObjectType Type);
    /** 시간표 구간에서 탐색용 활동 태그를 만든다. 구간 ActivityTag가 우선이고, 없으면 Activity 열거형으로 추론한다. */
    ISEKAICLASS_API FGameplayTag ResolveActivityTag(EICScheduleActivity Activity, const FGameplayTag& SegmentTag);
    ISEKAICLASS_API const TCHAR* LexType(EICLifeObjectType Type);
}
