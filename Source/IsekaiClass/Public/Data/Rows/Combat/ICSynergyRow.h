#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICSynergyRow.generated.h"

/** Unordered 2-4 NPC ID membership; Downed count toward membership but do not receive modifiers. */
USTRUCT(BlueprintType, meta=(ToolTip="지정 NPC 조합이 만족할 때 적용할 시너지의 정적 정의입니다. 현재 파티 판정과 능력치 효과 실행은 별도 연결 대상입니다.", ShortToolTip="지정 NPC 조합이 만족할 때 적용할 시너지의 정적 정의입니다."))
struct ISEKAICLASS_API FICSynergyRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy", meta=(ToolTip="시너지 조합 NPC ID 목록입니다. 중복을 피하고 실제 활성 파티/모드 판정과 연결합니다.\n입력 자료형: 이름/ID 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 파티 판정과 능력치 효과 실행은 별도 연결 대상입니다.")) TArray<FName> MemberNPCIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy", meta=(ToolTip="시너지 활성 조건입니다. 동료 조합/모드/관계 등을 실제 실행에서 판정해야 합니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 파티 판정과 능력치 효과 실행은 별도 연결 대상입니다.")) FICConditionGroup ActivationConditions;
    // Evaluate strongest modifier independently per stat across active combinations.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy", meta=(ToolTip="효과/시너지 Flat/Percent 능력치 기여 목록입니다. 실제 적용/종료에서 한 번 더하고 제거하는 GAS 연결이 필요합니다.\n입력 자료형: 구조/객체 (FICStatModifier) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 파티 판정과 능력치 효과 실행은 별도 연결 대상입니다.")) TArray<FICStatModifier> StatModifiers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="Texture2D 아이콘입니다. 실제 UI/표시 실행에서 이 참조를 사용하도록 연결합니다. 비면 전용 아이콘이 없습니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 파티 판정과 능력치 효과 실행은 별도 연결 대상입니다.")) TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="시너지 활성 NiagaraSystem입니다. 실제 활성 변화 이벤트와 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UNiagaraSystem).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 파티 판정과 능력치 효과 실행은 별도 연결 대상입니다.")) TSoftObjectPtr<UNiagaraSystem> ActivationVFX;
};
