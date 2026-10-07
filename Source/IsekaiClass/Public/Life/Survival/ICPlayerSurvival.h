#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "ICPlayerSurvival.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="HP/MP/SP와 자원 상한 및 SP 감소의 남은 부분 시간을 담습니다. 플레이가 진행된 10초마다 SP 1을 감소시키고 메뉴 정지에는 감소하지 않습니다. 화면 표시에서는 소수점을 내립니다.", ShortToolTip="HP/MP/SP와 자원 상한 및 SP 감소의 남은 부분 시간을 담습니다."))
struct FPlayerSurvivalState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 체력입니다. 0~MaxHP이며 LifeState와 함께 검증합니다.\n입력 자료형: 소수.\nC++ 초기값: 60.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float HP = 60.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="체력 상한 또는 장비 HP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 100.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MaxHP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 마나입니다. 0~MaxMP이며 실제 스킬 비용 확정은 전투 경로와 연결합니다.\n입력 자료형: 소수.\nC++ 초기값: 100.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="마나 상한 또는 장비 MP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 100.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MaxMP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="현재 생활 자원입니다. 0~MaxSP이며 표시에서는 내림 정수로 보여줍니다. 실제 플레이 경과 10초당 1이 감소합니다.\n입력 자료형: 소수.\nC++ 초기값: 70.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float SP = 70.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="생활 자원 상한 또는 장비 SP 추가량입니다. 캐릭터 원본과 장비 기여를 구분합니다.\n입력 자료형: 소수.\nC++ 초기값: 100.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) float MaxSP = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(ToolTip="다음 SP 감소까지 누적한 부분 PlayClock 초입니다. 0 이상 10 미만을 유지하고 저장/로드 때 보존해 타이머 초기화로 SP 감소가 밀리지 않게 합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) double SPDrainElapsedPlaySeconds = 0;
    static constexpr double SPDrainIntervalPlaySeconds = 10;

    bool IsValid() const;
    bool IsExhausted() const { IC_TRACE_FUNCTION(); return SP <= 0.f; }
    float RestoreHP(float Amount);
    float RestoreSP(float Amount);
    float ConsumeSP(float Amount);
    float AdvanceNaturalSP(double PlaySeconds);
    int32 CalculateSleepMinutes(float RecoveryPerGameMinute) const;
    void CompleteSleep();
};
