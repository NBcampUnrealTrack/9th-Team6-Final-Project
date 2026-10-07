#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "World/State/Types/ICCombatStateTypes.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICCharacterStateTypes.generated.h"

UENUM(BlueprintType, meta=(ToolTip="WorldState에서 직접 조회·변경할 6대 기본 능력치입니다. 최종 전투 파생값과 구분합니다."))
enum class EICMainStat : uint8 { STR UMETA(ToolTip="힘입니다.\nWorldState에서 직접 조회·변경할 6대 기본 능력치입니다. 최종 전투 파생값과 구분합니다."), DEX UMETA(ToolTip="민첩입니다.\nWorldState에서 직접 조회·변경할 6대 기본 능력치입니다. 최종 전투 파생값과 구분합니다."), INT UMETA(ToolTip="지능입니다.\nWorldState에서 직접 조회·변경할 6대 기본 능력치입니다. 최종 전투 파생값과 구분합니다."), CON UMETA(ToolTip="체력입니다.\nWorldState에서 직접 조회·변경할 6대 기본 능력치입니다. 최종 전투 파생값과 구분합니다."), CHA UMETA(ToolTip="매력입니다.\nWorldState에서 직접 조회·변경할 6대 기본 능력치입니다. 최종 전투 파생값과 구분합니다."), LUK UMETA(ToolTip="행운입니다.\nWorldState에서 직접 조회·변경할 6대 기본 능력치입니다. 최종 전투 파생값과 구분합니다.") };

UENUM(BlueprintType, meta=(ToolTip="지속 캐릭터 상태의 주체 분류입니다. NPC 정의와 유형이 일치해야 합니다."))
enum class EICCharacterKind : uint8 { Player UMETA(ToolTip="플레이어입니다. 고정 ID는 CHR_PLAYER입니다.\n지속 캐릭터 상태의 주체 분류입니다. NPC 정의와 유형이 일치해야 합니다."), Recruitable UMETA(ToolTip="영입 가능한 NPC입니다. 성장 능력치·자원·전투 정의를 사용합니다.\n지속 캐릭터 상태의 주체 분류입니다. NPC 정의와 유형이 일치해야 합니다."), Resident UMETA(ToolTip="일반 거주 NPC입니다.\n지속 캐릭터 상태의 주체 분류입니다. NPC 정의와 유형이 일치해야 합니다."), Background UMETA(ToolTip="배경 NPC입니다.\n지속 캐릭터 상태의 주체 분류입니다. NPC 정의와 유형이 일치해야 합니다.") };

UENUM(BlueprintType, meta=(ToolTip="NPC의 현재 일과/동료 모드입니다. 모드 전환 전 실행 캐시를 WorldState에 반영해야 합니다."))
enum class EICCharacterMode : uint8 { Scheduled UMETA(ToolTip="자신의 일과를 따르는 모드입니다.\nNPC의 현재 일과/동료 모드입니다. 모드 전환 전 실행 캐시를 WorldState에 반영해야 합니다."), Companion UMETA(ToolTip="플레이어 파티의 동료 모드입니다. 실제 PartySlot과 파티 한도를 함께 검사합니다.\nNPC의 현재 일과/동료 모드입니다. 모드 전환 전 실행 캐시를 WorldState에 반영해야 합니다."), Downed UMETA(Hidden, ToolTip="이전 v2 저장 데이터의 숫자값을 읽기 위한 호환 항목입니다. 새 NPC 모드나 새 DT에는 사용하지 마세요. 현재 동료의 쓰러짐은 CharacterLifeState.Downed에서 관리하고 모드는 Scheduled/Companion을 사용합니다.") };

UENUM(BlueprintType, meta=(ToolTip="캐릭터 지속 상태의 생존 분류입니다. 플레이어 컨트롤러의 사망 단계와 구분합니다."))
enum class EICCharacterLifeState : uint8 { Alive UMETA(ToolTip="살아 있는 상태입니다.\n캐릭터 지속 상태의 생존 분류입니다. 플레이어 컨트롤러의 사망 단계와 구분합니다."), Downed UMETA(ToolTip="쓰러진 동료 상태입니다. 부활 가능한 대상인지 함께 검사합니다.\n캐릭터 지속 상태의 생존 분류입니다. 플레이어 컨트롤러의 사망 단계와 구분합니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="WorldState가 저장하는 캐릭터 기본 능력치 6종입니다. 성장 값을 이 원본에 반영하고 장비/효과를 적용한 최종값과 구분합니다.", ShortToolTip="WorldState가 저장하는 캐릭터 기본 능력치 6종입니다."))
struct FICMainStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="힘 기본 능력치입니다. 정수 성장 원본이며 파생 물리 공격력과 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 STR = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="민첩 기본 능력치입니다. 속도·회피·치명타 공식 원본이며 최종 비율과 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 DEX = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="지능 기본 능력치입니다. MP/마법 공격 공식의 원본입니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 INT = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="체력 기본 능력치입니다. HP/SP 상한·방어 공식의 원본입니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 CON = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="매력 기본 능력치입니다. 개인 효과/동료/관계 보정에 사용하는 원본이며 중복 보정하지 않아야 합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 CHA = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="행운 기본 능력치입니다. 드롭/골드/치명타 보정 원본이며 실제 추첨 실행은 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 LUK = 1;
    bool IsValid() const;
    bool Set(EICMainStat Stat, int32 Value);
};

USTRUCT(BlueprintType, meta=(ToolTip="구형 디버프 저장 형식을 읽기 위한 호환 상태입니다. 현재 활성 효과 원본은 Combat.Effects이며 로드 시 호환 변환을 수행합니다.", ShortToolTip="구형 디버프 저장 형식을 읽기 위한 호환 상태입니다."))
struct FICDebuffState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="구형 저장의 디버프 식별 키입니다. 로드 변환용이며 새 효과는 Combat.Effects의 EffectID/InstanceID를 사용합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 활성 효과 원본은 Combat.Effects이며 로드 시 호환 변환을 수행합니다.")) FName ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="옛 디버프 남은 시간 호환 값입니다. 현재 효과 만료 PlayClock 원본으로 변환하며 새 상태 원본으로 쓰지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 활성 효과 원본은 Combat.Effects이며 로드 시 호환 변환을 수행합니다.")) double RemainingSeconds = 0;
};

USTRUCT(BlueprintType, meta=(ToolTip="플레이어/영입/주민/배경 캐릭터의 지속 상태입니다. 원본 ID·관계·기본 능력치·HP/MP/SP·모드·위치·전투 캐시를 저장합니다. 플레이어 장비는 Inventory가 원본이므로 이 구조의 Equipment는 비워 둡니다.", ShortToolTip="플레이어/영입/주민/배경 캐릭터의 지속 상태입니다."))
struct FICCharacterState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="지속 캐릭터 ID입니다. 플레이어 CHR_PLAYER 또는 NPC_R00_001~NPC_R08_999 규격을 사용하며 _000은 금지입니다. 영입/주민/배경 번호 범위도 종류와 맞춥니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName CharacterId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="정의/아이템/효과 분류입니다. 선택한 종류가 요구하는 참조/수치/대상 규칙을 함께 작성합니다.\n입력 자료형: 선택 목록 (EICCharacterKind).\nC++ 초기값: Recruitable — 영입 가능한 NPC입니다. 성장 능력치·자원·전투 정의를 사용합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICCharacterKind Kind = EICCharacterKind::Recruitable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 NPC Scheduled/Companion 모드입니다. 실제 모드 전환 API로 파티 슬롯·생존/장비 상태를 함께 검증합니다.\n입력 자료형: 선택 목록 (EICCharacterMode).\nC++ 초기값: Scheduled — 자신의 일과를 따르는 모드입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICCharacterMode Mode = EICCharacterMode::Scheduled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="캐릭터 Alive/Downed 원본입니다. Downed는 영입 동료 부활 대상으로 HP 0과 함께 검사합니다. 플레이어 사망 메뉴 상태는 Controller.bDead와 별개입니다.\n입력 자료형: 선택 목록 (EICCharacterLifeState).\nC++ 초기값: Alive — 살아 있는 상태입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICCharacterLifeState LifeState = EICCharacterLifeState::Alive;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 동료 슬롯 0~3입니다. -1은 파티 미참여이며 같은 슬롯 중복/모드/종류를 검사합니다.\n입력 자료형: 정수.\nC++ 초기값: -1(미지정). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 PartySlot = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="NPC 현재 관계 정수 또는 보상 관계 변화 목록입니다. 현재값은 0~100, 보상 목록은 NPC별 증감량입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Affinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="기본 능력치 6종의 지속/표시 원본입니다. 장비/효과 최종 파생값과 구분하며 변경은 상태 API를 사용합니다.\n입력 자료형: 구조/객체 (FICMainStats).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FICMainStats MainStats;
    // Current resources are durable; maxima are compatibility caches, rebuilt from definitions/effects.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 체력입니다. 0~MaxHP이며 LifeState와 함께 검증합니다.\n입력 자료형: 소수.\nC++ 초기값: 60. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float HP = 60;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 마나입니다. 0~MaxMP이며 실제 스킬 비용 확정은 전투 경로와 연결합니다.\n입력 자료형: 소수.\nC++ 초기값: 100. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 생활 자원입니다. 0~MaxSP이며 표시에서는 내림 정수로 보여줍니다. 실제 플레이 경과 10초당 1이 감소합니다.\n입력 자료형: 소수.\nC++ 초기값: 70. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float SP = 70;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="체력 상한 또는 장비 HP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 100. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MaxHP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="마나 상한 또는 장비 MP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 100. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MaxMP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="생활 자원 상한 또는 장비 SP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 100. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MaxSP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="캐릭터 자원을 마지막 처리한 누적 게임 분입니다. 미스폰 경과/수면 등 중복 처리를 막는 저장 기준입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 480. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double LastSimGameMinute = 480;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="캐릭터/몬스터의 마지막 PlayClock 처리 시각입니다. 오프라인 UTC와 구분하며 현재 시계보다 미래 값을 만들지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double LastSimPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="다음 SP 감소까지 누적한 부분 PlayClock 초입니다. 0 이상 10 미만을 유지하고 저장/로드 때 보존해 타이머 초기화로 SP 감소가 밀리지 않게 합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double SPDrainElapsedPlaySeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 캐릭터의 Anchor ID 기록입니다. 최초 위치 정의와 이동 후 원본을 구분하며 실제 일과/이동 실행에서 갱신합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName CurrentAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="캐릭터/드롭의 월드 위치 cm입니다. 유한 좌표를 사용하고 실제 이동/저장 시 원본에 반영합니다.\n입력 자료형: 3차원 좌표/크기.\nC++ 초기값: FVector::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="월드 회전 도입니다. 위치와 함께 저장/복원하며 메시 상대 회전과 구분합니다.\n입력 자료형: 회전 각도.\nC++ 초기값: FRotator::ZeroRotator. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="실제 해금 Skill ID 집합입니다. NPC 초기 고유 스킬을 등록하며 전체 스킬 실행/해금 연결은 별도입니다.\n입력 자료형: 이름/ID 중복 없는 집합.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TSet<FName> UnlockedSkillIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 NPC 공격 성향 원본입니다. 초기 정의와 별개로 변경값을 저장하며 실제 AI 실행을 연결합니다.\n입력 자료형: 선택 목록 (EICAttackDisposition).\nC++ 초기값: Defensive — 방어·지원 우선 규칙으로 공격하는 성향입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICAttackDisposition AttackDisposition = EICAttackDisposition::Defensive;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 저체력 동료 행동 원본입니다. Attack/Wait 규칙과 실제 AI 실행을 연결합니다.\n입력 자료형: 선택 목록 (EICLowHPDisposition).\nC++ 초기값: Wait — 낮은 HP에서 공격을 대기하는 성향입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICLowHPDisposition LowHPDisposition = EICLowHPDisposition::Wait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 NPC 적 선택 우선순위입니다. 플레이어 타깃 우선/근처 우선을 실제 AI에 연결합니다.\n입력 자료형: 선택 목록 (EICTargetPriority).\nC++ 초기값: PlayerTargetFirst — 플레이어가 지정한 적을 우선합니다. 해당 적이 유효하지 않을 때의 대체 선택도 실행 측에서 처리해야 합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) EICTargetPriority TargetPriority = EICTargetPriority::PlayerTargetFirst;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 NPC 자동 스킬 허용 원본입니다. 실제 AI/GAS 사용 판정과 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) bool bAutoSkillsEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="효과/쿨타임/캡처 시각의 전투 캐시입니다. 활성 참가자 상태를 반영한 뒤 저장/제거에 사용하며 GAS 실행 복원은 별도입니다.\n입력 자료형: 구조/객체 (FICCombatState).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FICCombatState Combat;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="장비 인스턴스 목록입니다. 플레이어 원본은 Inventory의 네 슬롯이고 CharacterState의 플레이어 Equipment는 비워 둡니다. NPC 장비는 CharacterState에 저장합니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="구형 디버프 호환 목록입니다. 현재 효과 캐시는 Combat.Effects이며 로드 변환 후 실제 GAS 복원 경로를 사용합니다.\n입력 자료형: 구조/객체 (FICDebuffState) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<FICDebuffState> Debuffs;
    bool IsDowned() const { IC_TRACE_FUNCTION(); return LifeState == EICCharacterLifeState::Downed; }
    bool HasStats() const { IC_TRACE_FUNCTION(); return Kind == EICCharacterKind::Player || Kind == EICCharacterKind::Recruitable; }
};
