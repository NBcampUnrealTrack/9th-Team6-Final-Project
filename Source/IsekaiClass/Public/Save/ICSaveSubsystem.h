#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveSubsystem.generated.h"

class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICSaveGame;
class UICLegacyPlayerSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICSaveStarted, int32, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FICSaveFinished, int32, Slot, bool, bSuccess, const FString&, Message);

UCLASS(meta=(ToolTip="현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다. 저장 전 상태를 반영하고 검증된 파일을 확정합니다. 수동 1~4, 자동 0 슬롯을 사용합니다.", ShortToolTip="현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다."))
class ISEKAICLASS_API UICSaveSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { IC_TRACE_FUNCTION(); return true; }
    virtual UWorld* GetTickableGameObjectWorld() const override { IC_TRACE_FUNCTION(); return GetWorld(); }
    virtual TStatId GetStatId() const override;

    UPROPERTY(BlueprintAssignable, meta=(ToolTip="실제 저장 시작 이벤트입니다. Slot에 맞춰 UI를 잠그고 완료 이벤트에서 상태를 해제합니다.\n입력 자료형: FICSaveStarted.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) FICSaveStarted OnSaveStarted;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="저장 완료/실패 이벤트입니다. Slot/bSuccess/Message를 확인하고 성공 때만 종료/다음 화면을 진행합니다.\n입력 자료형: FICSaveFinished.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) FICSaveFinished OnSaveFinished;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="실제 저장 진행 상태입니다. 완료 이벤트 전 중복 저장/로드/생성 요청을 제한합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) bool bSaving = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="자동 저장 요청 대기 상태입니다. 안전 조건이 되면 저장을 시도하며 직접 결과 성공 플래그로 사용하지 않습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) bool bAutoSavePending = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Save", meta=(ClampMin="1", ToolTip="실제 자동 저장 요청 PlayClock 초 간격입니다. 300=플레이 5분이며 안전하지 않으면 요청을 대기합니다. 메뉴 정지에는 플레이 시간이 흐르지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 300. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) double AutoSaveInterval = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Save", meta=(ClampMin="0", ClampMax="1", ToolTip="실제 자동 저장 HP/MaxHP 하한입니다. 0.25=25%이며 자동 저장 안전 판정에 사용합니다. 정적 SaveSettings와 자동 동기화되지 않습니다.\n입력 자료형: 소수.\nC++ 초기값: 0.25. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) float AutoSaveHealthThreshold = 0.25f;
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="전투·대상 선택·다운·모션 등 현재 저장 제한 상태를 저장 서브시스템에 전달합니다. 실제 상태 변경마다 호출 측에서 갱신해야 합니다.\n@param NewSafety 현재 전투·사망·대화·이벤트·효과 처리 상태의 스냅샷입니다. 입력 핀. 자료형: 구조/객체 (FICSaveSafety).")) void SetSafetyState(const FICSaveSafety& NewSafety);
    UFUNCTION(BlueprintPure, Category="IC|Save", meta=(ToolTip="현재 저장 제한·작업 중·단계·자동/수동 정책을 검사합니다. 성공해도 실제 저장 시작 직전에 다시 검사해야 합니다.\n@param bAutoSave true는 자동 저장 안전 정책, false는 수동 저장 안전 정책을 검사합니다. 입력 핀. 자료형: 체크박스(true/false).\n@return 현재 요청한 저장 종류가 가능하면 true입니다. 자료형: 체크박스(true/false).")) bool CanSave(bool bAutoSave) const;
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="지정 슬롯의 저장을 요청합니다. 실행 캐시 반영과 후보 검사 후 화면 미리보기·WorldState를 비동기로 저장합니다. 성공/실패는 OnSaveFinished에서 확인하세요.\n@param Slot 요청할 저장 슬롯입니다. 0자동 저장, 1~4수동 저장입니다. 입력 핀. 자료형: 정수.\n@return 저장 요청을 접수하면 true입니다. 파일 저장 완료를 의미하지 않습니다. 자료형: 체크박스(true/false).")) bool SaveSlot(int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="지정 슬롯을 읽고 형식·버전·상태·실행 참여자 복원 가능 여부를 검사한 뒤 WorldState와 실행 캐시를 적용합니다. 실패한 후보를 일부 적용하지 않습니다.\n@param Slot 불러올 저장 슬롯입니다. 0자동 저장, 1~4수동 저장입니다. 입력 핀. 자료형: 정수.\n@return 로드 검사와 복원에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool LoadSlot(int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="지정 원인의 자동 저장을 요청합니다. 자동 저장 간격과 안전 조건을 검사하며 즉시 저장되지 않을 수 있습니다.\n@param Reason 자동 저장 요청 원인을 구분할 안정적인 FName 키입니다. 입력 핀. 자료형: 이름/ID.")) void RequestAutoSave(FName Reason);
    UFUNCTION(BlueprintPure, Category="IC|Save", meta=(ToolTip="저장 슬롯의 유효 여부·이름·직업·위치·시각·파티·스크린샷 미리보기를 조회합니다. 미리보기는 현재 플레이 상태와 구분합니다.\n@return 슬롯 미리보기 목록입니다. bValid를 먼저 확인하세요. 자료형: 구조/객체 (FICSaveSlotInfo) 배열.")) TArray<FICSaveSlotInfo> GetSlotList() const;
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="이전 저장 형식을 현재 WorldState 형식으로 변환하고 검사합니다. 이전 데이터 호환용이며 새 콘텐츠 작성 경로에서는 현재 슬롯 형식을 사용하세요.\n@return 호환 변환과 적용에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool ImportLegacySave();
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="지정 저장 슬롯 파일과 관련 미리보기 파일을 삭제합니다. UI의 삭제 확인 이후에 호출하세요.\n@param Slot 삭제할 저장 슬롯입니다. 0자동 저장, 1~4수동 저장이며 삭제 확인 이후 호출하세요. 입력 핀. 자료형: 정수.\n@return 삭제 작업에 성공하면 true입니다. 파일이 복구된다는 뜻이 아닙니다. 자료형: 체크박스(true/false).")) bool DeleteSlot(int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="이전 저장 형식의 파일을 삭제합니다. 현재 슬롯 삭제와 별개이며 UI 확인 이후에 호출하세요.\n@return 이전 저장 파일 삭제에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool DeleteLegacySave();
    UFUNCTION(BlueprintPure, Category="IC|Save", meta=(ToolTip="현재 자동/수동 저장이 제한되는 원인을 조회합니다. UI 안내용이며 실제 저장은 CanSave와 시작 재검사를 통과해야 합니다.\n@param bAutoSave true는 자동 저장 안전 정책, false는 수동 저장 안전 정책을 검사합니다. 입력 핀. 자료형: 체크박스(true/false).\n@return 저장 제한 안내 문자열입니다. 제한이 없으면 빈 문자열입니다. 자료형: 문자열.")) FString GetSaveBlockReason(bool bAutoSave) const;
    FString GetLegacySavePath() const;
    bool HasLegacySave() const;
    void SetAutoSaveSuspended(bool bSuspended);
    bool ConvertLegacySave(const UICLegacyPlayerSaveGame* Legacy, FICWorldState& OutState) const;
    void ResetSession();
    void PollAutoSave();
    // Explicit dependency injection and isolated slot prefixes for automated checks.
    bool ConfigureServices(UICWorldStateSubsystem* InState, UICTimeSubsystem* InTime, const FString& InPrefix);
    bool SaveSlotSynchronously(int32 Slot);
    FString GetSlotPath(int32 Slot) const;
    // Region/quest code updates this whenever the player's displayed location changes.
    UFUNCTION(BlueprintCallable, Category="IC|Save", meta=(ToolTip="다음 저장 미리보기에 표시할 위치 이름을 지정합니다. 실제 월드 좌표나 플레이어 위치 원본을 변경하지 않습니다.\n@param Location 저장 미리보기에 표시할 위치 이름입니다. 실제 좌표 원본을 이동시키지 않습니다. 입력 핀. 자료형: 문자열.")) void SetPreviewLocation(const FString& Location) { IC_TRACE_FUNCTION(); PreviewLocation = Location.Left(128); }
    UFUNCTION(BlueprintPure, Category="IC|Save", meta=(ToolTip="현재 저장 미리보기용 위치 문구를 조회합니다.\n@return 저장 화면에 표시할 위치 문자열입니다. 자료형: 문자열.")) FString GetPreviewLocation() const { IC_TRACE_FUNCTION(); return PreviewLocation; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Save", meta=(ToolTip="실제 저장 화면 PNG 캡처 허용입니다. 체크 시 준비 과정에서 캡처를 시도하며 화면/뷰포트가 없는 검사 환경은 캡처가 불가능할 수 있습니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) bool bCaptureSaveThumbnail = true;
    FString GetLastMessage() const { IC_TRACE_FUNCTION(); return LastMessage; }
private:
    FString PreviewLocation;
    bool CaptureThumbnail(TArray<uint8>& OutPNG);
    UPROPERTY(meta=(ToolTip="저장 원본 또는 WorldStateSubsystem 참조입니다. 상태를 반영/검증한 뒤 서비스로 접근하고 독립 WBP 원본을 만들지 않습니다.\n입력 자료형: 객체 참조 (UICWorldStateSubsystem).\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) TObjectPtr<UICWorldStateSubsystem> WorldState;
    UPROPERTY(meta=(ToolTip="실제 TimeSubsystem 의존 참조입니다. 초기화/로컬 GameInstance로 연결하며 별도 시계를 복제하지 않습니다.\n입력 자료형: 객체 참조 (UICTimeSubsystem).\n소속 정의/시스템: 현재 슬롯 저장/로드·백업·자동 저장·썸네일·호환 가져오기 서비스입니다.")) TObjectPtr<UICTimeSubsystem> Time;
    FICSaveSafety Safety;
    FString Prefix = TEXT("IC_Team");
    FString LastMessage;
    double LastSavePlaySecond = 0;
    double RetryAfterPlaySecond = 0;
    FName PendingReason;
    uint64 AutoRequestGeneration = 0;
    bool bAutoSaveSuspended = false;
    bool PrepareSave(int32 Slot, TArray<uint8>& Bytes, double& CapturedPlaySecond, bool& bRotateBackup);
    UICSaveGame* ReadSlot(int32 Slot, bool bAllowBackup, bool& bUsedBackup) const;
    bool ValidateSave(const UICSaveGame* Save, int32 Slot) const;
    void FinishSave(int32 Slot, bool bSuccess, double CapturedPlaySecond, uint64 CapturedGeneration, const FString& Message);
};
