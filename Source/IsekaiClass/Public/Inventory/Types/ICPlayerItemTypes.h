#pragma once
#include "CoreMinimal.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICPlayerItemTypes.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="장착 장비의 능력치 기여 조회 결과입니다. Modifiers는 고정/가산 비율을 구분한 원본 기여이며 나머지 값은 BP 표시용 요약입니다. 파생/GAS 최종 적용은 별도 계산 측에서 처리합니다.", ShortToolTip="장착 장비의 능력치 기여 조회 결과입니다."))
struct FPlayerEquipmentStats
{
    GENERATED_BODY()
    // Preserve flat and percentage contributions independently for effective-stat/GAS calculation.
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="장착 장비 Flat/Percent 기여 목록입니다. 최종 파생/GAS 계산으로 넘길 때 두 단위를 구분해 한 번 적용합니다.\n입력 자료형: 구조/객체 (FICStatModifier) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<FICStatModifier> Modifiers;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="장착 장비 공격 기여 요약입니다. 물리/마법 구분은 Modifiers.Stat으로 확인하며 최종 공격력과 다릅니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float Attack=0.f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="기본 방어/장비 방어 기여입니다. 상위 구조의 기본값/추가량을 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float Defense=0.f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="체력 상한 또는 장비 HP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float MaxHP=0.f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="마나 상한 또는 장비 MP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float MaxMP=0.f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="생활 자원 상한 또는 장비 SP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float MaxSP=0.f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="장비 공격 속도 기여입니다. 비율 0.03=3%이며 최종 배율 적용은 파생 계산 측입니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float AttackSpeed=0.f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="장비 이동 속도 기여입니다. 비율 0.03=3%이며 Pawn의 실제 cm/s와 다릅니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float MoveSpeed=0.f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="장비 치명타 확률 기여입니다. 0.03=3%p이며 최종 상한 적용은 파생 계산 측입니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float CriticalRate=0.f;
};
