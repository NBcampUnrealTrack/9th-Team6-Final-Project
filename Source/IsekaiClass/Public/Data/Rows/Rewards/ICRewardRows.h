#pragma once
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICRewardRows.generated.h"

UENUM(BlueprintType, meta=(ToolTip="보상 지급 공간이 부족할 때의 처리 정책입니다. 일반 보상 실행기는 아직 연결이 필요합니다."))
enum class EICRewardOverflow : uint8 { RejectAll UMETA(ToolTip="전체 보상을 거부하고 아이템·골드·상태를 변경하지 않는 정책입니다.\n보상 지급 공간이 부족할 때의 처리 정책입니다. 일반 보상 실행기는 아직 연결이 필요합니다."), DropExcess UMETA(ToolTip="가방에 들어가지 않는 아이템을 월드 드롭으로 보존하는 정책입니다.\n보상 지급 공간이 부족할 때의 처리 정책입니다. 일반 보상 실행기는 아직 연결이 필요합니다."), DeferUntilWorldReturn UMETA(ToolTip="월드로 복귀할 때까지 지급을 보류하는 정책입니다. 보류 기록을 저장해야 합니다.\n보상 지급 공간이 부족할 때의 처리 정책입니다. 일반 보상 실행기는 아직 연결이 필요합니다.") };
UENUM(BlueprintType, meta=(ToolTip="드롭 후보를 선택하는 방식입니다. Probability와 Weight의 의미가 다릅니다."))
enum class EICDropSelection : uint8 { IndependentProbability UMETA(ToolTip="각 후보를 자신의 0~1 확률로 독립 판정합니다. 여러 후보가 함께 선택될 수 있습니다.\n드롭 후보를 선택하는 방식입니다. Probability와 Weight의 의미가 다릅니다."), WeightedChoice UMETA(ToolTip="후보의 양수 가중치 합에 대한 비율로 선택합니다. 지정한 추첨 횟수와 중복 규칙을 확인하세요.\n드롭 후보를 선택하는 방식입니다. Probability와 Weight의 의미가 다릅니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="보상으로 적용할 NPC 관계 변화량입니다. NPCID는 NPC DT 참조이며 Amount는 정수 증감량입니다. 실제 보상 거래는 별도 연결 대상입니다.", ShortToolTip="보상으로 적용할 NPC 관계 변화량입니다."))
struct ISEKAICLASS_API FICAffinityReward
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="NPC DT의 정의 ID입니다. R08 지역 규격의 같은 RowName/ID를 지정하고 액터 이름·인스턴스 GUID와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 보상 거래는 별도 연결 대상입니다.")) FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="정수 변화량입니다. 관계 보상은 증감량이며 골드 설정 함수에서는 총액입니다. 상위 구조의 의미를 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 보상 거래는 별도 연결 대상입니다.")) int32 Amount = 0;
};

USTRUCT(BlueprintType, meta=(ToolTip="아이템·골드·관계·레시피 해금과 가방 초과 처리의 정적 보상입니다. 지급은 모두 사전 검사한 뒤 거래/완료 영수증과 함께 한 번 확정하도록 정의합니다. 보상 실행기는 아직 연결 대상입니다.", ShortToolTip="아이템·골드·관계·레시피 해금과 가방 초과 처리의 정적 보상입니다."))
struct ISEKAICLASS_API FICRewardRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="아이템 선호 또는 보상 지급 목록입니다. 상위 구조별 선호 분류/수량을 작성하며 실제 실행은 별도입니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.\n소속 정의/시스템: 보상 실행기는 아직 연결 대상입니다.")) TArray<FICItemAmount> Items;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="보유 골드 또는 보상 지급량입니다. 보유 원본은 WorldState.Gold이며 보상에서는 증가량입니다. 음수는 허용하지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 보상 실행기는 아직 연결 대상입니다.")) int32 Gold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="NPC 현재 관계 정수 또는 보상 관계 변화 목록입니다. 현재값은 0~100, 보상 목록은 NPC별 증감량입니다.\n입력 자료형: 구조/객체 (FICAffinityReward) 배열.\n소속 정의/시스템: 보상 실행기는 아직 연결 대상입니다.")) TArray<FICAffinityReward> Affinity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="해금할 Recipe ID 목록입니다. 실제 원본은 WorldState.UnlockedRecipeIDs입니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 보상 실행기는 아직 연결 대상입니다.")) TArray<FName> UnlockRecipeIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="보상 가방 초과 처리 규칙입니다. 전체 거부/초과 드롭/월드 복귀까지 대기를 구분합니다. 실제 일반 보상 실행기는 별도 연결 대상입니다.\n입력 자료형: 선택 목록 (EICRewardOverflow).\nC++ 초기값: RejectAll — 전체 보상을 거부하고 아이템·골드·상태를 변경하지 않는 정책입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 보상 실행기는 아직 연결 대상입니다.")) EICRewardOverflow Overflow = EICRewardOverflow::RejectAll;
};

USTRUCT(BlueprintType, meta=(ToolTip="드롭 후보의 아이템·수량 범위·확률/가중치·행운/등급 적용 규칙입니다. Selection 방식에 맞춰 Probability 또는 Weight를 사용합니다. 전투 드롭 추첨 실행은 별도 연결 대상입니다.", ShortToolTip="드롭 후보의 아이템·수량 범위·확률/가중치·행운/등급 적용 규칙입니다."))
struct ISEKAICLASS_API FICDropEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 전투 드롭 추첨 실행은 별도 연결 대상입니다.")) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ToolTip="최소 지급 수량입니다. 양수 정수로 MaximumCount 이하로 작성합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 전투 드롭 추첨 실행은 별도 연결 대상입니다.")) int32 MinimumCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ToolTip="최대 지급 수량입니다. MinimumCount 이상이며 스택/보관 공간 검사도 필요합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 전투 드롭 추첨 실행은 별도 연결 대상입니다.")) int32 MaximumCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1", ToolTip="독립 추첨 확률입니다. 0~1에서 0.25=25%입니다. 가중치 선택 Weight와 구분합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.\n소속 정의/시스템: 전투 드롭 추첨 실행은 별도 연결 대상입니다.")) double Probability = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="후보 상대 가중치입니다. 후보 1과 3이면 각각 25%/75%입니다. 0은 선택 기여 없음이며 음수/비정상 값은 사용하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 전투 드롭 추첨 실행은 별도 연결 대상입니다.")) double Weight = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="아이템 드롭 확률 행운 보정입니다. 실제 독립 확률 계산/상한 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 전투 드롭 추첨 실행은 별도 연결 대상입니다.")) bool bApplyPlayerLUKToProbability = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="이 드롭 장비의 등급 추첨 사용입니다. EquipmentGrades와 실제 보상 추첨 실행을 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 전투 드롭 추첨 실행은 별도 연결 대상입니다.")) bool bEquipmentGradeRoll = false;
};

USTRUCT(BlueprintType, meta=(ToolTip="장비 드롭 등급 하나의 가중치와 행운 반영 여부입니다. 옵션 개수/수치 정의와 별도이며 전투 드롭 등급 추첨 연결이 필요합니다.", ShortToolTip="장비 드롭 등급 하나의 가중치와 행운 반영 여부입니다."))
struct ISEKAICLASS_API FICEquipmentGradeWeight
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="정의 후보 등급 또는 저장 인스턴스 등급입니다. 인스턴스 0~4는 Common/Uncommon/Rare/Epic/Legendary입니다.\n입력 자료형: 선택 목록 (EPlayerItemGrade).\nC++ 초기값: Common — 일반 등급입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerItemGrade Grade = EPlayerItemGrade::Common;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="후보 상대 가중치입니다. 후보 1과 3이면 각각 25%/75%입니다. 0은 선택 기여 없음이며 음수/비정상 값은 사용하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double Weight = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="장비 등급 추첨 행운 보정입니다. 해제하면 기본 가중치이며 실제 추첨 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bApplyPlayerLUK = false;
};

USTRUCT(BlueprintType, meta=(ToolTip="몬스터 등의 아이템/골드 드롭 추첨 정의입니다. WorldState의 실제 드롭 레코드와 구분하세요. 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.", ShortToolTip="몬스터 등의 아이템/골드 드롭 추첨 정의입니다."))
struct ISEKAICLASS_API FICDropTableRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="드롭 추첨 방식입니다. 독립 확률은 Probability, 가중치 선택은 Weight를 사용하도록 정의합니다.\n입력 자료형: 선택 목록 (EICDropSelection).\nC++ 초기값: IndependentProbability — 각 후보를 자신의 0~1 확률로 독립 판정합니다. 여러 후보가 함께 선택될 수 있습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.")) EICDropSelection Selection = EICDropSelection::IndependentProbability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ToolTip="드롭 추첨 반복 횟수입니다. 양수이며 실제 추첨/행운/중복 지급 정책을 연결합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.")) int32 Rolls = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="상점/드롭 항목 목록입니다. 상점은 Item/표시 순서, 드롭은 후보 수량/확률/가중치이며 상위 타입을 확인합니다.\n입력 자료형: 구조/객체 (FICDropEntry) 배열.\n소속 정의/시스템: 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.")) TArray<FICDropEntry> Entries;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="드롭 장비 등급 가중치 목록입니다. 옵션 풀과 별개인 전투 추첨 규칙이며 실제 추첨 연결이 필요합니다.\n입력 자료형: 구조/객체 (FICEquipmentGradeWeight) 배열.\n소속 정의/시스템: 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.")) TArray<FICEquipmentGradeWeight> EquipmentGrades;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="드롭 최소 정수 골드입니다. 0 이상/MaximumGold 이하로 작성합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.")) int32 MinimumGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="드롭 최대 정수 골드입니다. MinimumGold 이상이며 보정/지급 실행 연결이 필요합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.")) int32 MaximumGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="드롭 골드 행운 보정입니다. 실제 기본량/곡선/상한 추첨 연결은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 지속 드롭·줍기는 구현되어 있지만 이 DT를 사용한 전투 보상 추첨은 별도 연결 대상입니다.")) bool bApplyPlayerLUKToGold = true;
};
