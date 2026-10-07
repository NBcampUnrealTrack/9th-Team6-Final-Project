#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveGame.generated.h"

UCLASS(meta=(ToolTip="현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다. 실제 파일 쓰기는 SaveSubsystem을 사용하고 직접 이 객체만 저장해 공통 검증을 우회하지 마세요.", ShortToolTip="현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다."))
class ISEKAICLASS_API UICSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UICSaveGame();
    static constexpr int32 CurrentVersion = 3;
    static bool EncodeForFile(UICSaveGame* Save, TArray<uint8>& Bytes);
    static UICSaveGame* DecodeFile(const TArray<uint8>& Bytes);
    static bool IsFutureFile(const TArray<uint8>& Bytes);
    bool UpgradeFromVersion2();
    UPROPERTY(SaveGame, meta=(ToolTip="현재 저장 파일 데이터 버전입니다. CRC 파일 형식/로드 변환에서 사용하며 DT DataVersion과 별개입니다.\n입력 자료형: 정수.\nC++ 초기값: CurrentVersion. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다.")) int32 SaveVersion = CurrentVersion;
    UPROPERTY(SaveGame, meta=(ToolTip="슬롯 번호/장착 위치입니다. 배열은 0부터, 수동 저장은 1~4/자동 0입니다. 타입/상위 용도를 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다.")) int32 Slot = 0;
    UPROPERTY(SaveGame, meta=(ToolTip="저장 실제 날짜/시각입니다. 슬롯 표시용이며 제작/효과 시간의 원본은 PlayClock/GameClock입니다.\n입력 자료형: FDateTime.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다.")) FDateTime SavedAt;
    UPROPERTY(SaveGame, meta=(ToolTip="저장 원본 또는 WorldStateSubsystem 참조입니다. 상태를 반영/검증한 뒤 서비스로 접근하고 독립 WBP 원본을 만들지 않습니다.\n입력 자료형: 구조/객체 (FICWorldState).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다.")) FICWorldState WorldState;
    UPROPERTY(SaveGame, meta=(ToolTip="실제 시계 서브시스템 참조 또는 저장 시계 스냅샷입니다. GameMinute 분과 PlaySecond 초를 구분해 저장/복원합니다.\n입력 자료형: 구조/객체 (FICClockState).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다.")) FICClockState Clock;
    UPROPERTY(SaveGame, meta=(ToolTip="저장 슬롯 위치/파티/PNG 표시 요약입니다. 복원 원본은 WorldState/Clock이므로 표시 문자열로 상태를 복원하지 않습니다.\n입력 자료형: 구조/객체 (FICSavePreview).\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 현재 저장 파일의 버전·슬롯·저장 시각·WorldState·시계·미리보기 묶음입니다.")) FICSavePreview Preview;
};
