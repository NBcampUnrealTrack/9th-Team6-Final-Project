#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "Life/Survival/ICPlayerSurvival.h"
#include "ICLegacyPlayerSaveGame.generated.h"

UCLASS(meta=(ToolTip="예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.", ShortToolTip="예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다."))
class ISEKAICLASS_API UICLegacyPlayerSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame, meta=(ToolTip="예전 저장 호환 데이터 버전입니다. 현재 SaveVersion과 구분하며 새 저장 작성에는 사용하지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 8. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) int32 Version=8;
    // V1 importer compatibility. Runtime saves use UICSaveSubsystem.
    UPROPERTY(SaveGame, meta=(ToolTip="현재 체력입니다. 0~MaxHP이며 LifeState와 함께 검증합니다.\n입력 자료형: 소수.\nC++ 초기값: 60.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) float HP=60.f;
    UPROPERTY(SaveGame, meta=(ToolTip="현재 생활 자원입니다. 0~MaxSP이며 표시에서는 내림 정수로 보여줍니다. 실제 플레이 경과 10초당 1이 감소합니다.\n입력 자료형: 소수.\nC++ 초기값: 70.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) float SP=70.f;
    UPROPERTY(SaveGame, meta=(ToolTip="플레이어 힘 기본 능력치의 표시/호환 값입니다. WorldState STR와 대응합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) int32 Strength=1;
    UPROPERTY(SaveGame, meta=(ToolTip="선택/저장 플레이어 이름입니다. 공백 제외 1~12자/줄바꿈 없음 규칙을 현재 생성/저장 검사에서 요구합니다.\n입력 자료형: 문자열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) FString PlayerName;
    UPROPERTY(SaveGame, meta=(ToolTip="외형 프리셋 0/1 인덱스입니다. 실제 외형 배열/성별 선택과 맞추고 기존 저장 선택값을 유지합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) int32 Preset=0;
    UPROPERTY(SaveGame, meta=(ToolTip="선택/저장 직업 인덱스입니다. 0=전사,1=궁수,2=마법사,3=힐러입니다. Class enum의 None 포함 값과 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) int32 Job=0;
    UPROPERTY(SaveGame, meta=(ToolTip="최초 Skill ID 기록입니다. 직업별 NewGame.StartingSkills에서 선택하며 실제 사용 가능한 스킬 콘텐츠/GAS 연결과 별개입니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) FName FirstSkill;
    UPROPERTY(SaveGame, meta=(ToolTip="일반 가방의 슬롯 원본입니다. 배열 길이가 용량이고 빈 칸도 유지합니다. 기본 20칸이며 거래 후보를 검증해 변경합니다.\n입력 자료형: 구조/객체 (FPlayerContainer).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) FPlayerContainer Bag;
    UPROPERTY(SaveGame, meta=(ToolTip="퀘스트 아이템 인스턴스 원본입니다. 일반 가방과 분리하고 판매/버리기 금지 규칙을 검사합니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) TArray<FPlayerItemInstance> QuestItems;
    UPROPERTY(SaveGame, meta=(ToolTip="장비 인스턴스 목록입니다. 플레이어 원본은 Inventory의 네 슬롯이고 CharacterState의 플레이어 Equipment는 비워 둡니다. NPC 장비는 CharacterState에 저장합니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(SaveGame, meta=(ToolTip="퀵슬롯 ID 배열 또는 개수 규격입니다. 플레이 배열 정확히 4칸/인덱스 0~3=키 1~4이며 저장 길이를 검사합니다.\n입력 자료형: 이름/ID 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) TArray<FName> QuickSlots;
    UPROPERTY(SaveGame, meta=(ToolTip="컨테이너 키→슬롯 내용물 원본입니다. 기본 PlayerHome은 40칸이며 소유권 원본은 상위 WorldState.ContainerOwnership과 구분합니다.\n입력 자료형: 이름/ID → 구조/객체 (FPlayerContainer) 맵.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) TMap<FName,FPlayerContainer> Storage;
    UPROPERTY(SaveGame, meta=(ToolTip="캐릭터/드롭의 월드 위치 cm입니다. 유한 좌표를 사용하고 실제 이동/저장 시 원본에 반영합니다.\n입력 자료형: 3차원 좌표/크기.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) FVector Position;
    UPROPERTY(SaveGame, meta=(ToolTip="월드 회전 도입니다. 위치와 함께 저장/복원하며 메시 상대 회전과 구분합니다.\n입력 자료형: 회전 각도.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) FRotator Rotation;
    UPROPERTY(SaveGame, meta=(ToolTip="HP/MP/SP 표시/실행 상태입니다. 지속 원본과 참가자 캐시를 반영하고 직접 WBP에서 소모/저장을 중복 실행하지 않습니다.\n입력 자료형: 구조/객체 (FPlayerSurvivalState).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) FPlayerSurvivalState Survival;
    UPROPERTY(SaveGame, meta=(ToolTip="누적 GameClock 분입니다. 시작 480=08:00이며 날짜가 지나도 누적 유지합니다. 화면 하루 시간은 1440 나머지로 계산합니다.\n입력 자료형: 정수.\nC++ 초기값: 480. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) int32 GameMinute=480;
    UPROPERTY(SaveGame, meta=(ToolTip="예전 저장의 플레이 초 호환 값입니다. 현재 시계 원본은 Clock.PlaySecond이며 가져오기 때 변환합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) float PlaySeconds=0.f;
    UPROPERTY(SaveGame, meta=(ToolTip="기본 능력치 6종의 지속/표시 원본입니다. 장비/효과 최종 파생값과 구분하며 변경은 상태 API를 사용합니다.\n입력 자료형: 구조/객체 (FPlayerMainStats).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) FPlayerMainStats MainStats;
    UPROPERTY(SaveGame, meta=(ToolTip="보유 골드 또는 보상 지급량입니다. 보유 원본은 WorldState.Gold이며 보상에서는 증가량입니다. 음수는 허용하지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 예전 저장 파일을 읽어 현재 WorldState로 가져오는 데이터 전용 호환 클래스입니다. 새 저장 원본으로 사용하지 말고 현재 SaveSubsystem의 ImportLegacySave를 사용하세요.")) int32 Gold=0;
};

// UE SaveGame headers look up the original native class by its literal path.
// This data-only compatibility class lets the importer read pre-refactor files.
UCLASS(NotBlueprintable, meta=(ToolTip="예전 SaveGame 헤더에 기록된 클래스 경로를 읽기 위해 유지하는 호환 별칭입니다. Blueprint 생성 대상이 아니며 새 게임 기능을 구현하는 클래스가 아닙니다.", ShortToolTip="예전 SaveGame 헤더에 기록된 클래스 경로를 읽기 위해 유지하는 호환 별칭입니다."))
class ISEKAICLASS_API UPlayerPrototypeSave : public UICLegacyPlayerSaveGame
{
    GENERATED_BODY()
};
