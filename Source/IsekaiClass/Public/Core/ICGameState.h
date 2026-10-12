#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ICGameState.generated.h"
UENUM(BlueprintType, meta=(ToolTip="게임의 큰 진행 단계입니다. 단계 변경과 시간 실행 여부는 GameState/TimeSubsystem 경로에서 함께 관리합니다."))
enum class EICGamePhase : uint8 { CharacterCreation UMETA(ToolTip="초기 캐릭터 생성 단계입니다. 이름·직업·외형을 선택하고 생성 확정 전에는 플레이를 시작하지 않습니다.\n게임의 큰 진행 단계입니다. 단계 변경과 시간 실행 여부는 GameState/TimeSubsystem 경로에서 함께 관리합니다."), Playing UMETA(ToolTip="실제 월드 플레이 단계입니다. 다른 정지 요청이 없을 때 시간이 진행합니다.\n게임의 큰 진행 단계입니다. 단계 변경과 시간 실행 여부는 GameState/TimeSubsystem 경로에서 함께 관리합니다."), Menu UMETA(ToolTip="메뉴 단계입니다. 메뉴 정지 요청과 함께 사용하고 마지막 정지 요청 해제 후 플레이를 재개합니다.\n게임의 큰 진행 단계입니다. 단계 변경과 시간 실행 여부는 GameState/TimeSubsystem 경로에서 함께 관리합니다."), Dead UMETA(ToolTip="플레이어 사망 단계입니다. 동료의 Downed 상태와 구분하며 저장 안전 조건을 확인합니다.\n게임의 큰 진행 단계입니다. 단계 변경과 시간 실행 여부는 GameState/TimeSubsystem 경로에서 함께 관리합니다.") };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerTimeJumped,int32,FromMinute,int32,ToMinute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerGameMinutesAdvanced,int32,Minutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICGamePhaseChanged,EICGamePhase,Phase);
class UICTimeSubsystem;
UCLASS(Blueprintable, meta=(ToolTip="현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다. 실제 시간 원본은 TimeSubsystem이고 저장되는 캐릭터·인벤토리 상태 원본은 WorldStateSubsystem입니다.", ShortToolTip="현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다."))
class ISEKAICLASS_API AICGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    AICGameState();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="현재 생성/플레이/메뉴/사망 단계 표시입니다. SetPhase로 이벤트를 발행하며 지속 캐릭터 원본과 구분합니다.\n입력 자료형: 선택 목록 (EICGamePhase).\nC++ 초기값: CharacterCreation — 초기 캐릭터 생성 단계입니다. 이름·직업·외형을 선택하고 생성 확정 전에는 플레이를 시작하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다.")) EICGamePhase Phase=EICGamePhase::CharacterCreation;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Storage", meta=(ToolTip="플레이어 집 공유 컨테이너 키입니다. 기본 PlayerHome이며 동일 내용물 공유 규칙을 사용하는 원본과 맞춥니다.\n입력 자료형: 이름/ID.\nC++ 초기값: 문자열 \"PlayerHome\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다.")) FName SharedHomeStorageID=TEXT("PlayerHome");
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="누적 GameClock 분입니다. 시작 480=08:00이며 날짜가 지나도 누적 유지합니다. 화면 하루 시간은 1440 나머지로 계산합니다.\n입력 자료형: 정수.\nC++ 초기값: 480. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다.")) int32 GameMinute=480;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="현재 게임 단계 변경 이벤트입니다. 새 Phase에 맞춰 표시하며 시간 원본/저장 원본을 직접 중복 갱신하지 않습니다.\n입력 자료형: FICGamePhaseChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: 현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다.")) FICGamePhaseChanged OnPhaseChanged;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="GameClock 점프 이벤트입니다. 이전/이후 분으로 수면·일과 경과를 처리하며 PlayClock 제작 시간을 임의 증가시키지 않습니다.\n입력 자료형: FPlayerTimeJumped.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: 현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다.")) FPlayerTimeJumped OnTimeJumped;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="자연 시간으로 정수 게임 분이 증가한 이벤트입니다. 수면 점프 이벤트와 구분해 중복 경과 처리를 피합니다.\n입력 자료형: FPlayerGameMinutesAdvanced.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: 현재 게임 단계와 시간 표시를 제공하는 월드 액터입니다.")) FPlayerGameMinutesAdvanced OnGameMinutesAdvanced;
    UFUNCTION(BlueprintCallable, meta=(ToolTip="GameState의 게임 단계를 변경하고 단계 이벤트를 발행합니다. 실제 시간 실행과 정지 요청도 관련 경로에서 조정하세요.\n@param NewPhase 전환할 게임 단계입니다. 입력 핀. 자료형: 선택 목록 (EICGamePhase).")) void SetPhase(EICGamePhase NewPhase);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="GameClock을 지정 분만큼 진행시키는 시간 점프를 요청합니다. 생산용 PlayClock 초와 구분합니다.\n@param Minutes 진행/점프할 GameClock 분입니다. PlayClock 초와 구분합니다. 입력 핀. 자료형: 정수.")) void AdvanceGameMinutes(int32 Minutes);
private:
    UICTimeSubsystem* GetTime() const;
    UFUNCTION(meta=(ToolTip="TimeSubsystem의 실제 시계 원본을 읽어 GameState 표시값을 갱신합니다.")) void RefreshClock();
    UFUNCTION(meta=(ToolTip="게임 분 진행 이벤트를 받는 내부 콜백입니다. 표시용 날짜·시간을 갱신합니다.\n@param Minutes 진행/점프할 GameClock 분입니다. PlayClock 초와 구분합니다. 입력 핀. 자료형: 정수.")) void HandleMinutesAdvanced(int32 Minutes);
    UFUNCTION(meta=(ToolTip="수면 등 GameClock 시간 점프의 내부 콜백입니다. 이전/이후 누적 분으로 표시 상태를 갱신합니다.\n@param From 점프 이전 누적 GameClock 분입니다. 입력 핀. 자료형: 배정밀도 소수.\n@param To 점프 이후 누적 GameClock 분입니다. 입력 핀. 자료형: 배정밀도 소수.")) void HandleTimeJumped(double From,double To);
};
