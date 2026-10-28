#pragma once

#include "CoreMinimal.h"
#include "SmartObjectDefinition.h"
#include "NPC/LifeObject/ICLifeObjectTypes.h"
#include "ICLifeObjectBehaviorDefinition.generated.h"

/**
 * Smart Object Definition 에셋의 "Default Behavior Definitions"(또는 슬롯별 Behavior Definitions)에 넣는 행동 정의.
 *
 * Smart Object 시스템은 슬롯을 '점유(Occupied)' 상태로 바꿀 때 이 타입의 정의가 있는지 확인한다.
 * 없으면 점유 표시만 실패하고(예약은 유지되므로 중복 사용은 여전히 막힌다) 경고 로그가 남는다.
 * 오브젝트 종류별 기본 애니메이션은 여기서, 개별 오브젝트 예외는 AICLifeObjectActor에서 덮어쓴다.
 */
UCLASS(EditInlineNew, CollapseCategories, meta=(DisplayName="IC Life Object Behavior"))
class ISEKAICLASS_API UICLifeObjectBehaviorDefinition : public USmartObjectBehaviorDefinition
{
    GENERATED_BODY()

public:
    /** 이 정의를 쓰는 슬롯에서 재생할 기본 연출. 나중에 애니메이션 담당 에셋이 들어오면 지정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|Life")
    FICLifeSlotPresentation Presentation;
};
