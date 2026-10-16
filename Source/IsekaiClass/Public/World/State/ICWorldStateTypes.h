#pragma once

#include "CoreMinimal.h"
#include "World/State/Types/ICCharacterStateTypes.h"
#include "World/State/Types/ICInventoryStateTypes.h"
#include "World/State/Types/ICQuestStateTypes.h"
#include "World/State/Types/ICWorldProgressStateTypes.h"
#include "World/State/Types/ICClockStateTypes.h"
#include "World/State/Types/ICLifeStateTypes.h"
#include "ICWorldStateTypes.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="저장되는 전체 지속 상태 원본입니다. 플레이어 기본 정보·캐릭터·인벤토리·퀘스트·제작·소유권·드롭·진행·중복 처리 영수증을 포함합니다. 정적 이름/규칙은 ID로 DT에서 조회합니다.", ShortToolTip="저장되는 전체 지속 상태 원본입니다."))
struct FICWorldState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="지속 플레이어 생성 완료입니다. 이름/첫 스킬/CHR_PLAYER 상태와 함께 저장 검사합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) bool bPlayerCreated = false;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="선택/저장 플레이어 이름입니다. 공백 제외 1~12자/줄바꿈 없음 규칙을 현재 생성/저장 검사에서 요구합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) FString PlayerName;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="외형 프리셋 0/1 인덱스입니다. 실제 외형 배열/성별 선택과 맞추고 기존 저장 선택값을 유지합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) int32 Preset = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="선택/저장 직업 인덱스입니다. 0=전사,1=궁수,2=마법사,3=힐러입니다. Class enum의 None 포함 값과 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) int32 Job = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="최초 Skill ID 기록입니다. 직업별 NewGame.StartingSkills에서 선택하며 실제 사용 가능한 스킬 콘텐츠/GAS 연결과 별개입니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) FName FirstSkill;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="보유 골드 또는 보상 지급량입니다. 보유 원본은 WorldState.Gold이며 보상에서는 증가량입니다. 음수는 허용하지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 200. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) int32 Gold=200;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="플레이어 저장 월드 위치 cm입니다. 실제 Pawn에서 반영하고 로드 때 복원합니다.\n입력 자료형: 3차원 좌표/크기.\nC++ 초기값: FVector(0, 0, 120). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) FVector PlayerPosition = FVector(0, 0, 120);
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="플레이어 저장 회전 도입니다. 실제 Controller/Pawn 기준 복원 방향과 맞춥니다.\n입력 자료형: 회전 각도.\nC++ 초기값: FRotator::ZeroRotator. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) FRotator PlayerRotation = FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="캐릭터 ID→지속 상태 맵입니다. 스폰되지 않은 캐릭터도 유지하며 키와 값의 CharacterId가 같아야 합니다.\n입력 자료형: 이름/ID → 구조/객체 (FICCharacterState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, FICCharacterState> Characters;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="캐릭터의 인벤토리 컴포넌트 또는 WorldState 인벤토리 후보/원본입니다. 실제 컴포넌트는 지속 원본을 조회하며 WBP에 별도 보관 원본을 만들지 않습니다.\n입력 자료형: 구조/객체 (FICInventoryState).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) FICInventoryState Inventory;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="Quest ID→현재 수락/진행/완료/보상 상태입니다. 정적 정의와 카운터를 분리해 저장합니다.\n입력 자료형: 이름/ID → 구조/객체 (FICQuestState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, FICQuestState> Quests;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="실제 해금 Warp ID 집합입니다. 발견/이동 실행에서 갱신하며 저장 때 유지합니다.\n입력 자료형: 이름/ID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FName> UnlockedWarps;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="몬스터 처치 시각의 기존 원본 맵입니다. 실제 슬롯별 상태 SpawnSlots와 중복 원본이 생기지 않게 실제 처치 실행을 연결합니다.\n입력 자료형: 이름/ID → 배정밀도 소수 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, double> MonsterKilledAt;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="처치한 보스 Monster ID 집합입니다. 실제 진행/보상 이벤트에서 원본을 갱신하고 중복 완료를 검사합니다.\n입력 자료형: 이름/ID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FName> DefeatedBosses;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="기존 자원 재생 시각 맵입니다. ResourcePoints의 지점별 원본과 중복되지 않게 실제 경과 실행을 연결합니다.\n입력 자료형: 이름/ID → 배정밀도 소수 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, double> ResourceReadyAt;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="지속 드롭 레코드 목록입니다. GUID·내용물·위치·PlayClock 생성 시각을 유지하며 액터 제거가 원본 삭제를 뜻하지 않습니다.\n입력 자료형: 구조/객체 (FICDropState) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TArray<FICDropState> Drops;
    // Stable anchor/spawner IDs are keys; definitions remain in DT/DataAsset.
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="작업 GUID→저장 제작 상태입니다. 시작 비용/재료 예약/완료 시각/결과 영수증을 유지합니다.\n입력 자료형: 인스턴스 GUID → 구조/객체 (FICProductionJobState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FGuid, FICProductionJobState> ProductionJobs;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="작물 상태 맵입니다. 정의/소유자·심은/수확 가능 게임 시각을 유지하며 실제 성장 실행은 별도입니다.\n입력 자료형: 이름/ID → 구조/객체 (FICCropState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, FICCropState> Crops;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="스포너/슬롯 키→몬스터 현재 상태입니다. 스폰 액터와 별개로 유지하며 실제 셀/스폰 실행 연결은 별도입니다.\n입력 자료형: 이름/ID → 구조/객체 (FICSpawnSlotState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, FICSpawnSlotState> SpawnSlots;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="지점 키→자원 고갈/재생 상태입니다. 정의 ID와 지점 인스턴스 키를 구분하며 채집 실행은 별도입니다.\n입력 자료형: 이름/ID → 구조/객체 (FICResourcePointState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, FICResourcePointState> ResourcePoints;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="컨테이너 키→현재 소유자/주거/플레이어 소유 기록입니다. 실제 내용물 Storage와 구분합니다.\n입력 자료형: 이름/ID → 구조/객체 (FICContainerOwnershipState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, FICContainerOwnershipState> ContainerOwnership;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="컨테이너/설치 키→실제 설치 기록입니다. 주거/앵커/수량 검사를 거쳐 실제 설치 실행과 연결해야 합니다.\n입력 자료형: 이름/ID → 구조/객체 (FICStorageInstallationState) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, FICStorageInstallationState> StorageInstallations;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="발견한 Region ID 집합입니다. 실제 지역/지도 발견 실행과 연결해야 합니다.\n입력 자료형: 이름/ID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FName> DiscoveredRegionIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="발견한 Anchor ID 집합입니다. 실제 지도 발견 실행과 연결해야 합니다.\n입력 자료형: 이름/ID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FName> DiscoveredAnchorIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="등록한 Codex ID 집합입니다. 첫 등록만 새 기록이며 실제 도감 이벤트 실행을 연결합니다.\n입력 자료형: 이름/ID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FName> CodexEntryIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="실제 해금 Recipe ID 집합입니다. 기본 해금 레시피 외 추가 해금 원본이며 저장 때 유지합니다.\n입력 자료형: 이름/ID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FName> UnlockedRecipeIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="Progression ID→현재 단계 정수 원본입니다. 실제 진행 실행 연결이 필요하며 정적 Stages 배열과 구분합니다.\n입력 자료형: 이름/ID → 정수 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, int32> ProgressionStages;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="플래그 키→현재 bool 원본입니다. 정적 정의 ID와 별개이며 일반 조건/진행 판정 실행에서 조회합니다.\n입력 자료형: 이름/ID → 체크박스(true/false) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TMap<FName, bool> Flags;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="이미 확정한 거래 GUID 집합입니다. 같은 GUID 거래 재실행을 거부해 아이템·골드·효과 중복 확정을 막습니다.\n입력 자료형: 인스턴스 GUID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FGuid> CompletedTransactionIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="이미 처리한 게임 이벤트 GUID 집합입니다. 같은 변경/완료 사건을 중복 적용하지 않도록 실제 실행 경로에 연결합니다.\n입력 자료형: 인스턴스 GUID 중복 없는 집합.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) TSet<FGuid> ProcessedEventIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="엔딩 완료 원본입니다. EndingKind/보상 영수증과 함께 실제 엔딩 실행에서 확정해야 합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) bool bEndingCleared = false;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="완료 엔딩 종류 키입니다. 현재 검사 허용 키는 Ending.Normal/Ending.Hidden이며 bEndingCleared와 함께 기록합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 저장되는 전체 지속 상태 원본입니다.")) FName EndingKind;
};

USTRUCT(BlueprintType, meta=(ToolTip="저장 허용 여부를 판정할 현재 안전 상태입니다. Controller가 전투·사망·대화·이벤트·아이템 처리 상태를 갱신하고 SaveSubsystem이 검사합니다.", ShortToolTip="저장 허용 여부를 판정할 현재 안전 상태입니다."))
struct FICSaveSafety
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재 전투 상태입니다. 메뉴/저장/생산 안전 판정에 사용하며 실제 전투 수신기에서 시작/종료를 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 저장 허용 여부를 판정할 현재 안전 상태입니다.")) bool bInCombat = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재 대화 상태입니다. 저장/생산 안전 판정에 사용하며 대화 종료/위젯 제거 시 반드시 해제합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 저장 허용 여부를 판정할 현재 안전 상태입니다.")) bool bInDialogue = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재 이벤트 연출 상태입니다. 저장/생산 안전 판정에 사용하며 종료/레벨 전환 시 정리합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 저장 허용 여부를 판정할 현재 안전 상태입니다.")) bool bInEvent = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재 플레이어 사망/저장 차단 상태입니다. LifeState.Downed 동료와 구분하며 실제 사망 UI/안전 검사에 사용합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 저장 허용 여부를 판정할 현재 안전 상태입니다.")) bool bDead = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="아이템 효과/원자 거래 처리 중 저장 차단 상태입니다. Safety에 반영해 처리 도중 저장을 막습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 저장 허용 여부를 판정할 현재 안전 상태입니다.")) bool bEffectPending = false;
};

// Preview metadata is independent of gameplay state and optional for older saves.
USTRUCT(BlueprintType, meta=(ToolTip="저장 슬롯의 화면 표시용 요약과 PNG 썸네일입니다. 실제 복원 원본은 WorldState/Clock이며 미리보기 문자열을 게임 상태로 사용하지 마세요.", ShortToolTip="저장 슬롯의 화면 표시용 요약과 PNG 썸네일입니다."))
struct FICSavePreview
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="저장 화면에 표시할 지역/장소 이름입니다. 표시 요약이며 실제 플레이어 좌표/Region ID 원본이 아닙니다.\n입력 자료형: 문자열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FString Location;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="저장 미리보기 파티 캐릭터 ID 목록입니다. 표시 순서/초상화에 사용하며 실제 파티 원본은 Characters.PartySlot입니다.\n입력 자료형: 이름/ID 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<FName> PartyIds;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="저장 순간 화면 PNG 바이트입니다. 캡처 허용/뷰포트와 파일 크기 검사를 거쳐 저장하며 캡처 실패는 실제 저장 결과/안내를 확인합니다.\n입력 자료형: 0~255 정수 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<uint8> ThumbnailPNG;
};

USTRUCT(BlueprintType, meta=(ToolTip="디스크에서 읽은 슬롯의 존재·유효성·백업·이름·시각·자원·미리보기 정보입니다. bExists와 bValid를 구분해 손상 파일이 있는 슬롯을 빈 슬롯처럼 처리하지 마세요.", ShortToolTip="디스크에서 읽은 슬롯의 존재·유효성·백업·이름·시각·자원·미리보기 정보입니다."))
struct FICSaveSlotInfo
{
    GENERATED_BODY()
    // 0 = auto, 1..4 = manual.
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="슬롯 번호/장착 위치입니다. 배열은 0부터, 수동 저장은 1~4/자동 0입니다. 타입/상위 용도를 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 Slot = 0;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="슬롯 파일이 존재합니다. bValid와 별개이며 손상된 파일이 있는 슬롯도 존재 상태입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bExists = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="파일/슬롯 데이터 검사를 통과했습니다. bExists만 true인 손상 슬롯에서는 로드를 계속하지 않습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bValid = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="슬롯 정보가 백업 파일을 사용합니다. 원본 손상/복구 상황을 UI에 안내하고 정상 빈 슬롯과 구분합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bUsesBackup = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="선택/저장 플레이어 이름입니다. 공백 제외 1~12자/줄바꿈 없음 규칙을 현재 생성/저장 검사에서 요구합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString PlayerName;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="저장 실제 날짜/시각입니다. 슬롯 표시용이며 제작/효과 시간의 원본은 PlayClock/GameClock입니다.\n입력 자료형: FDateTime.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FDateTime SavedAt;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="외형 프리셋 0/1 인덱스입니다. 실제 외형 배열/성별 선택과 맞추고 기존 저장 선택값을 유지합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 Preset = 0;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="저장 슬롯 위치/파티/PNG 표시 요약입니다. 복원 원본은 WorldState/Clock이므로 표시 문자열로 상태를 복원하지 않습니다.\n입력 자료형: 구조/객체 (FICSavePreview).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICSavePreview Preview;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="실제 시계 서브시스템 참조 또는 저장 시계 스냅샷입니다. GameMinute 분과 PlaySecond 초를 구분해 저장/복원합니다.\n입력 자료형: 구조/객체 (FICClockState).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICClockState Clock;
};
