#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICHintRow.generated.h"

class UTexture2D;

UENUM(BlueprintType, meta=(ToolTip="첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."))
enum class EICFirstHintTrigger : uint8 { Movement UMETA(ToolTip="첫 이동 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."), Combat UMETA(ToolTip="첫 전투 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."), Item UMETA(ToolTip="첫 아이템 사용 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."), StatGrowth UMETA(ToolTip="첫 능력치 성장 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."), Production UMETA(ToolTip="첫 생산 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."), Gift UMETA(ToolTip="첫 선물 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."), Recruit UMETA(ToolTip="첫 영입 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다."), Sleep UMETA(ToolTip="첫 수면 안내입니다.\n첫 행동 안내를 표시할 사건입니다. 이미 표시한 안내는 WorldState에 기록해 중복 표시를 막습니다.") };

/** First action help is shown once and can be revisited. Disabling hints never locks the described feature. */
USTRUCT(BlueprintType, meta=(ToolTip="처음 발생하는 행동에 표시할 안내의 트리거·입력 안내 태그·이미지 정의입니다. 이미 본 기록은 WorldState에 저장하며 안내 실행/UI는 별도 연결 대상입니다.", ShortToolTip="처음 발생하는 행동에 표시할 안내의 트리거·입력 안내 태그·이미지 정의입니다."))
struct ISEKAICLASS_API FICHintRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint", meta=(ToolTip="최초 힌트 행동 종류입니다. 이미 본 기록과 실제 행동 이벤트/안내 UI를 연결합니다.\n입력 자료형: 선택 목록 (EICFirstHintTrigger).\nC++ 초기값: Movement — 첫 이동 안내입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 이미 본 기록은 WorldState에 저장하며 안내 실행/UI는 별도 연결 대상입니다.")) EICFirstHintTrigger Trigger = EICFirstHintTrigger::Movement;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint", meta=(ToolTip="힌트에 표시할 입력 안내 GameplayTag입니다. 실제 IA/IMC 키 표시 해석과 UI 연결이 필요합니다.\n입력 자료형: GameplayTag.\n소속 정의/시스템: 이미 본 기록은 WorldState에 저장하며 안내 실행/UI는 별도 연결 대상입니다.")) FGameplayTag ActionPromptTag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint", meta=(ToolTip="표시 순서 정수 키입니다. 실제 UI 정렬 연결을 확인하고 배열 순서/ID와 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 이미 본 기록은 WorldState에 저장하며 안내 실행/UI는 별도 연결 대상입니다.")) int32 DisplayOrder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint", meta=(ToolTip="힌트 Texture2D 안내 그림입니다. 실제 힌트 UI 연결이 필요합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\n소속 정의/시스템: 이미 본 기록은 WorldState에 저장하며 안내 실행/UI는 별도 연결 대상입니다.")) TSoftObjectPtr<UTexture2D> Illustration;
};
