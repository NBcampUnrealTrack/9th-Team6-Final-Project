#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICInventoryStateTypes.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="컨테이너의 현재 소유자·소속 집·공유 그룹 기록입니다. DT의 초기 소유 정의와 구분해 저장합니다.", ShortToolTip="컨테이너의 현재 소유자·소속 집·공유 그룹 기록입니다."))
struct FICContainerOwnershipState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 소유 캐릭터 ID입니다. 플레이어 CHR_PLAYER 또는 실제 NPC ID를 사용하며 소유권 원본을 저장합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 컨테이너의 현재 소유자·소속 집·공유 그룹 기록입니다.")) FName OwnerCharacterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Home DT의 주거 ID입니다. 초기 주거 정의와 현재 소유권/설치 기록을 구분하세요.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 컨테이너의 현재 소유자·소속 집·공유 그룹 기록입니다.")) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="컨테이너의 현재 플레이어 소유 여부입니다. OwnerCharacterID/주거 상태와 함께 유지합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 컨테이너의 현재 소유자·소속 집·공유 그룹 기록입니다.")) bool bPlayerOwned = false;
};

USTRUCT(BlueprintType, meta=(ToolTip="설치한 보관함의 집 ID·보관함 ID·생산 ID를 보존합니다. 보관함의 실제 내용물은 Inventory.Storage에서 ContainerID로 조회합니다. 설치 위치·GUID·용량을 이 구조의 필드로 저장하는 방식은 현재 구현되어 있지 않습니다.", ShortToolTip="설치한 보관함의 집 ID·보관함 ID·생산 ID를 보존합니다."))
struct FICStorageInstallationState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Home DT의 주거 ID입니다. 초기 주거 정의와 현재 소유권/설치 기록을 구분하세요.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 설치 위치·GUID·용량을 이 구조의 필드로 저장하는 방식은 현재 구현되어 있지 않습니다.")) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="컨테이너의 안정적인 저장 키입니다. 집/공유 그룹/소유자와 함께 설정하고 여러 물체가 같은 내용물을 공유할 때만 같은 키를 사용합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 설치 위치·GUID·용량을 이 구조의 필드로 저장하는 방식은 현재 구현되어 있지 않습니다.")) FName ContainerID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="Production DT ID입니다. 현재 레시피 실행은 제작 시간/SP/결과 종류를 연결 규칙과 비교합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 설치 위치·GUID·용량을 이 구조의 필드로 저장하는 방식은 현재 구현되어 있지 않습니다.")) FName ProductionID;
};

USTRUCT(BlueprintType, meta=(ToolTip="플레이어의 가방·퀘스트 아이템·장착 장비·퀵슬롯·보관함 내용물을 보존하는 인벤토리 원본입니다. 변경은 InventoryComponent의 거래 API를 통해 검사하고 WorldState에 확정합니다. 집 소유권과 보관함 설치 기록은 WorldState의 별도 필드에서 관리합니다.", ShortToolTip="플레이어의 가방·퀘스트 아이템·장착 장비·퀵슬롯·보관함 내용물을 보존하는 인벤토리 원본입니다."))
struct FICInventoryState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="일반 가방의 슬롯 원본입니다. 배열 길이가 용량이고 빈 칸도 유지합니다. 기본 20칸이며 거래 후보를 검증해 변경합니다.\n입력 자료형: 구조/객체 (FPlayerContainer).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 플레이어의 가방·퀘스트 아이템·장착 장비·퀵슬롯·보관함 내용물을 보존하는 인벤토리 원본입니다.")) FPlayerContainer Bag;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="퀘스트 아이템 인스턴스 원본입니다. 일반 가방과 분리하고 판매/버리기 금지 규칙을 검사합니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 플레이어의 가방·퀘스트 아이템·장착 장비·퀵슬롯·보관함 내용물을 보존하는 인벤토리 원본입니다.")) TArray<FPlayerItemInstance> QuestItems;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="장비 인스턴스 목록입니다. 플레이어 원본은 Inventory의 네 슬롯이고 CharacterState의 플레이어 Equipment는 비워 둡니다. NPC 장비는 CharacterState에 저장합니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 플레이어의 가방·퀘스트 아이템·장착 장비·퀵슬롯·보관함 내용물을 보존하는 인벤토리 원본입니다.")) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="퀵슬롯 ID 배열 또는 개수 규격입니다. 플레이 배열 정확히 4칸/인덱스 0~3=키 1~4이며 저장 길이를 검사합니다.\n입력 자료형: 이름/ID 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 플레이어의 가방·퀘스트 아이템·장착 장비·퀵슬롯·보관함 내용물을 보존하는 인벤토리 원본입니다.")) TArray<FName> QuickSlots;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="컨테이너 키→슬롯 내용물 원본입니다. 기본 PlayerHome은 40칸이며 소유권 원본은 상위 WorldState.ContainerOwnership과 구분합니다.\n입력 자료형: 이름/ID → 구조/객체 (FPlayerContainer) 맵.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 플레이어의 가방·퀘스트 아이템·장착 장비·퀵슬롯·보관함 내용물을 보존하는 인벤토리 원본입니다.")) TMap<FName, FPlayerContainer> Storage;
    FICInventoryState()
    {
        IC_TRACE_FUNCTION();
        Bag.Slots.SetNum(20);
        Equipment.SetNum(4);
        QuickSlots.SetNum(4);
        Storage.Add(TEXT("PlayerHome")).Slots.SetNum(40);
    }
};
