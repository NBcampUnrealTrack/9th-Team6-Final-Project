#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICQuestRow.generated.h"

UENUM(BlueprintType, meta=(ToolTip="퀘스트의 콘텐츠 분류입니다. 선후행 조건과 완료 방식은 별도 필드로 지정합니다."))
enum class EICQuestType : uint8
{
    General UMETA(ToolTip="일반 퀘스트입니다.\n퀘스트의 콘텐츠 분류입니다. 선후행 조건과 완료 방식은 별도 필드로 지정합니다."),
    Chain UMETA(ToolTip="다른 퀘스트와 선후행 관계가 있는 연쇄 퀘스트입니다.\n퀘스트의 콘텐츠 분류입니다. 선후행 조건과 완료 방식은 별도 필드로 지정합니다."),
    Main UMETA(ToolTip="주요 진행을 이끄는 메인 퀘스트입니다.\n퀘스트의 콘텐츠 분류입니다. 선후행 조건과 완료 방식은 별도 필드로 지정합니다.")
};


UENUM(BlueprintType, meta=(ToolTip="퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."))
enum class EICQuestObjectiveType : uint8
{
    Talk UMETA(ToolTip="지정 NPC와 대화하는 목표입니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    DeliverItem UMETA(ToolTip="아이템을 전달하고 필요한 소모를 확정하는 목표입니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    Defeat UMETA(ToolTip="지정 몬스터를 처치하는 목표입니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    Collect UMETA(ToolTip="획득 사건을 집계하는 목표입니다. 현재 보유 수량 목표와 구분합니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    Craft UMETA(ToolTip="지정 생산/레시피를 완료하는 목표입니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    Affinity UMETA(ToolTip="NPC 관계 값을 만족하는 목표입니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    Stat UMETA(ToolTip="지정 능력치 값을 만족하는 목표입니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    Visit UMETA(ToolTip="지정 앵커/지역을 방문하는 목표입니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다."),
    PossessItem UMETA(ToolTip="현재 아이템 보유 수량을 만족하는 목표입니다. 누적 획득 횟수와 구분합니다.\n퀘스트 목표의 집계 방식입니다. 종류에 맞는 대상 ID·필요 수량·능력치·비교를 지정해야 합니다.")
};

UENUM(BlueprintType, meta=(ToolTip="퀘스트 완료 확정의 경로입니다. 보상과 완료 플래그를 한 거래로 처리해야 합니다."))
enum class EICQuestCompletionMode : uint8
{
    NPCInteraction UMETA(ToolTip="완료 NPC와 상호작용할 때 완료를 확정하는 방식입니다.\n퀘스트 완료 확정의 경로입니다. 보상과 완료 플래그를 한 거래로 처리해야 합니다."),
    
    SystemEvent UMETA(ToolTip="목표를 만족한 시스템 사건에서 완료를 확정하는 방식입니다.\n퀘스트 완료 확정의 경로입니다. 보상과 완료 플래그를 한 거래로 처리해야 합니다.")
};

USTRUCT(BlueprintType, meta=(ToolTip="퀘스트 목표 하나의 종류·대상·수량/임계값·활동 필터·대사/안내 위치 정의입니다. 목표별 카운터는 QuestState, 공통 중복 이벤트 이력은 WorldState에 기록합니다. 실제 퀘스트 실행기는 아직 연결 대상입니다.", ShortToolTip="퀘스트 목표 하나의 종류·대상·수량/임계값·활동 필터·대사/안내 위치 정의입니다."))
struct ISEKAICLASS_API FICQuestObjectiveDefinition
{
    GENERATED_BODY()

    /** Stable key inside this quest; counters and event receipts live in WorldState. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ToolTip="퀘스트 안의 목표 고유 키입니다. 진행 카운터/이벤트 영수증을 연결하므로 저장 이후 이름 변경 시 변환이 필요합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName ObjectiveID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ToolTip="상세 설명의 String Table 키입니다. 문장 원문 대신 키를 작성하고 해당 테이블에 원문을 등록합니다. 사용하는 화면/노드의 키 해석 연결이 필요합니다.\n입력 자료형: 문자열.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FString DescriptionKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICQuestObjectiveType).\nC++ 초기값: Talk — 지정 NPC와 대화하는 목표입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICQuestObjectiveType Type = EICQuestObjectiveType::Talk;

    /** NPC/monster/item/recipe/anchor/region definition, according to objective type. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ToolTip="미리보기 RenderTarget 또는 조건/사용 대상입니다. 구조 타입에 맞는 참조를 지정하고 게임 상태 원본과 표시 출력을 구분합니다.\n입력 자료형: 구조/객체 (FICDefinitionRef).\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FICDefinitionRef Target;

    /** Optional additional output-item filter for successful production/collection events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Activity", meta=(ToolTip="성공한 생산/수집 결과의 Item ID 필터입니다. 목표 Target과 별도로 결과물을 제한하도록 정의합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName OutputItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Activity", meta=(ToolTip="퀘스트에 LifeActivity 분류 필터를 사용합니다. 해제하면 이 필터를 사용하지 않도록 정의합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    bool bFilterLifeActivity = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Activity", meta=(EditCondition="bFilterLifeActivity", ToolTip="퀘스트에 인정할 생산/생활 활동입니다. bFilterLifeActivity 체크 때 필터로 사용하도록 정의합니다.\n입력 자료형: 선택 목록 (EICLifeActivity).\nC++ 초기값: Gathering — 채집 활동입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n편집 활성 조건: bFilterLifeActivity.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICLifeActivity LifeActivity = EICLifeActivity::Gathering;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ClampMin="1", ToolTip="목표 누적 횟수/수량입니다. 1 이상 정수이며 현재값 임계 조건의 RequiredValue와 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    int32 RequiredCount = 1;

    /** Only current affinity/stat conditions use this threshold; they are rechecked before reward. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState", meta=(ToolTip="능력치/관계 조건 임계값입니다. Compare·RequiredStat·Target을 함께 작성하고 보고 직전 재검사를 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    double RequiredValue = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState", meta=(ToolTip="현재값과 임계값의 비교 연산입니다. GreaterOrEqual이면 기준 이상을 요구합니다.\n입력 자료형: 선택 목록 (EICComparison).\nC++ 초기값: GreaterOrEqual — 값이 기준값 이상일 때 만족합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICComparison Compare = EICComparison::GreaterOrEqual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState", meta=(ToolTip="목표 Type=Stat의 비교 능력치입니다. RequiredValue/Compare를 함께 지정합니다.\n입력 자료형: 선택 목록 (EICStat).\nC++ 초기값: None — 능력치 대상을 지정하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICStat RequiredStat = EICStat::None;

    /** Base stats are the default. An effective-stat exception must be explicitly authored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState", meta=(ToolTip="체크하면 장비/효과 반영 유효 능력치, 해제하면 기본 능력치를 비교하도록 정의합니다. 실제 유효 계산 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    bool bUseEffectiveStats = false;

    /** Only Talk/Visit may opt into state already true at acceptance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState", meta=(ToolTip="퀘스트 수락 전부터 만족한 상태를 인정할지 설정합니다. Talk/Visit만 명시적으로 허용하며 모든 누적 목표를 소급 인정하는 규칙이 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    bool bAllowCurrentState = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="Dialogue DT ID입니다. 같은 대사 내부 NodeID/ChoiceID와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName DialogueID;

    /** Optional fixed-location guidance; live NPC locations still come from WorldState/schedule. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance", meta=(ToolTip="목표 안내용 고정 Anchor ID입니다. 이동 NPC 현재 위치 원본은 WorldState/일과이므로 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName GuidanceAnchorID;
};

USTRUCT(BlueprintType, meta=(ToolTip="퀘스트 목표들을 All/Any로 묶습니다. 그룹 사이의 결합 방식은 상위 Quest.ObjectiveGroupPolicy를 확인하세요.", ShortToolTip="퀘스트 목표들을 All/Any로 묶습니다."))
struct ISEKAICLASS_API FICQuestObjectiveGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives", meta=(ToolTip="그룹 내부 결합 방식입니다. All=모두, Any=하나 이상입니다. 그룹 간 방식은 상위 정의를 확인합니다.\n입력 자료형: 선택 목록 (EICConditionPolicy).\nC++ 초기값: All — 그룹 안의 조건을 모두 만족해야 합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다."))
    EICConditionPolicy Policy = EICConditionPolicy::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives", meta=(ToolTip="그룹의 목표 목록 또는 저장 목표별 카운터 맵입니다. 정적 조건과 현재 정수 진행을 구분합니다.\n입력 자료형: 구조/객체 (FICQuestObjectiveDefinition) 배열."))
    TArray<FICQuestObjectiveDefinition> Objectives;
};

/** Static quest rules. Active state/counters/reward receipts and repeat-instance IDs are never DT fields. */
USTRUCT(BlueprintType, meta=(ToolTip="퀘스트 수락·목표·완료·보상·반복·대사 연결의 정적 정의입니다. 활성 상태·진행 수치·완료 영수증은 WorldState에만 기록합니다. 실제 퀘스트 실행기는 아직 연결 대상입니다.", ShortToolTip="퀘스트 수락·목표·완료·보상·반복·대사 연결의 정적 정의입니다."))
struct ISEKAICLASS_API FICQuestRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest", meta=(ToolTip="일반/연속/메인 퀘스트 분류입니다. 체인/선행/후속 규칙을 함께 작성하며 분류만으로 진행 실행이 생기지 않습니다.\n입력 자료형: 선택 목록 (EICQuestType).\nC++ 초기값: General — 일반 퀘스트입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICQuestType QuestType = EICQuestType::General;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest", meta=(ToolTip="튜토리얼 퀘스트 분류입니다. 실제 안내/진행/UI 연결과 별개인 정의 표시입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    bool bTutorial = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName RegionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest", meta=(ToolTip="연속 퀘스트 그룹 키입니다. 개별 Quest ID와 별개이며 같은 체인에 같은 값을 사용합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName ChainID;

    /** Every listed quest must have completed; optional OR prerequisites use AcceptanceConditions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance", meta=(ToolTip="모두 완료되어야 하는 선행 Quest ID 목록입니다. OR 조건은 AcceptanceConditions에 정의합니다. 순환 참조를 피하세요.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    TArray<FName> PrerequisiteQuestIDs;

    /** Groups are ANDed; each group explicitly chooses AND/OR for its member conditions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance", meta=(ToolTip="퀘스트 수락 조건 그룹입니다. 그룹 간 모두 만족, 그룹 내부는 All/Any를 선택하도록 정의합니다.\n입력 자료형: 구조/객체 (FICConditionGroup) 배열.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    TArray<FICConditionGroup> AcceptanceConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance", meta=(ToolTip="퀘스트 제안 NPC ID입니다. 제안 위치/대사와 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName OfferNPCID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance", meta=(ToolTip="퀘스트 제안의 고정 Anchor ID입니다. OfferNPCID와 위치 안내/수락 상호작용을 함께 정의합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName OfferAnchorID;

    /** Dedicated quest-tab grants are reclaimed on abandonment; unrelated inventory remains untouched. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance", meta=(ToolTip="퀘스트 수락 시 지급할 퀘스트 아이템 목록입니다. Item ID/수량을 검증하고 수락 기록과 같은 거래로 지급하도록 정의합니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    TArray<FICItemAmount> AcceptanceQuestItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives", meta=(ToolTip="목표 그룹 사이 All/Any 결합 방식입니다. 각 그룹 내부 Policy와 별개이므로 두 수준을 구분합니다.\n입력 자료형: 선택 목록 (EICConditionPolicy).\nC++ 초기값: All — 그룹 안의 조건을 모두 만족해야 합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICConditionPolicy ObjectiveGroupPolicy = EICConditionPolicy::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives", meta=(ToolTip="퀘스트 목표 그룹 목록입니다. 그룹 내부/그룹 사이 결합 방식과 목표 로컬 ID를 작성합니다.\n입력 자료형: 구조/객체 (FICQuestObjectiveGroup) 배열.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    TArray<FICQuestObjectiveGroup> ObjectiveGroups;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion", meta=(ToolTip="퀘스트 완료 방식입니다. NPCInteraction은 보고 상호작용, SystemEvent는 명시적인 시스템 완료 규칙입니다. 목표 달성과 보상 확정을 구분합니다.\n입력 자료형: 선택 목록 (EICQuestCompletionMode).\nC++ 초기값: NPCInteraction — 완료 NPC와 상호작용할 때 완료를 확정하는 방식입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICQuestCompletionMode CompletionMode = EICQuestCompletionMode::NPCInteraction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion", meta=(ToolTip="퀘스트 보고 NPC ID입니다. 목표 달성만으로 완료/보상이 확정되는 것과 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName TurnInNPCID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion", meta=(ToolTip="퀘스트 보고 Anchor ID입니다. TurnInNPCID와 완료 상호작용 판정에서 사용하도록 정의합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName TurnInAnchorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion", meta=(ToolTip="Reward DT ID입니다. 완료 플래그/영수증과 보상 지급을 함께 확정하도록 정의합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName RewardID;

    /** Opens availability only, never automatic acceptance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion", meta=(ToolTip="후속 Quest ID 목록입니다. 선행 조건과 맞추고 자기 참조/순환을 피합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    TArray<FName> SuccessorQuestIDs;

    /** Ordinary rewards are atomic. Only the first ending quest uses the ending reward policy. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion", meta=(ToolTip="엔딩 담당 퀘스트 표시입니다. 실제 완료/보상/귀환 실행을 연결하며 체크만으로 엔딩이 재생되지 않습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    bool bEndingQuest = false;

    /** Main and chain quests must author false; general quests may allow abandonment/reacceptance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat", meta=(ToolTip="퀘스트 포기 허용입니다. 체크는 허용, 해제는 금지 규격이며 실제 포기/수락품/진행 정리 실행은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    bool bAllowAbandon = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat", meta=(ToolTip="퀘스트 반복 허용입니다. 해제는 일회성, 체크는 지연/조건/회차 GUID를 사용하도록 정의합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    bool bRepeatable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat", meta=(EditCondition="bRepeatable", ToolTip="반복 퀘스트 지연 기준 시계입니다. PlayClock 초와 GameClock 분 중 해당 지연 필드를 사용합니다.\n입력 자료형: 선택 목록 (EICClock).\nC++ 초기값: PlayClock — 누적 실제 플레이 초입니다. 생산·쿨타임·드롭 수명처럼 활동한 시간을 사용하며 수면 점프로 늘리지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n편집 활성 조건: bRepeatable.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    EICClock RepeatClock = EICClock::PlayClock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat", meta=(ClampMin="0.0", Units="s", EditCondition="bRepeatable && RepeatClock == EICClock::PlayClock", ToolTip="PlayClock 기준 반복 퀘스트 지연 초입니다. 메뉴 정지 중 경과하지 않으며 마지막 완료 시각과 비교합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n편집 활성 조건: bRepeatable && RepeatClock == EICClock::PlayClock.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    double RepeatDelayPlaySeconds = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat", meta=(ClampMin="0.0", EditCondition="bRepeatable && RepeatClock == EICClock::GameClock", ToolTip="GameClock 기준 반복 퀘스트 지연 분입니다. 수면 점프 영향과 마지막 완료 시각을 실제 판정기에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n편집 활성 조건: bRepeatable && RepeatClock == EICClock::GameClock.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    double RepeatDelayGameMinutes = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat", meta=(EditCondition="bRepeatable", ToolTip="반복 퀘스트 재제공 조건입니다. 기준 시계·지연 시간과 함께 재수락 전에 검사합니다.\n입력 자료형: 구조/객체 (FICConditionGroup) 배열.\n편집 활성 조건: bRepeatable.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    TArray<FICConditionGroup> RepeatConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="제안 상태 Dialogue ID입니다. 실제 상태별 대사 선택 연결이 필요합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName OfferDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="진행 상태 Dialogue ID입니다. 실제 상태/조건별 선택 연결이 필요합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName ActiveDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="보고 가능 상태 Dialogue ID입니다. 보상 확정 전 재검사와 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName ReadyDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="완료 뒤 Dialogue ID입니다. 표시로 보상을 재지급하지 않도록 완료 영수증과 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    FName CompletedDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="Texture2D 아이콘입니다. 실제 UI/표시 실행에서 이 참조를 사용하도록 연결합니다. 비면 전용 아이콘이 없습니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\n소속 정의/시스템: 실제 퀘스트 실행기는 아직 연결 대상입니다."))
    TSoftObjectPtr<UTexture2D> Icon;
};
