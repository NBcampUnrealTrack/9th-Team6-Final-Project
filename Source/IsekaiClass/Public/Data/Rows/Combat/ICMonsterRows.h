#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICMonsterRows.generated.h"

class APawn;
class UAnimInstance;
class UBehaviorTree;
class UBlackboardData;
class USkeletalMesh;

/** Monster values are authored directly and never derived from player/recruit primary stats. */
USTRUCT(BlueprintType, meta=(ToolTip="몬스터 전투 기본값입니다. 플레이어의 6대 성장 능력치와 별도이며 자원 상한·공격·방어·속도·치명타를 지정합니다. 실제 몬스터 액터/GAS 적용은 별도 구현 대상입니다.", ShortToolTip="몬스터 전투 기본값입니다."))
struct ISEKAICLASS_API FICMonsterCombatStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="1", ToolTip="체력 상한 또는 장비 HP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 MaxHP = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ToolTip="마나 상한 또는 장비 MP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 MaxMP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ToolTip="물리 기본 공격력입니다. 실제 피해/방어 계산 실행은 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 PhysicalAttack = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ToolTip="마법 기본 공격력입니다. 실제 피해/방어 계산 실행은 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 MagicAttack = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ToolTip="기본 방어/장비 방어 기여입니다. 상위 구조의 기본값/추가량을 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 Defense = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ToolTip="몬스터 기본 이동 cm/s입니다. 300=3m/s이며 실제 Pawn 이동 적용은 별도입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double MoveSpeedCmPerSecond = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ToolTip="공격 속도 배율입니다. 1=기준, 1.2=20% 증가입니다. 실제 몽타주/공격 적용은 별도입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double AttackSpeedMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", ClampMax="1", ToolTip="치명타 확률입니다. 0.05=5%이며 Flat 확률 기여와 Percent 보정을 구분합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double CriticalChance = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="1", ToolTip="치명타 피해 배율입니다. 1.5=150%이며 확률과 별개입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double CriticalMultiplier = 1.0;
};

USTRUCT(BlueprintType, meta=(ToolTip="몬스터가 선택할 스킬과 가중치·조건입니다. 같은 후보 집합의 양수 가중치 합으로 선택 확률을 계산하도록 정의하며 AI 실행은 별도로 연결합니다.", ShortToolTip="몬스터가 선택할 스킬과 가중치·조건입니다."))
struct ISEKAICLASS_API FICMonsterSkillChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ToolTip="Skill DT ID입니다. 조회만으로 스킬이 실행되지 않습니다. 실제 GAS/타격/비용 확정 경로를 연결해야 합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName SkillID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0", ToolTip="후보 상대 가중치입니다. 후보 1과 3이면 각각 25%/75%입니다. 0은 선택 기여 없음이며 음수/비정상 값은 사용하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double Weight = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ToolTip="상위 동작 조건 묶음입니다. All/Any와 종류별 필드를 작성합니다. 일반 조건 실행기는 별도 연결 대상이며 정의 검사 성공은 실행 성공과 다릅니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICConditionGroup Conditions;
};

USTRUCT(BlueprintType, meta=(ToolTip="보스 패턴 선택 후보와 가중치·조건입니다. PatternID는 BossPattern DT의 정의를 가리킵니다. 실제 패턴 선택기는 연결이 필요합니다.", ShortToolTip="보스 패턴 선택 후보와 가중치·조건입니다."))
struct ISEKAICLASS_API FICBossPatternChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="BossPattern DT ID입니다. 소유 MonsterID와 페이즈 몬스터를 맞춥니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName PatternID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0", ToolTip="후보 상대 가중치입니다. 후보 1과 3이면 각각 25%/75%입니다. 0은 선택 기여 없음이며 음수/비정상 값은 사용하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double Weight = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="상위 동작 조건 묶음입니다. All/Any와 종류별 필드를 작성합니다. 일반 조건 실행기는 별도 연결 대상이며 정의 검사 성공은 실행 성공과 다릅니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICConditionGroup Conditions;
};

USTRUCT(BlueprintType, meta=(ToolTip="보스 페이즈의 진입 조건·HP 기준·패턴 후보·진입 효과·전환 연출입니다. 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.", ShortToolTip="보스 페이즈의 진입 조건·HP 기준·패턴 후보·진입 효과·전환 연출입니다."))
struct ISEKAICLASS_API FICBossPhaseDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ClampMin="1", ToolTip="보스 페이즈 순서 번호입니다. 중복 없는 양수 정수로 작성하고 현재 런타임 페이즈와 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) int32 PhaseIndex = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ToolTip="표시 이름의 String Table 키입니다. 예: ITM_CONS_001.NAME. 해당 테이블에 원문을 등록하세요. 키 입력과 DisplayName FText의 String Table 연결은 별개이며 사용하는 화면이 키를 해석해야 표시됩니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) FString NameKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ToolTip="페이즈 진입에 HP 비율 기준을 사용합니다. EnterAtOrBelowHPRatio와 추가 조건의 실제 실행을 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) bool bUseHPThreshold = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ClampMin="0", ClampMax="1", ToolTip="보스 페이즈 진입 HP/MaxHP 상한입니다. 0.5=50% 이하이며 bUseHPThreshold 체크 시 조건으로 사용하도록 정의합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) double EnterAtOrBelowHPRatio = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ToolTip="단계/페이즈 진입 조건입니다. 상태·영수증으로 같은 진입을 중복 처리하지 않아야 합니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) FICConditionGroup EntryConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ToolTip="페이즈 패턴 후보 목록입니다. Pattern ID·가중치·조건을 작성하고 실제 선택 실행을 연결합니다.\n입력 자료형: 구조/객체 (FICBossPatternChoice) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) TArray<FICBossPatternChoice> Patterns;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ToolTip="보스 페이즈 진입 StatusEffect ID 목록입니다. 중복 적용/종료는 실제 페이즈 실행에서 처리합니다.\n입력 자료형: 이름/ID 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) TArray<FName> EntryStatusEffectIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ToolTip="페이즈 전환 AnimMontage입니다. 전환 중 상태/무적 종료를 실제 전투 경로에 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UAnimMontage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) TSoftObjectPtr<UAnimMontage> TransitionMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ToolTip="배경음 SoundBase입니다. 상위 상태 변화에 실제 전환을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 페이즈 진행은 DT 값이 아니라 런타임 전투 상태입니다.")) TSoftObjectPtr<USoundBase> BGM;
};

USTRUCT(BlueprintType, meta=(ToolTip="보스 패턴 안에서 실행할 스킬 한 단계입니다. 시작 오프셋과 반복 간격은 PlayClock 초 기준이며 실제 타임라인 실행이 필요합니다.", ShortToolTip="보스 패턴 안에서 실행할 스킬 한 단계입니다."))
struct ISEKAICLASS_API FICBossPatternStep
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ToolTip="Skill DT ID입니다. 조회만으로 스킬이 실행되지 않습니다. 실제 GAS/타격/비용 확정 경로를 연결해야 합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName SkillID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ClampMin="0", ToolTip="작업/스킬/패턴 시작 기준 오프셋 초입니다. 절대 PlayClock 시각과 구분하고 실제 타임라인 이벤트를 맞춥니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double StartOffsetPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ClampMin="1", ToolTip="단계/타격 반복 횟수입니다. 양수이며 같은 반복의 타격 번호/이벤트 중복 검증을 연결합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 RepeatCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ClampMin="0", ToolTip="반복 타격/단계 사이 PlayClock 초 간격입니다. 0은 같은 시간 기준 반복 규격이며 실제 중복 검증을 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double RepeatIntervalPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step", meta=(ToolTip="선택 몽타주 내부 섹션 이름입니다. 실제 에셋에 존재해야 하며 정의 ID가 아닙니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName MontageSection;
};

USTRUCT(BlueprintType, meta=(ToolTip="보스 패턴 시작을 기준으로 무적이 켜지는 시간 구간입니다. 시작보다 종료가 빨라서는 안 됩니다. 중단 시 무적 종료 보장은 전투 실행 측에서 처리합니다.", ShortToolTip="보스 패턴 시작을 기준으로 무적이 켜지는 시간 구간입니다."))
struct ISEKAICLASS_API FICPatternInvulnerabilityWindow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Window", meta=(ClampMin="0", ToolTip="작업/스킬/패턴 시작 기준 오프셋 초입니다. 절대 PlayClock 시각과 구분하고 실제 타임라인 이벤트를 맞춥니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double StartOffsetPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Window", meta=(ClampMin="0", ToolTip="기준 시작 뒤 종료 오프셋 초입니다. StartOffsetPlaySeconds 이상이어야 합니다. 실제 중단에서도 종료 처리를 한 번 실행해야 합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) double EndOffsetPlaySeconds = 0.0;
};

/** Boss pattern definition. Phase indices, running step and invulnerability state are runtime data. */
USTRUCT(BlueprintType, meta=(ToolTip="보스 패턴의 선택 조건·시간·쿨타임·스킬 단계·무적 구간·경고 연출입니다. 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.", ShortToolTip="보스 패턴의 선택 조건·시간·쿨타임·스킬 단계·무적 구간·경고 연출입니다."))
struct ISEKAICLASS_API FICBossPatternRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="Monster DT의 정의 ID입니다. 스폰 슬롯의 개별 인스턴스 키와 구분합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) FName MonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="패턴 선택 추가 조건입니다. 실제 거리·쿨타임과 함께 검사하도록 정의합니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) FICConditionGroup SelectionConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0", ToolTip="최소 허용 거리 cm입니다. 0은 양수 최소 거리 요구가 없는 규격이며 MaximumRangeCm와 맞춥니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) double MinimumRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0", ToolTip="허용 최대 거리 cm입니다. 최소 거리보다 작지 않게 작성하고 0의 제한 해제 여부는 실제 대상/패턴 실행 규칙을 확인합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) double MaximumRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0", ToolTip="보스 경고 후 실행 지연 초입니다. 실제 경고 표시와 패턴 타임라인 연결이 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) double TelegraphPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0", ToolTip="보스 패턴 전체 PlayClock 초입니다. 단계/무적 종료가 이 범위 안에 있도록 작성하며 실제 전투 타임라인 실행은 별도입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) double DurationPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ClampMin="0", ToolTip="스킬/패턴 재사용 지연 PlayClock 초입니다. 실제 캐시/실행/복원을 연결해야 합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) double CooldownPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="방어 불가 보스 패턴 정의입니다. 실제 개별 타격 규칙/방어 판정 실행을 맞춥니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) bool bUnblockable = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="페이즈 변화 시 남은 패턴 취소 정의입니다. 실제 타격/무적/연출 종료를 함께 정리해야 합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) bool bCancelOnPhaseChange = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="강화 단계 또는 보스 패턴 단계 목록입니다. 강화는 목표 레벨별, 패턴은 시간/Skill별 규칙을 작성합니다.\n입력 자료형: 구조/객체 (FICBossPatternStep) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) TArray<FICBossPatternStep> Steps;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pattern", meta=(ToolTip="패턴 기준 무적 시간 구간 목록입니다. 실제 시작/종료/중단 이벤트 수신 연결이 필요합니다.\n입력 자료형: 구조/객체 (FICPatternInvulnerabilityWindow) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) TArray<FICPatternInvulnerabilityWindow> InvulnerabilityWindows;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ToolTip="AnimMontage입니다. 스켈레톤·섹션·Notify와 실제 재생 경로를 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (UAnimMontage).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) TSoftObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="보스 경고 String Table 키입니다. 원문과 실제 경고 UI 해석을 연결합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) FString WarningKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="보스 경고 Texture2D 아이콘입니다. 실제 경고 UI에 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) TSoftObjectPtr<UTexture2D> WarningIcon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="보스 경고 NiagaraSystem입니다. 판정 영역/시점과 실제 실행을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UNiagaraSystem).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) TSoftObjectPtr<UNiagaraSystem> TelegraphVFX;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="보스 경고 SoundBase입니다. 실제 패턴 예고 시점에 재생을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 정의 조회/검사는 지원하지만 패턴 전투 실행기는 아직 연결되지 않았습니다.")) TSoftObjectPtr<USoundBase> TelegraphSound;
};

/** Direct monster/boss definition; spawner rows own per-instance origin, slot and chase area. */
USTRUCT(BlueprintType, meta=(ToolTip="몬스터의 전투·탐지·추격·스킬·드롭·페이즈·외형 정의입니다. 처치 시각·리스폰 상태는 WorldState에 저장하고 액터가 원본을 소유하지 않도록 설계합니다. 실제 몬스터 실행 연결은 남아 있습니다.", ShortToolTip="몬스터의 전투·탐지·추격·스킬·드롭·페이즈·외형 정의입니다."))
struct ISEKAICLASS_API FICMonsterRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster", meta=(ToolTip="정의/아이템/효과 분류입니다. 선택한 종류가 요구하는 참조/수치/대상 규칙을 함께 작성합니다.\n입력 자료형: 선택 목록 (EICMonsterKind).\nC++ 초기값: Normal — 일반 몬스터입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) EICMonsterKind Kind = EICMonsterKind::Normal;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster", meta=(ToolTip="몬스터 전투 기본 수치 묶음입니다. 플레이어 6대 기본 능력치와 별개이며 실제 액터/GAS 적용 연결이 필요합니다.\n입력 자료형: 구조/객체 (FICMonsterCombatStats).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) FICMonsterCombatStats CombatStats;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0", ToolTip="몬스터 탐지 거리 cm입니다. 시야각/가림 검사와 실제 AI에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) double DetectionRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0", ClampMax="360", ToolTip="몬스터 탐지 시야 총 각도입니다. 도 단위이며 거리/가림/실제 AI 판정을 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 360.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) double SightAngleDegrees = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ToolTip="몬스터 탐지 가림 검사 요구입니다. 실제 AI 시야 실행 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) bool bRequiresLineOfSight = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0", ToolTip="몬스터 공격 사거리 cm입니다. 실제 타깃/거리/시야 검사와 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) double AttackRangeCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0", ToolTip="몬스터 공격 시도 간격 PlayClock 초입니다. 실제 AI/GAS 공격 실행은 별도 연결 대상입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) double AttackIntervalPlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0", ToolTip="경로 실패 판단 PlayClock 초입니다. 실제 AI 경로 상태 실행을 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) double PathFailurePlaySeconds = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0", ToolTip="몬스터 원점 복귀 완료 반지름 cm입니다. 실제 AI 이동/리셋 실행에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) double ReturnAcceptanceRadiusCm = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ToolTip="몬스터 Skill ID 후보/가중치/조건 목록입니다. 실제 AI/GAS 선택 실행은 별도입니다.\n입력 자료형: 구조/객체 (FICMonsterSkillChoice) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TArray<FICMonsterSkillChoice> Skills;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ToolTip="몬스터 BehaviorTree입니다. Blackboard·AIController와 키/실행 연결을 맞춥니다.\n입력 자료형: Soft Object 에셋 참조 (UBehaviorTree).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TSoftObjectPtr<UBehaviorTree> BehaviorTree;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ToolTip="몬스터 BlackboardData입니다. BehaviorTree의 키 이름/자료형을 맞춥니다.\n입력 자료형: Soft Object 에셋 참조 (UBlackboardData).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TSoftObjectPtr<UBlackboardData> Blackboard;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="0", ToolTip="몬스터 재생 PlayClock 초입니다. 메뉴 중 경과하지 않으며 슬롯 덮어쓰기 설정/실제 스폰 원본과 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 90.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) double RespawnPlaySeconds = 90.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reward", meta=(ToolTip="DropTable DT ID입니다. 지속 드롭 원본과 별개이며 전투 추첨 실행 연결이 필요합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) FName DropTableID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reward", meta=(ToolTip="최초 처치 Reward ID입니다. 완료 영수증으로 중복 지급을 막는 실행 연결이 필요합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) FName FirstKillRewardID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reward", meta=(ToolTip="반복 처치 Reward ID입니다. 최초 보상과 중복 지급하지 않도록 실제 경로를 연결합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) FName RepeatKillRewardID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ToolTip="일반 피격 경직 면역입니다. 실제 특수 제어/피해와 구분하는 전투 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) bool bImmuneToOrdinaryHitStagger = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ToolTip="보스 전투 진입 조건입니다. 지역/보스 실행 측에서 검사합니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) FICConditionGroup BossEntryConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ToolTip="보스 전투 종료/이탈 조건입니다. 상태 캐시 반영과 종료 실행을 함께 연결합니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) FICConditionGroup BossExitConditions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ToolTip="보스 페이즈 목록입니다. 단계 번호·진입 조건·패턴을 작성하고 현재 페이즈 상태는 런타임에서 관리합니다.\n입력 자료형: 구조/객체 (FICBossPhaseDefinition) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TArray<FICBossPhaseDefinition> BossPhases;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="몬스터 APawn 자식 BP 클래스입니다. 실제 스폰/AI/GAS 초기화 연결이 필요합니다.\n입력 자료형: Soft Class 에셋 참조 (APawn).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TSoftClassPtr<APawn> PawnClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="메시 에셋 또는 표시 컴포넌트입니다. 필드 타입에 맞게 연결하고 스켈레톤·크기·축을 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (USkeletalMesh).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="AnimInstance 자식 Anim BP 클래스입니다. 메시 스켈레톤과 호환되어야 합니다.\n입력 자료형: Soft Class 에셋 참조 (UAnimInstance).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TSoftClassPtr<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="Texture2D 초상화입니다. 게임 메시와 별개이며 실제 표시 UI에서 로드합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ToolTip="몬스터 전투 배경음입니다. 진입/종료와 음악 전환 실행을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 실제 몬스터 실행 연결은 남아 있습니다.")) TSoftObjectPtr<USoundBase> BattleBGM;
};
