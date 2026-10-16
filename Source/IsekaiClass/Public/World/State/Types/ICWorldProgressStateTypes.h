#pragma once
#include "CoreMinimal.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "World/State/Types/ICCombatStateTypes.h"
#include "ICWorldProgressStateTypes.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.", ShortToolTip="지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다."))
struct FICSpawnSlotState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,meta=(ToolTip="이번 생성 회차의 고유 GUID입니다. 처치 보상 영수증과 연결해 저장·로드·중복 사망 알림에서 같은 보상을 다시 지급하지 않습니다. 예전 저장에는 비어 있을 수 있으며 새 생성 시 발급합니다.")) FGuid IncarnationID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,meta=(ToolTip="처치 후 보상 지급이 정의 누락·가방 부족 등으로 보류되었는지 기록합니다. 원본 아이템·골드는 변경하지 않고 다음 갱신에서 후보 거래를 다시 검사합니다. 완료 전에는 같은 슬롯을 재생성하지 않습니다.")) bool bRewardPending=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Spawner DT ID입니다. 슬롯 키/SlotIndex와 묶어 고유 지점을 관리합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) FName SpawnerID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="0부터 시작하는 슬롯 인덱스입니다. 스포너 고유 슬롯 또는 UI 저장 슬롯 번호의 상위 규칙을 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) int32 SlotIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Monster DT의 정의 ID입니다. 스폰 슬롯의 개별 인스턴스 키와 구분합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) FName MonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="스폰 슬롯 처치 상태입니다. 재생 시각/셀 스폰 실행과 함께 저장합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) bool bDefeated = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="스폰 슬롯 재생 가능 절대 PlayClock 초입니다. 마지막 처치/현재 셀 상태를 함께 검사하는 실행기가 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) double RespawnReadyAtPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="캐릭터/드롭의 월드 위치 cm입니다. 유한 좌표를 사용하고 실제 이동/저장 시 원본에 반영합니다.\n입력 자료형: 3차원 좌표/크기.\nC++ 초기값: FVector::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="월드 회전 도입니다. 위치와 함께 저장/복원하며 메시 상대 회전과 구분합니다.\n입력 자료형: 회전 각도.\nC++ 초기값: FRotator::ZeroRotator. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 체력입니다. 0~MaxHP이며 LifeState와 함께 검증합니다.\n입력 자료형: 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) float HP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 마나입니다. 0~MaxMP이며 실제 스킬 비용 확정은 전투 경로와 연결합니다.\n입력 자료형: 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) float MP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="체력 상한 또는 장비 HP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) float MaxHP = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="마나 상한 또는 장비 MP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) float MaxMP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="캐릭터/몬스터의 마지막 PlayClock 처리 시각입니다. 오프라인 UTC와 구분하며 현재 시계보다 미래 값을 만들지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) double LastSimPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="효과/쿨타임/캡처 시각의 전투 캐시입니다. 활성 참가자 상태를 반영한 뒤 저장/제거에 사용하며 GAS 실행 복원은 별도입니다.\n입력 자료형: 구조/객체 (FICCombatState).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 지역/스포너/슬롯별 스폰·처치·재생·셀 상태 원본입니다. 실제 스폰 제어 연결은 아직 필요하지만 저장 검사와 시간 기준을 제공합니다.")) FICCombatState Combat;
};

USTRUCT(BlueprintType, meta=(ToolTip="자원 지점의 고갈·다음 재생 PlayClock·생산 상태 원본입니다. 실제 채집 실행은 별도 연결 대상입니다.", ShortToolTip="자원 지점의 고갈·다음 재생 PlayClock·생산 상태 원본입니다."))
struct FICResourcePointState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="자원 지점의 Resource DT 정의 ID입니다. 현재 지점 상태와 정적 획득/재생 규칙을 구분합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 자원 지점의 고갈·다음 재생 PlayClock·생산 상태 원본입니다. 실제 채집 실행은 별도 연결 대상입니다.")) FName DefinitionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="자원 지점 남은 사용 수입니다. 0은 소진 상태이며 실제 자원 실행에서 재생/획득 거래와 함께 갱신합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 자원 지점의 고갈·다음 재생 PlayClock·생산 상태 원본입니다. 실제 채집 실행은 별도 연결 대상입니다.")) int32 RemainingUses = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="자원 지점 사용 가능 절대 PlayClock 시각입니다. UTC/수면 게임 분과 구분합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 자원 지점의 고갈·다음 재생 PlayClock·생산 상태 원본입니다. 실제 채집 실행은 별도 연결 대상입니다.")) double ReadyAtPlaySecond = 0;
};

USTRUCT(BlueprintType, meta=(ToolTip="월드 드롭의 고유 GUID·아이템 내용물·위치·생성 PlayClock 초를 보존합니다. DropActor는 이 원본을 표시하고 획득 거래에서 내용물과 가방을 함께 확정합니다. 거래 완료 이력은 WorldState의 공통 거래 기록을 사용합니다.", ShortToolTip="월드 드롭의 고유 GUID·아이템 내용물·위치·생성 PlayClock 초를 보존합니다."))
struct FICDropState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="실제 드롭 레코드 GUID입니다. 정적 DT 문자열 ID가 아닙니다. 생성 때 새 GUID를 만들고 줍기/수명/로드에서도 같은 값을 사용합니다.\n입력 자료형: 인스턴스 GUID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FGuid ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="실제 드롭의 아이템 인스턴스 목록입니다. GUID/수량/강화/옵션을 보존하며 줍기는 가방 이동과 목록 감소를 같은 거래로 처리합니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<FPlayerItemInstance> Contents;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="캐릭터/드롭의 월드 위치 cm입니다. 유한 좌표를 사용하고 실제 이동/저장 시 원본에 반영합니다.\n입력 자료형: 3차원 좌표/크기.\nC++ 초기값: FVector::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="드롭 생성 절대 PlayClock 초입니다. 현재 시각과 수명 차이로 제거하며 UTC/오프라인 시간과 구분합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double CreatedPlaySecond = 0;
};
