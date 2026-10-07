#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICWorldStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICCharacterChanged, FName, CharacterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICStateChanged);

UCLASS(meta=(ToolTip="GameInstance에 살아 있는 지속 상태의 원본입니다. 액터 제거 뒤에도 캐릭터·인벤토리·작업·퀘스트·월드 기록을 유지합니다. 활성 참가자 캐시를 반영하고 검증된 변경 뒤 이벤트를 발행합니다.", ShortToolTip="GameInstance에 살아 있는 지속 상태의 원본입니다."))
class ISEKAICLASS_API UICWorldStateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="캐릭터 원본 변경 이벤트입니다. ID에 해당하는 스냅샷을 다시 조회하며 화면/액터 제거 때 구독을 해제합니다.\n입력 자료형: FICCharacterChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameInstance에 살아 있는 지속 상태의 원본입니다.")) FICCharacterChanged OnCharacterChanged;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="WorldState 인벤토리 확정 후 이벤트입니다. 성공한 원본을 다시 조회하고 거래 소모를 이벤트 안에서 재실행하지 않습니다.\n입력 자료형: FICStateChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameInstance에 살아 있는 지속 상태의 원본입니다.")) FICStateChanged OnInventoryChanged;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="전체 WorldState 복원 이벤트입니다. 액터/UI 표시를 원본으로 재설정하고 이전 이벤트 구독을 정리합니다.\n입력 자료형: FICStateChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameInstance에 살아 있는 지속 상태의 원본입니다.")) FICStateChanged OnWorldRestored;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="WorldState 골드 변경 이벤트입니다. 현재 원본을 재조회해 UI를 갱신합니다.\n입력 자료형: FICStateChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameInstance에 살아 있는 지속 상태의 원본입니다.")) FICStateChanged OnPlayerGoldChanged;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="지속 드롭 원본 변경 이벤트입니다. Controller가 표시 액터를 동기화하며 실제 원본 변경을 다시 중복 실행하지 않습니다.\n입력 자료형: FICStateChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.\n소속 정의/시스템: GameInstance에 살아 있는 지속 상태의 원본입니다.")) FICStateChanged OnDropsChanged;

    static FName PlayerId() { IC_TRACE_FUNCTION(); return TEXT("CHR_PLAYER"); }
    static bool IsCharacterIdValid(FName ID);
    UFUNCTION(BlueprintPure, Category="IC|State", meta=(ToolTip="캐릭터 ID의 현재 스냅샷을 조회합니다. 등록된 활성 참여자 캐시와 플레이어 장착 정보를 반영하며 저장 원본 자체를 반환하지 않습니다. 캐릭터가 없으면 기본값 상태입니다.\n@param CharacterId WorldState에서 조회할 캐릭터 ID입니다. 플레이어는 CHR_PLAYER입니다. 입력 핀. 자료형: 이름/ID.\n@return 조회 상태의 복사본입니다. ID가 None이면 대상이 없으므로 사용하지 마세요. 자료형: 구조/객체 (FICCharacterState).")) FICCharacterState GetSnapshot(FName CharacterId) const;
    UFUNCTION(BlueprintPure, Category="IC|State", meta=(ToolTip="WorldState 인벤토리 원본의 복사본을 조회합니다. 복사본 수정은 원본에 반영되지 않으며 변경 API를 사용해야 합니다.\n@return 현재 인벤토리 스냅샷입니다. 자료형: 구조/객체 (FICInventoryState).")) FICInventoryState GetInventorySnapshot() const { IC_TRACE_FUNCTION(); return State.Inventory; }
    UFUNCTION(BlueprintPure, Category="IC|State", meta=(ToolTip="현재 지속 상태 원본 전체를 복사합니다. 저장 후보를 만들기 전에는 활성 실행 캐시 반영을 먼저 수행해야 합니다.\n@return WorldState 전체 스냅샷입니다. 복사본 변경은 원본에 반영되지 않습니다. 자료형: 구조/객체 (FICWorldState).")) FICWorldState ExportState() const { IC_TRACE_FUNCTION(); return State; }
    const FICCharacterState* GetStoredCharacter(FName ID) const { IC_TRACE_FUNCTION(); return State.Characters.Find(ID); }
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="실제 NPC·직업·영입 전투·초기 장비 DT를 검증해 미등록 캐릭터의 지속 상태를 생성합니다. 이미 등록된 원본을 초기값으로 덮어쓰지 않으며 임시 NPC 정의를 만들지 않습니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Kind 생성할 지속 캐릭터 종류입니다. 실제 NPC 정적 유형과 일치해야 하며 플레이어는 전용 생성 경로를 사용합니다. 입력 핀. 자료형: 선택 목록 (EICCharacterKind).\n@return 이미 유효하게 등록되어 있거나 생성에 성공하면 true입니다. 누락 참조면 false입니다. 자료형: 체크박스(true/false).")) bool EnsureCharacter(FName ID, EICCharacterKind Kind);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="캐릭터 후보 상태를 검사하고 WorldState에 확정합니다. ID·종류·자원·능력치·장비 등이 유효해야 합니다. 성공 후 변경 이벤트를 발행합니다.\n@param Character 반영하거나 표시할 캐릭터 상태/액터입니다. 이 함수의 입력 자료형을 확인하세요. 입력 핀. 자료형: 구조/객체 (FICCharacterState).\n@return 전체 상태 변경을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool SetCharacterState(const FICCharacterState& Character);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="지정 NPC의 관계 총값을 설정합니다. 증감량이 아닌 최종값이며 대상·범위 검사를 통과해야 합니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Value 지정한 관계 또는 기본 능력치의 최종 정수값입니다. 더할 양이 아닙니다. 입력 핀. 자료형: 정수.\n@return 관계 변경을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool SetAffinity(FName ID, int32 Value);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="캐릭터의 지정 기본 능력치를 최종 정수값으로 설정합니다. 파생 공격력/최대 자원을 직접 설정하는 함수와 구분하세요.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Stat 변경할 기본 능력치입니다. STR·DEX·INT·CON·CHA·LUK 중 선택합니다. 입력 핀. 자료형: 선택 목록 (EICMainStat).\n@param Value 지정한 관계 또는 기본 능력치의 최종 정수값입니다. 더할 양이 아닙니다. 입력 핀. 자료형: 정수.\n@return 능력치 변경을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool SetMainStat(FName ID, EICMainStat Stat, int32 Value);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="캐릭터의 현재 HP·MP·SP를 함께 설정합니다. 유한값·범위·생존 상태 등 후보 검사가 실패하면 부분 적용하지 않습니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param HP 설정할 현재 HP입니다. 유한값이며 0~최대 HP 범위여야 합니다. 입력 핀. 자료형: 소수.\n@param MP 설정할 현재 MP입니다. 유한값이며 0~최대 MP 범위여야 합니다. 입력 핀. 자료형: 소수.\n@param SP 설정할 현재 SP입니다. 유한값이며 0~최대 SP 범위여야 합니다. 입력 핀. 자료형: 소수.\n@return 자원 변경 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool SetVitals(FName ID, float HP, float MP, float SP);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="캐릭터의 일과/동료 모드 변경을 요청합니다. 호출 전에 활성 실행 캐시를 WorldState에 반영해야 합니다. 현재 변경 함수는 종류·Downed·파티 상태를 검사하며 모드가 일과로 바뀌면 파티 슬롯을 해제합니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Mode 새 일과/동료 모드입니다. 전환 전에 실행 캐시를 반영하세요. 입력 핀. 자료형: 선택 목록 (EICCharacterMode).\n@return 모드 변경을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool SetCharacterMode(FName ID, EICCharacterMode Mode);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="캐릭터의 동료 위치를 지정합니다. 0~3은 동료 슬롯, -1은 파티 미배치이며 중복과 허용 종류를 검사합니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Slot 동료 슬롯 0~3 또는 미배치 -1입니다. 입력 핀. 자료형: 정수.\n@return 파티 배치 변경을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool SetPartySlot(FName ID, int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="인벤토리 후보 전체를 실제 정의·장비·예약 참조와 함께 검사해 WorldState에 확정합니다. 부분 스택이나 장비만 먼저 적용하면 안 됩니다.\n@param Inventory 거래에 사용할 실제 인벤토리 컴포넌트입니다. 현재 WorldState에 연결된 컴포넌트를 전달하세요. 입력 핀. 자료형: 구조/객체 (FICInventoryState).\n@return 전체 인벤토리 후보를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool CommitInventoryState(const FICInventoryState& Inventory);
    bool CommitCharacterAndInventory(const FICCharacterState& Character, const FICInventoryState& Inventory);
    bool CommitGameplayTransaction(const FICWorldState& Candidate, FGuid TransactionID);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="플레이어의 저장 위치 cm와 회전 각도를 함께 갱신합니다. 실제 Pawn 이동은 별도 실행 경로에서 반영하세요.\n@param Position 저장할 월드 위치입니다. X/Y/Z 단위는 cm입니다. 입력 핀. 자료형: 3차원 좌표/크기.\n@param Rotation 저장할 월드 회전입니다. Pitch/Yaw/Roll 단위는 도입니다. 입력 핀. 자료형: 회전 각도.\n@return 유효한 위치/회전 원본을 반영하면 true입니다. 자료형: 체크박스(true/false).")) bool SetPlayerTransform(FVector Position, FRotator Rotation);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="지정 캐릭터 ID와 상태 인터페이스를 구현한 활성 객체를 등록합니다. 동일 ID 중복 소유와 무효 객체를 거부하며 EndPlay에서 해제해야 합니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Participant UICStateParticipant 인터페이스를 구현한 실제 활성 액터/컴포넌트입니다. 입력 핀. 자료형: UObject*.\n@return 유효 참여자 등록에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool RegisterStateParticipant(FName ID, UObject* Participant);
    UFUNCTION(BlueprintCallable, Category="IC|State", meta=(ToolTip="지정 ID의 해당 실행 참여자 연결을 해제합니다. BeginPlay/EndPlay를 짝으로 연결하고 저장해야 할 캐시는 제거 전에 반영하세요.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Participant UICStateParticipant 인터페이스를 구현한 실제 활성 액터/컴포넌트입니다. 입력 핀. 자료형: UObject*.")) void UnregisterStateParticipant(FName ID, UObject* Participant);
    bool SetPlayerGold(int32 Value);
    bool FlushParticipants();
    bool CanRestoreState(const FICWorldState& Candidate) const;
    bool RestoreState(const FICWorldState& Candidate);
    UFUNCTION(BlueprintPure, Category="IC|State", meta=(ToolTip="WorldState 후보의 ID·수치·참조 모양·인벤토리·파티·생산 예약·중복 거래 등 저장 구조를 검사합니다. 검사만 수행하고 원본을 바꾸지 않습니다.\n@param Candidate 검사 또는 복원할 전체 WorldState 후보입니다. 사전 검사에서 원본을 변경하지 마세요. 입력 핀. 자료형: 구조/객체 (FICWorldState).\n@param FailureReason 검사 실패 원인의 출력 문자열입니다. 실패했을 때 원본 적용을 중단하고 표시하세요. 입력/출력 핀. 자료형: 문자열.\n@return 후보 구조에 오류가 없으면 true입니다. false일 때 FailureReason을 확인하세요. 자료형: 체크박스(true/false).")) static bool ValidateStateWithReason(const FICWorldState& Candidate, FString& FailureReason);
    static bool ValidateState(const FICWorldState& Candidate);
    static bool ValidateAdditionalState(const FICWorldState& Candidate, FString& FailureReason);
    static bool ValidateStateClock(const FICWorldState& Candidate, const FICClockState& Clock);
    UFUNCTION(BlueprintPure, Category="IC|State", meta=(ToolTip="현재 시각을 기준으로 지정 생산 작업의 남은 PlayClock 초를 계산합니다. 수면 GameClock 분과 구분합니다.\n@param JobID 생산 작업의 고유 GUID입니다. 시작에서는 출력이며 완료/취소에서는 동일 작업의 GUID를 전달합니다. 입력 핀. 자료형: 인스턴스 GUID.\n@param PlaySecond 남은 시간을 계산할 현재 누적 PlayClock 초입니다. GameClock 분을 전달하지 마세요. 입력 핀. 자료형: 배정밀도 소수.\n@return 남은 작업 시간의 초 값입니다. 없는 작업 또는 유효하지 않은 현재 시각은 -1, 완료/취소 작업과 시간이 끝난 작업은 0입니다. 작업 상태도 함께 확인하세요. 자료형: 배정밀도 소수.")) double GetProductionRemainingSeconds(FGuid JobID, double PlaySecond) const;
    static bool ValidateInventoryShape(const FICInventoryState& Inventory, TSet<FGuid>* ExistingIDs = nullptr);
    bool ApplyWorldProgress(const FICWorldState& Candidate);
private:
    bool AdvanceOfflineCharacter(FName ID);
    UPROPERTY(meta=(ToolTip="GameInstance의 유일한 지속 상태 원본입니다. 내부 거래/검증 API가 갱신하고 성공 뒤 이벤트를 발행합니다. BP에 별도 원본 복사본을 유지하지 않습니다.\n입력 자료형: 구조/객체 (FICWorldState).\n소속 정의/시스템: GameInstance에 살아 있는 지속 상태의 원본입니다.")) FICWorldState State;
    TMap<FName, TWeakObjectPtr<UObject>> Participants;
};
