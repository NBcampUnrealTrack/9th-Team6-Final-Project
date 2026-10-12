#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ICGameplayEffects.generated.h"

UCLASS(meta=(ToolTip="HP에 IC.Data.Amount를 가산하는 즉시 효과입니다. 최종 피해·치유·아이템 회복 계산 결과를 전달하며 최대값 제한은 AttributeSet에서 처리합니다."))
class ISEKAICLASS_API UICHealthEffect : public UGameplayEffect { GENERATED_BODY() public: UICHealthEffect(); };
UCLASS(meta=(ToolTip="MP 비용 또는 회복량을 적용하는 즉시 효과입니다. 스킬 첫 효과 확정 시 비용을 한 번만 전달합니다."))
class ISEKAICLASS_API UICManaEffect : public UGameplayEffect { GENERATED_BODY() public: UICManaEffect(); };
UCLASS(meta=(ToolTip="플레이어 SP 변경을 적용하는 즉시 효과입니다. 자연 감소는 PlayClock 10초당 1이며 NPC·몬스터에는 적용하지 않습니다."))
class ISEKAICLASS_API UICSPChangeEffect : public UGameplayEffect { GENERATED_BODY() public: UICSPChangeEffect(); };
UCLASS(meta=(ToolTip="DT 상태 효과의 실행 핸들과 상태 태그를 ASC에 유지하는 효과입니다. 만료·Tick은 공통 PlayClock 경로가 관리하며 저장에는 핸들 대신 정의 ID와 시각을 기록합니다."))
class ISEKAICLASS_API UICStatusGameplayEffect : public UGameplayEffect { GENERATED_BODY() public: UICStatusGameplayEffect(); };
