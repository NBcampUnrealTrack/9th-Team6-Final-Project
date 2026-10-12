#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICGiftPreferenceRow.generated.h"

UENUM(BlueprintType, meta=(ToolTip="NPC의 선물 선호 단계입니다. 실제 관계 증감량은 GiftPreference 행에서 지정합니다."))
enum class EICGiftPreference : uint8 { Preferred UMETA(ToolTip="선호 선물입니다.\nNPC의 선물 선호 단계입니다. 실제 관계 증감량은 GiftPreference 행에서 지정합니다."), Neutral UMETA(ToolTip="보통 선물입니다.\nNPC의 선물 선호 단계입니다. 실제 관계 증감량은 GiftPreference 행에서 지정합니다."), Disliked UMETA(ToolTip="기피 선물입니다.\nNPC의 선물 선호 단계입니다. 실제 관계 증감량은 GiftPreference 행에서 지정합니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="NPC의 특정 아이템 선호 분류입니다. ItemID가 Item DT에 존재해야 하며 선호에 따른 관계 변화량은 상위 GiftPreference 행을 사용합니다.", ShortToolTip="NPC의 특정 아이템 선호 분류입니다."))
struct ISEKAICLASS_API FICGiftPreferenceEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift", meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.")) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift", meta=(ToolTip="해당 아이템 선호/보통/기피 분류입니다. 상위 행의 해당 관계 증감량을 사용하도록 정의합니다.\n입력 자료형: 선택 목록 (EICGiftPreference).\nC++ 초기값: Neutral — 보통 선물입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICGiftPreference Preference = EICGiftPreference::Neutral;
};

/** Reusable preference definition. Unlisted items are neutral; daily quotas and diminishing returns are not part of the design. */
USTRUCT(BlueprintType, meta=(ToolTip="NPC의 선호·보통·기피 선물과 관계 증감량입니다. 실제 선물 거래·관계 이벤트 실행은 아직 연결 대상입니다.", ShortToolTip="NPC의 선호·보통·기피 선물과 관계 증감량입니다."))
struct ISEKAICLASS_API FICGiftPreferenceRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift", meta=(ToolTip="선호 선물 관계 증가 정수입니다. 실제 선물 소모/관계 변경 거래를 연결해야 합니다.\n입력 자료형: 정수.\nC++ 초기값: 5. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 선물 거래·관계 이벤트 실행은 아직 연결 대상입니다.")) int32 PreferredAffinityDelta = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift", meta=(ToolTip="보통 선물 관계 증감 정수입니다. 0은 변화 없음이며 실제 선물 거래는 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 선물 거래·관계 이벤트 실행은 아직 연결 대상입니다.")) int32 NeutralAffinityDelta = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift", meta=(ToolTip="기피 선물의 관계 정수 증감입니다. -5는 관계 5 감소이며 실제 선물 거래 실행과 연결합니다.\n입력 자료형: 정수.\nC++ 초기값: -5. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 선물 거래·관계 이벤트 실행은 아직 연결 대상입니다.")) int32 DislikedAffinityDelta = -5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift", meta=(ToolTip="아이템 선호 또는 보상 지급 목록입니다. 상위 구조별 선호 분류/수량을 작성하며 실제 실행은 별도입니다.\n입력 자료형: 구조/객체 (FICGiftPreferenceEntry) 배열.\n소속 정의/시스템: 실제 선물 거래·관계 이벤트 실행은 아직 연결 대상입니다.")) TArray<FICGiftPreferenceEntry> Items;
};
