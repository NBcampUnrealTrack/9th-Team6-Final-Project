#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICTimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FICTimeJumped, double, FromGameMinute, double, ToGameMinute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICClockChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICNaturalMinutesAdvanced,int32,Minutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICPlaySecondsAdvanced,double,Seconds);

UCLASS(meta=(ToolTip="GameClock과 PlayClock의 원본입니다. 메뉴/팝업별 소유자·사유의 정지 요청을 함께 관리하고 마지막 요청이 끝나야 재개합니다. 수면은 GameClock 점프, 실제 경과는 PlayClock 증가로 구분합니다.", ShortToolTip="GameClock과 PlayClock의 원본입니다."))
class ISEKAICLASS_API UICTimeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="GameClock 점프 이벤트입니다. 이전/이후 분으로 수면·일과 경과를 처리하며 PlayClock 제작 시간을 임의 증가시키지 않습니다.\n입력 자료형: FICTimeJumped.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameClock과 PlayClock의 원본입니다.")) FICTimeJumped OnTimeJumped;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="시계 변경 이벤트입니다. GameMinute/PlaySecond 표시를 다시 조회하며 직접 시계 값을 증가시키는 신호로 쓰지 않습니다.\n입력 자료형: FICClockChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameClock과 PlayClock의 원본입니다.")) FICClockChanged OnClockChanged;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="자연 시간으로 정수 게임 분이 증가한 이벤트입니다. 수면 점프 이벤트와 구분해 중복 경과 처리를 피합니다.\n입력 자료형: FICNaturalMinutesAdvanced.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameClock과 PlayClock의 원본입니다.")) FICNaturalMinutesAdvanced OnGameMinutesAdvanced;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="자연 PlayClock 증가량 초 이벤트입니다. SP·생산이 사용하며 메뉴 정지에는 증가하지 않습니다. 같은 증가량을 다시 시계에 더하지 마세요.\n입력 자료형: FICPlaySecondsAdvanced.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameClock과 PlayClock의 원본입니다.")) FICPlaySecondsAdvanced OnPlaySecondsAdvanced;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { IC_TRACE_FUNCTION(); return true; }
    virtual UWorld* GetTickableGameObjectWorld() const override { IC_TRACE_FUNCTION(); return GetWorld(); }
    virtual TStatId GetStatId() const override;
    UFUNCTION(BlueprintPure, Category="IC|Time", meta=(ToolTip="현재 누적 GameClock 분·PlayClock 초의 복사본을 조회합니다. 화면 시간은 누적 게임 분에서 계산하세요.\n@return 현재 시계 스냅샷입니다. 자료형: 구조/객체 (FICClockState).")) FICClockState GetClock() const { IC_TRACE_FUNCTION(); return Clock; }
    UFUNCTION(BlueprintCallable, Category="IC|Time", meta=(ToolTip="시간 진행의 기본 실행 상태를 설정합니다. true여도 메뉴·팝업 등 정지 요청이 남으면 IsRunning은 false입니다.\n@param Running true는 기본 시간 실행, false는 기본 정지입니다. 다른 정지 요청이 있으면 true여도 진행하지 않습니다. 입력 핀. 자료형: 체크박스(true/false).")) void SetRunning(bool Running);
    UFUNCTION(BlueprintCallable, Category="IC|Time", meta=(ToolTip="기존 메뉴 연결용 공통 정지 상태를 설정합니다. 여러 소유자의 메뉴/팝업은 SetPauseRequest로 각각 관리하세요.\n@param Paused true는 해당 정지 요청 등록, false는 해제입니다. 입력 핀. 자료형: 체크박스(true/false).")) void SetMenuPaused(bool Paused);
    UFUNCTION(BlueprintPure, Category="IC|Time", meta=(ToolTip="기본 실행 상태와 모든 정지 요청을 합쳐 실제 시간이 진행 가능한지 조회합니다.\n@return 현재 두 시계가 진행 가능한 상태이면 true입니다. 자료형: 체크박스(true/false).")) bool IsRunning() const { IC_TRACE_FUNCTION(); return bRunning && !bMenuPaused && PauseRequests.IsEmpty(); }
    // Each owner/reason pair is idempotent. Closing one popup cannot release another's pause.
    UFUNCTION(BlueprintCallable, Category="IC|Time", meta=(ToolTip="Owner와 Reason 쌍으로 메뉴·팝업·대상 모드의 정지 요청을 등록/해제합니다. 같은 쌍의 중복 등록은 한 요청이며 마지막 요청을 해제해야 월드가 재개합니다. 사라진 Owner의 요청은 정리됩니다.\n@param Owner 정지 요청을 소유하는 위젯/액터/객체입니다. 동일 소유자·이유 조합으로 해제하세요. 입력 핀. 자료형: UObject*.\n@param Reason 정지 요청의 안정적인 FName 키입니다. 같은 Owner와 Reason 쌍으로 해제해야 합니다. 입력 핀. 자료형: 이름/ID.\n@param Paused true는 해당 정지 요청 등록, false는 해제입니다. 입력 핀. 자료형: 체크박스(true/false).")) void SetPauseRequest(UObject* Owner, FName Reason, bool Paused);
    UFUNCTION(BlueprintCallable, Category="IC|Time", meta=(ToolTip="해당 Owner가 등록한 모든 정지 요청을 해제합니다. 위젯 제거·액터 종료에서 호출하고 다른 소유자의 요청은 유지합니다.\n@param Owner 정지 요청을 소유하는 위젯/액터/객체입니다. 동일 소유자·이유 조합으로 해제하세요. 입력 핀. 자료형: UObject*.")) void ReleasePauseRequests(UObject* Owner);
    UFUNCTION(BlueprintCallable, Category="IC|Time", meta=(ToolTip="레벨 전환 전에 이전 월드의 정지 소유자·메뉴 요청을 정리합니다. 새 레벨 초기화 시 기본 실행 상태를 다시 맞추세요.")) void PrepareForTravel();
    UFUNCTION(BlueprintCallable, Category="IC|Time", meta=(ToolTip="양수 GameClock 분만큼 수면 등의 시간 점프를 요청합니다. PlayClock은 증가시키지 않고 전/후 시간 점프 이벤트를 발행합니다.\n@param Minutes 진행/점프할 GameClock 분입니다. PlayClock 초와 구분합니다. 입력 핀. 자료형: 배정밀도 소수.\n@return 유효한 시간 점프를 반영하면 true입니다. 자료형: 체크박스(true/false).")) bool JumpGameMinutes(double Minutes);
    bool RestoreClock(const FICClockState& Saved);
    void ResetClock();
    void AdvanceClock(double DeltaSeconds, bool bWorldPaused);
private:
    struct FPauseRequest { TWeakObjectPtr<UObject> Owner; FName Reason; };
    TArray<FPauseRequest> PauseRequests;
    void UpdateWorldPause();
    TWeakObjectPtr<UWorld> PausedWorld;
    bool bOwnsWorldPause = false;
    UPROPERTY(meta=(ToolTip="실제 시계 서브시스템 참조 또는 저장 시계 스냅샷입니다. GameMinute 분과 PlaySecond 초를 구분해 저장/복원합니다.\n입력 자료형: 구조/객체 (FICClockState).\n소속 정의/시스템: GameClock과 PlayClock의 원본입니다.")) FICClockState Clock;
    bool bRunning = false;
    bool bMenuPaused = true;
};
