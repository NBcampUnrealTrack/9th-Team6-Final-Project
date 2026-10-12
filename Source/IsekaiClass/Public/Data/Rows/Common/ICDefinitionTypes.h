#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ICDefinitionTypes.generated.h"

UENUM(BlueprintType, meta=(ToolTip="정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."))
enum class EICDefinitionType : uint8
{
    Settings UMETA(ToolTip="새 게임·시간·능력치·전투·인벤토리·파티·지도·저장 설정 DT의 공통 분류입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), PlayerPreset UMETA(ToolTip="플레이어 외형 프리셋 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Class UMETA(ToolTip="직업과 시작 무기·공격·스킬 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), NPC UMETA(ToolTip="NPC 이름·일과·거주·외형의 정적 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), RecruitCombat UMETA(ToolTip="영입 NPC의 초기 전투 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Skill UMETA(ToolTip="스킬 비용·대상·타격·취소 규칙 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), SkillEffect UMETA(ToolTip="스킬 효과 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), StatusEffect UMETA(ToolTip="지속 상태 효과 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."),
    Synergy UMETA(ToolTip="파티 조합 시너지 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Monster UMETA(ToolTip="몬스터 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), BossPattern UMETA(ToolTip="보스 패턴 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Region UMETA(ToolTip="지역 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Anchor UMETA(ToolTip="월드의 의미 있는 위치 앵커 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Spawner UMETA(ToolTip="몬스터 스폰 슬롯 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Warp UMETA(ToolTip="워프 발견·목적지 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Home UMETA(ToolTip="집·소유·시설 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Item UMETA(ToolTip="인벤토리 아이템 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Equipment UMETA(ToolTip="장비 수치·강화·옵션 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."),
    Enhancement UMETA(ToolTip="강화 단계별 규칙 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Option UMETA(ToolTip="장비 옵션 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), OptionPool UMETA(ToolTip="장비 등급별 옵션 후보 풀 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Reward UMETA(ToolTip="보상 묶음 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), DropTable UMETA(ToolTip="드롭 선택 규칙 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Schedule UMETA(ToolTip="NPC 하루 일과 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), GiftPreference UMETA(ToolTip="NPC 선물 선호 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."),
    Dialogue UMETA(ToolTip="String Table 키를 참조하는 대사 그래프 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Service UMETA(ToolTip="상점·강화·워프·여관 서비스 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Shop UMETA(ToolTip="상점 판매 목록 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Recipe UMETA(ToolTip="생산 레시피 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Production UMETA(ToolTip="생산 작업 규칙 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Resource UMETA(ToolTip="벌목·채광·채집 자원 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Crop UMETA(ToolTip="작물 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Fish UMETA(ToolTip="물고기 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), FishingPoint UMETA(ToolTip="낚시터 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."),
    Quest UMETA(ToolTip="퀘스트 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Progression UMETA(ToolTip="메인 퀘스트·해금·게임 진행 연결 정의입니다. 콘텐츠 경로는 Quest 분류를 사용합니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Ending UMETA(ToolTip="엔딩 조건·연출 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Codex UMETA(ToolTip="도감 항목 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Landmark UMETA(ToolTip="명소 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Map UMETA(ToolTip="지도 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), Hint UMETA(ToolTip="첫 행동 안내 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), ResourceReference UMETA(ToolTip="외부 리소스와 사용 출처 정의입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요."), IDRegistry UMETA(ToolTip="정의 ID 등록부입니다.\n정적 정의를 조회할 DT 종류입니다. Settings 안에는 여러 설정 RowStruct가 있으므로 정확한 종류별 조회 함수를 사용하세요.")
};

UENUM(BlueprintType, meta=(ToolTip="능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."))
enum class EICStat : uint8
{
    None UMETA(ToolTip="능력치 대상을 지정하지 않습니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), STR UMETA(ToolTip="힘 기본 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), DEX UMETA(ToolTip="민첩 기본 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), INT UMETA(ToolTip="지능 기본 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), CON UMETA(ToolTip="체력 기본 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), CHA UMETA(ToolTip="매력 기본 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), LUK UMETA(ToolTip="행운 기본 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), MaxHP UMETA(ToolTip="최대 HP입니다. 현재 HP와 구분합니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), MaxMP UMETA(ToolTip="최대 MP입니다. 현재 MP와 구분합니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), MaxSP UMETA(ToolTip="최대 SP입니다. 현재 SP와 구분합니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), PhysicalAttack UMETA(ToolTip="물리 공격 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."),
    MagicAttack UMETA(ToolTip="마법 공격 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), Defense UMETA(ToolTip="방어 능력치입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), AttackSpeed UMETA(ToolTip="공격 속도 능력치입니다. 가산 비율과 최종 배수를 구분하세요.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), MoveSpeed UMETA(ToolTip="이동 속도 능력치입니다. 최종 월드 속도는 cm/초를 사용합니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), CriticalChance UMETA(ToolTip="치명타 확률입니다. 사용하는 필드의 비율/퍼센트포인트 단위를 확인하세요.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), CriticalMultiplier UMETA(ToolTip="치명타 피해 배수입니다. 1.5는 기본 피해의 1.5배입니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요."), AutoDodgeChance UMETA(ToolTip="자동 회피 확률입니다. 입력 회피 동작과 구분합니다.\n능력치 변경 대상입니다. 성장 기본값·장비 보정·파생값을 구분하고 비율 단위는 사용하는 필드 설명을 따르세요.")
};

UENUM(BlueprintType, meta=(ToolTip="직업 선택입니다. 생성 화면 인덱스는 전사0·궁수1·마법사2·힐러3 순서이고 enum 숫자 자체와 혼용하지 마세요."))
enum class EICCharacterClass : uint8 { None UMETA(ToolTip="직업을 지정하지 않은 상태입니다.\n직업 선택입니다. 생성 화면 인덱스는 전사0·궁수1·마법사2·힐러3 순서이고 enum 숫자 자체와 혼용하지 마세요."), Warrior UMETA(ToolTip="전사입니다. 직업 정의와 검 무기 계열을 확인하세요.\n직업 선택입니다. 생성 화면 인덱스는 전사0·궁수1·마법사2·힐러3 순서이고 enum 숫자 자체와 혼용하지 마세요."), Archer UMETA(ToolTip="궁수입니다. 직업 정의와 활 무기 계열을 확인하세요.\n직업 선택입니다. 생성 화면 인덱스는 전사0·궁수1·마법사2·힐러3 순서이고 enum 숫자 자체와 혼용하지 마세요."), Mage UMETA(ToolTip="마법사입니다. 직업 정의와 지팡이 무기 계열을 확인하세요.\n직업 선택입니다. 생성 화면 인덱스는 전사0·궁수1·마법사2·힐러3 순서이고 enum 숫자 자체와 혼용하지 마세요."), Healer UMETA(ToolTip="힐러입니다. 직업 정의와 완드 무기 계열을 확인하세요.\n직업 선택입니다. 생성 화면 인덱스는 전사0·궁수1·마법사2·힐러3 순서이고 enum 숫자 자체와 혼용하지 마세요.") };
UENUM(BlueprintType, meta=(ToolTip="무기의 계열 분류입니다. 직업의 허용 계열과 장비의 계열을 함께 검사합니다."))
enum class EICWeaponFamily : uint8 { None UMETA(ToolTip="무기 계열이 없는 장비 또는 미지정 값입니다.\n무기의 계열 분류입니다. 직업의 허용 계열과 장비의 계열을 함께 검사합니다."), Sword UMETA(ToolTip="검 계열입니다.\n무기의 계열 분류입니다. 직업의 허용 계열과 장비의 계열을 함께 검사합니다."), Bow UMETA(ToolTip="활 계열입니다.\n무기의 계열 분류입니다. 직업의 허용 계열과 장비의 계열을 함께 검사합니다."), Staff UMETA(ToolTip="지팡이 계열입니다.\n무기의 계열 분류입니다. 직업의 허용 계열과 장비의 계열을 함께 검사합니다."), Wand UMETA(ToolTip="완드 계열입니다.\n무기의 계열 분류입니다. 직업의 허용 계열과 장비의 계열을 함께 검사합니다.") };
UENUM(BlueprintType, meta=(ToolTip="NPC 정적 콘텐츠 분류입니다. NPC_R0[0~8]_번호 ID의 번호 구간과 일치해야 합니다."))
enum class EICNPCType : uint8 { Recruitable UMETA(ToolTip="영입 가능 NPC입니다. 번호001~099를 사용합니다.\nNPC 정적 콘텐츠 분류입니다. NPC_R0[0~8]_번호 ID의 번호 구간과 일치해야 합니다."), Resident UMETA(ToolTip="일반 거주 NPC입니다. 번호101~199를 사용합니다.\nNPC 정적 콘텐츠 분류입니다. NPC_R0[0~8]_번호 ID의 번호 구간과 일치해야 합니다."), Background UMETA(ToolTip="배경 NPC입니다. 번호201~299를 사용합니다.\nNPC 정적 콘텐츠 분류입니다. NPC_R0[0~8]_번호 ID의 번호 구간과 일치해야 합니다.") };
UENUM(BlueprintType, meta=(ToolTip="시간 값이 사용하는 시계입니다. 두 시계 모두 메뉴 정지 동안 멈춥니다."))
enum class EICClock : uint8 { GameClock UMETA(ToolTip="누적 게임 분입니다. 일과·수면·농사 등 게임 안의 시각을 사용하며 수면 점프가 반영됩니다.\n시간 값이 사용하는 시계입니다. 두 시계 모두 메뉴 정지 동안 멈춥니다."), PlayClock UMETA(ToolTip="누적 실제 플레이 초입니다. 생산·쿨타임·드롭 수명처럼 활동한 시간을 사용하며 수면 점프로 늘리지 않습니다.\n시간 값이 사용하는 시계입니다. 두 시계 모두 메뉴 정지 동안 멈춥니다.") };
UENUM(BlueprintType, meta=(ToolTip="조건 값과 기준값을 비교하는 연산입니다."))
enum class EICComparison : uint8 { Equal UMETA(ToolTip="값이 기준값과 같을 때 만족합니다.\n조건 값과 기준값을 비교하는 연산입니다."), NotEqual UMETA(ToolTip="값이 기준값과 다를 때 만족합니다.\n조건 값과 기준값을 비교하는 연산입니다."), Greater UMETA(ToolTip="값이 기준값보다 클 때 만족합니다.\n조건 값과 기준값을 비교하는 연산입니다."), GreaterOrEqual UMETA(ToolTip="값이 기준값 이상일 때 만족합니다.\n조건 값과 기준값을 비교하는 연산입니다."), Less UMETA(ToolTip="값이 기준값보다 작을 때 만족합니다.\n조건 값과 기준값을 비교하는 연산입니다."), LessOrEqual UMETA(ToolTip="값이 기준값 이하일 때 만족합니다.\n조건 값과 기준값을 비교하는 연산입니다.") };
UENUM(BlueprintType, meta=(ToolTip="같은 조건 그룹 안의 결합 방식입니다. 빈 그룹 처리와 상위 그룹의 결합은 호출 실행 측의 규칙을 확인하세요."))
enum class EICConditionPolicy : uint8 { All UMETA(ToolTip="그룹 안의 조건을 모두 만족해야 합니다.\n같은 조건 그룹 안의 결합 방식입니다. 빈 그룹 처리와 상위 그룹의 결합은 호출 실행 측의 규칙을 확인하세요."), Any UMETA(ToolTip="그룹 안의 조건을 하나 이상 만족해야 합니다.\n같은 조건 그룹 안의 결합 방식입니다. 빈 그룹 처리와 상위 그룹의 결합은 호출 실행 측의 규칙을 확인하세요.") };
UENUM(BlueprintType, meta=(ToolTip="검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."))
enum class EICConditionType : uint8
{
    None UMETA(ToolTip="조건 종류를 지정하지 않았습니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), QuestAccepted UMETA(ToolTip="Target의 퀘스트 수락 여부를 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), QuestCompleted UMETA(ToolTip="Target의 퀘스트 완료 여부를 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), ItemInBag UMETA(ToolTip="Target 아이템의 가방 수량을 기준값과 비교합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), Stat UMETA(ToolTip="지정 캐릭터의 Stat 능력치를 비교합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), Affinity UMETA(ToolTip="지정 NPC와의 관계 값을 비교합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), VisitedAnchor UMETA(ToolTip="앵커 방문 기록을 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."),
    BossDefeated UMETA(ToolTip="보스 처치 기록을 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), WarpUnlocked UMETA(ToolTip="워프 해금 기록을 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), RecipeUnlocked UMETA(ToolTip="레시피 해금 기록을 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), Flag UMETA(ToolTip="지정 플래그와 기준값을 비교합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), HasGameplayTag UMETA(ToolTip="필수 GameplayTag의 보유 여부를 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), CurrentHPRatio UMETA(ToolTip="현재 HP/최대 HP 비율을 비교합니다. 0.30은 30%입니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."),
    CurrentMPRatio UMETA(ToolTip="현재 MP/최대 MP 비율을 비교합니다. 0.30은 30%입니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), InCombat UMETA(ToolTip="현재 전투 여부를 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), TargetExists UMETA(ToolTip="현재 유효한 대상이 있는지 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), HasStatusEffect UMETA(ToolTip="지정 상태 효과를 보유했는지 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다."), CharacterMode UMETA(ToolTip="일과/동료 모드가 지정 값인지 검사합니다.\n검사할 조건의 종류입니다. 종류별 필요한 Target·Stat·Value·Flag·Tags를 작성합니다. 일반 조건 실행기는 아직 연결이 필요합니다.")
};

USTRUCT(BlueprintType, meta=(ToolTip="정의 종류와 ID를 묶은 참조입니다. Type이 가리키는 DT 종류 안에 ID가 존재해야 합니다. 서로 다른 종류의 ID를 섞으면 참조 검증에 실패합니다.", ShortToolTip="정의 종류와 ID를 묶은 참조입니다."))
struct ISEKAICLASS_API FICDefinitionRef
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICDefinitionType).\nC++ 초기값: Item — 인벤토리 아이템 정의입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICDefinitionType Type = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="Type이 가리키는 DT의 정의 ID입니다. 그 테이블의 RowName/ID와 같아야 합니다. None은 해당 참조가 선택 사항인 상위 규칙에서만 비워 둡니다.\n입력 자료형: 이름/ID.")) FName ID;
};

USTRUCT(BlueprintType, meta=(ToolTip="성장 전 초기 기본 능력치 6종입니다. 최소 1의 정수이며 장비·효과가 더해진 파생 능력치와 구분합니다. 실제 증가한 능력치는 WorldState에 보존합니다.", ShortToolTip="성장 전 초기 기본 능력치 6종입니다."))
struct ISEKAICLASS_API FICBaseStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000", ToolTip="힘 기본 능력치입니다. 정수 성장 원본이며 파생 물리 공격력과 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10000.")) int32 STR = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000", ToolTip="민첩 기본 능력치입니다. 속도·회피·치명타 공식 원본이며 최종 비율과 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10000.")) int32 DEX = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000", ToolTip="지능 기본 능력치입니다. MP/마법 공격 공식의 원본입니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10000.")) int32 INT = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000", ToolTip="체력 기본 능력치입니다. HP/SP 상한·방어 공식의 원본입니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10000.")) int32 CON = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000", ToolTip="매력 기본 능력치입니다. 개인 효과/동료/관계 보정에 사용하는 원본이며 중복 보정하지 않아야 합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10000.")) int32 CHA = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10000", ToolTip="행운 기본 능력치입니다. 드롭/골드/치명타 보정 원본이며 실제 추첨 실행은 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10000.")) int32 LUK = 1;
};

USTRUCT(BlueprintType, meta=(ToolTip="아이템 정의 ID와 정수 수량의 묶음입니다. 아이템 DT에 존재하는 ID와 양수 수량을 넣으세요. 인벤토리 인스턴스 GUID와 구분합니다.", ShortToolTip="아이템 정의 ID와 정수 수량의 묶음입니다."))
struct ISEKAICLASS_API FICItemAmount
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.")) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ToolTip="정수 수량입니다. 요구/지급 정의에서는 양수, 보유 인스턴스에서는 0이 빈 슬롯입니다. 음수 상태는 거부합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.")) int32 Count = 1;
};

USTRUCT(BlueprintType, meta=(ToolTip="하나의 능력치에 더할 고정값과 가산 비율입니다. Flat과 Percent를 분리해 기록하며 Percent 0.10은 10%입니다. 최종 적용 순서와 상한은 능력치 실행 측에서 결정합니다.", ShortToolTip="하나의 능력치에 더할 고정값과 가산 비율입니다."))
struct ISEKAICLASS_API FICStatModifier
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="효과/조건/옵션 능력치 종류입니다. 기본/파생 능력치를 구분하며 None 허용 여부는 상위 규칙을 확인합니다.\n입력 자료형: 선택 목록 (EICStat).\nC++ 초기값: None — 능력치 대상을 지정하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICStat Stat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="능력치에 더할 고정량입니다. 10이면 +10 기여이며 Percent와 분리 누적해 실제 파생 계산에 적용합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) double Flat = 0;
    // Additive percentage expressed as a ratio: 0.10 means +10 percent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="가산 비율입니다. 0.10=10%, 10=1000%입니다. 고정량과 분리 누적해 실제 파생 계산에서 한 번 적용합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) double Percent = 0;
};

USTRUCT(BlueprintType, meta=(ToolTip="정적 조건 하나입니다. Type에 따라 Target·Stat·Value·Flag·RequiredTags 중 필요한 필드가 달라집니다. 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.", ShortToolTip="정적 조건 하나입니다."))
struct ISEKAICLASS_API FICCondition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICConditionType).\nC++ 초기값: None — 조건 종류를 지정하지 않았습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) EICConditionType Type = EICConditionType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="조건이 참조할 정적 정의 종류/ID입니다. Type에 맞는 NPC/아이템/퀘스트/앵커 등을 선택하고 Flag/태그 조건은 별도 필드를 사용합니다.\n입력 자료형: 구조/객체 (FICDefinitionRef).\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) FICDefinitionRef Target;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="효과/조건/옵션 능력치 종류입니다. 기본/파생 능력치를 구분하며 None 허용 여부는 상위 규칙을 확인합니다.\n입력 자료형: 선택 목록 (EICStat).\nC++ 초기값: None — 능력치 대상을 지정하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) EICStat Stat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재값과 임계값의 비교 연산입니다. GreaterOrEqual이면 기준 이상을 요구합니다.\n입력 자료형: 선택 목록 (EICComparison).\nC++ 초기값: GreaterOrEqual — 값이 기준값 이상일 때 만족합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) EICComparison Compare = EICComparison::GreaterOrEqual;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="조건 비교 기준입니다. Type/Stat별 수량·능력치·관계·비율 단위가 다르므로 종류와 함께 작성합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) double Value = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="체크하면 장비/효과 반영 유효 능력치, 해제하면 기본 능력치를 비교하도록 정의합니다. 실제 유효 계산 연결이 필요합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) bool bUseEffectiveStats = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="퀘스트 수락 전부터 만족한 상태를 인정할지 설정합니다. Talk/Visit만 명시적으로 허용하며 모든 누적 목표를 소급 인정하는 규칙이 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) bool bAllowCurrentState = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="체크하면 조건 판정을 반대로 사용하도록 정의합니다. 수치 부호를 바꾸는 설정이 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) bool bNegate = false;
    // Flag keys are not definition IDs and are checked separately.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="게임 플래그 키입니다. 정의 ID가 아니며 실제 값은 WorldState에서 판정합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) FName Flag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="필요 GameplayTag 목록입니다. 등록된 태그와 실제 GAS/태그 판정 연결을 확인합니다.\n입력 자료형: GameplayTag 목록.\n소속 정의/시스템: 구조와 참조 검사는 제공되지만 모든 조건의 게임 실행 판정기는 아직 연결되어 있지 않습니다.")) FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType, meta=(ToolTip="Conditions의 조건들을 Policy에 따라 묶습니다. All은 모두 만족, Any는 하나 이상 만족입니다. 빈 조건의 허용 여부와 여러 그룹 사이의 결합 방식은 이 구조를 사용하는 상위 정의를 확인하세요.", ShortToolTip="Conditions의 조건들을 Policy에 따라 묶습니다."))
struct ISEKAICLASS_API FICConditionGroup
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="그룹 내부 결합 방식입니다. All=모두, Any=하나 이상입니다. 그룹 간 방식은 상위 정의를 확인합니다.\n입력 자료형: 선택 목록 (EICConditionPolicy).\nC++ 초기값: All — 그룹 안의 조건을 모두 만족해야 합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICConditionPolicy Policy = EICConditionPolicy::All;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="상위 동작 조건 묶음입니다. All/Any와 종류별 필드를 작성합니다. 일반 조건 실행기는 별도 연결 대상이며 정의 검사 성공은 실행 성공과 다릅니다.\n입력 자료형: 구조/객체 (FICCondition) 배열.")) TArray<FICCondition> Conditions;
};

USTRUCT(BlueprintType, meta=(ToolTip="모든 정적 DT 행의 공통 메타데이터입니다. RowName과 ID를 일치시키고 대문자·숫자·밑줄 40자 이내의 안정적인 ID를 사용하세요. 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.", ShortToolTip="모든 정적 DT 행의 공통 메타데이터입니다."))
struct ISEKAICLASS_API FICDefinitionRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="정적 정의의 안정적인 식별자입니다. DT의 RowName과 같아야 하며 대문자 A~Z·숫자·밑줄 40자 이내로 작성합니다. 사용 중인 ID를 바꾸면 저장/참조가 끊기므로 폐기·대체 관계를 관리하세요. NPC 예: NPC_R08_001.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.")) FName ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="표시 이름의 String Table 키입니다. 예: ITM_CONS_001.NAME. 해당 테이블에 원문을 등록하세요. 키 입력과 DisplayName FText의 String Table 연결은 별개이며 사용하는 화면이 키를 해석해야 표시됩니다.\n입력 자료형: 문자열.\n소속 정의/시스템: 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.")) FString NameKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="상세 설명의 String Table 키입니다. 문장 원문 대신 키를 작성하고 해당 테이블에 원문을 등록합니다. 사용하는 화면/노드의 키 해석 연결이 필요합니다.\n입력 자료형: 문자열.\n소속 정의/시스템: 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.")) FString DescriptionKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="분류·조건·효과 매칭용 GameplayTag 목록입니다. 프로젝트에 등록한 태그를 선택합니다. 빈 목록은 태그 없음이며 임의 문자열/정의 ID와 구분합니다.\n입력 자료형: GameplayTag 목록.\n소속 정의/시스템: 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.")) FGameplayTagContainer Tags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ClampMin="1", ToolTip="행 데이터의 규격 버전입니다. 1 이상의 정수이며 저장 파일 버전과 별개입니다. 변경만으로 저장 변환이 실행되지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.")) int32 DataVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="폐기된 정의 표시입니다. 체크하면 공통 런타임 조회가 거부합니다. 기존 참조/저장을 먼저 변환하고 등록부의 ReplacementID를 관리하세요.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.")) bool bDeprecated = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Definition", meta=(ToolTip="시험 콘텐츠 표시입니다. 체크한 정의는 공통 실제 플레이 조회에서 거부합니다. 출시 정의와 검사 데이터를 구분합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 자원·성장·진행 상태는 이 행에 기록하지 않고 WorldState에 저장합니다.")) bool bTestOnly = false;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

USTRUCT(BlueprintType, meta=(ToolTip="정의 ID의 종류·소속 DT·폐기와 대체 관계를 기록하는 등록부 행입니다. 실제 정의 내용은 DefinitionTable의 같은 ID 행에 있습니다.", ShortToolTip="정의 ID의 종류·소속 DT·폐기와 대체 관계를 기록하는 등록부 행입니다."))
struct ISEKAICLASS_API FICIDRegistryRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="등록부 ID의 DT 종류입니다. DefinitionTable 실제 RowStruct와 맞춰 작성합니다.\n입력 자료형: 선택 목록 (EICDefinitionType).\nC++ 초기값: Item — 인벤토리 아이템 정의입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICDefinitionType DefinitionType = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="폐기 ID의 대체 정의 ID입니다. 이 값만으로 저장/참조가 자동 변환되지 않으므로 실제 변환을 수행한 뒤 관리합니다.\n입력 자료형: 이름/ID.")) FName ReplacementID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="ID 등록부 항목이 실제로 정의된 DT 참조입니다. 같은 RowName/ID가 존재해야 하며 DefinitionType도 맞춥니다.\n입력 자료형: Soft Object 에셋 참조 (UDataTable).")) TSoftObjectPtr<UDataTable> DefinitionTable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="정의 등록부의 관리 설명입니다. ID 사용처/폐기/작성 근거를 기록하며 게임 상태/텍스트 원문이 아닙니다.\n입력 자료형: 문자열.")) FString Notes;
};
