#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "ICClockStateTypes.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="누적 GameClock 분과 누적 PlayClock 초의 저장 원본입니다. 하루 시각은 누적 분에서 계산합니다. 메뉴 정지 중 두 시계 모두 진행하지 않으며 수면 시간 점프는 GameClock에만 반영합니다.", ShortToolTip="누적 GameClock 분과 누적 PlayClock 초의 저장 원본입니다."))
struct FICClockState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="누적 GameClock 분입니다. 시작 480=08:00이며 날짜가 지나도 누적 유지합니다. 화면 하루 시간은 1440 나머지로 계산합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 480. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 누적 GameClock 분과 누적 PlayClock 초의 저장 원본입니다.")) double GameMinute = 480;
    UPROPERTY(BlueprintReadOnly, SaveGame, meta=(ToolTip="누적 월드 진행 PlayClock 초입니다. 메뉴/팝업 정지 중 증가하지 않고 수면 GameClock 점프로 증가하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.\n소속 정의/시스템: 누적 GameClock 분과 누적 PlayClock 초의 저장 원본입니다.")) double PlaySecond = 0;
    bool IsValid() const { IC_TRACE_FUNCTION(); return FMath::IsFinite(GameMinute) && GameMinute >= 0 && FMath::IsFinite(PlaySecond) && PlaySecond >= 0; }
};
