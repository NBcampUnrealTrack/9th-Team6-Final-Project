#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "Life/Survival/ICPlayerSurvival.h"
#include "World/State/ICStateParticipant.h"
#include "Input/ICInputTypes.h"
#include "ICPlayerController.generated.h"

class AICInteractionActor;
class AICNPCBase;
class AICDroppedItem;
class UICCraftingComponent;
class UICInventoryComponent;
class UICGameplayWidget;
class UICQuestWidget;
class UICQuestTrackerWidget;
class UICNPCDialogueWidget;
class UICMainMenuWidget;
class UICCharacterCreateWidget;
class UICSaveLoadWidget;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICSaveSubsystem;
class UInputMappingContext;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerCombatRequest,FName,Action,int32,Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerInitialized,FName,JobID,FName,FirstSkillID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerStatBookApplied,FName,TargetID,EPlayerMainStat,Stat,int32,Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerExhaustionChanged,bool,bExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerSleepRequested,int32,GameMinutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerEquipmentStatsChanged,FPlayerEquipmentStats,Stats);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICMenuChanged,const FString&,Menu);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICNoticeChanged,const FString&,Notice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICSaveSessionChanged);

UCLASS(Blueprintable, meta=(ToolTip="실제 플레이어 입력·메뉴·캐릭터 생성·아이템 사용·저장 요청을 조정합니다. 지속 상태 원본은 WorldState이고 이 클래스의 공개 값은 화면 표시와 입력 처리를 위한 값입니다. 전투 실행은 OnCombatRequest의 실제 수신 경로를 연결해야 합니다.", ShortToolTip="실제 플레이어 입력·메뉴·캐릭터 생성·아이템 사용·저장 요청을 조정합니다."))
class ISEKAICLASS_API AICPlayerController : public APlayerController, public IICStateParticipant
{
    GENERATED_BODY()
public:
    AICPlayerController();
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|UI",meta=(ToolTip="저장·불러오기 공통 화면 클래스입니다. WBP_IC_SaveLoad를 지정하면 게임 중 OpenSaveMenu/OpenLoadMenu 호출 시 자동 표시합니다. 슬롯 선택·확인·삭제·종료 규칙은 컨트롤러의 Session 코드가 담당합니다.")) TSubclassOf<UICSaveLoadWidget> SaveLoadWidgetClass;
    UPROPERTY(BlueprintAssignable,Category="IC|Session",meta=(ToolTip="슬롯 목록 또는 저장/불러오기 선택이 변경된 알림입니다. 위젯은 컨트롤러의 SaveSlots와 선택값을 다시 표시하며 제거 시 구독을 해제합니다.")) FICSaveSessionChanged OnSaveSessionChanged;
    UFUNCTION(BlueprintCallable,Category="IC|Session",meta=(ToolTip="현재 저장 또는 불러오기 화면의 슬롯을 선택합니다. 저장은 1~4, 불러오기는 자동 0과 수동 1~4이며 두 선택값은 독립적으로 유지됩니다. 저장 중과 확인 중에는 선택을 변경하지 않습니다.\n@param Slot 선택할 저장 슬롯 번호입니다. 입력 핀. 자료형: 정수.")) void SelectSessionSlot(int32 Slot);
    UFUNCTION(BlueprintCallable,Category="IC|Session",meta=(ToolTip="현재 불러오기 슬롯을 재검사하고 컨트롤러의 공통 확인 팝업을 엽니다. 확인 이후에만 로드하며 메인 메뉴에서는 지정한 플레이 맵으로 이동합니다.")) void RequestLoadPlayer();
    UFUNCTION(BlueprintPure,Category="IC|Session",meta=(ToolTip="확인 팝업 뒤에 있는 저장/불러오기 메뉴 키를 반환합니다. 위젯 표시용 조회이며 상태 원본은 컨트롤러입니다.\n@return 현재 세션 메뉴 키입니다. 출력 핀. 자료형: 문자열.")) FString GetSessionMenu() const;
    UFUNCTION(BlueprintPure,Category="IC|Session",meta=(ToolTip="현재 세션 메뉴에 맞는 선택 슬롯 번호를 반환합니다. 저장 선택과 불러오기 선택은 서로 덮어쓰지 않습니다.\n@return 현재 선택 슬롯 번호입니다. 출력 핀. 자료형: 정수.")) int32 GetSelectedSessionSlot() const;
    int32 GetPendingSessionSlot() const { IC_TRACE_FUNCTION();return ModalSlot; }
    void SetSessionGameplayMap(TSoftObjectPtr<UWorld> Map) { IC_TRACE_FUNCTION();SessionGameplayMap=Map; }
    bool HandleSessionKey(const FKey& Key,bool bRepeat=false);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Main Menu", meta=(ToolTip="공통 컨트롤러가 메인 메뉴 레벨에서 표시할 UICMainMenuWidget 자식 WBP 클래스입니다. BP_ICPlayerController의 클래스 기본값에 실제 WBP_MainMenu를 지정하세요. 게임모드의 MainMenuMap과 일치하는 레벨에서만 생성하며 플레이 레벨에서는 캐릭터 생성/게임 화면을 사용합니다.\n입력 자료형: 클래스 선택(UICMainMenuWidget)."))
    TSubclassOf<UICMainMenuWidget> MainMenuWidgetClass;
    UFUNCTION(BlueprintPure, Category="IC|Main Menu", meta=(ToolTip="공통 ICGameMode가 현재 월드를 메인 메뉴 레벨로 지정했는지 조회합니다. 같은 컨트롤러를 메뉴와 플레이에서 사용하므로 저장 화면의 불러오기/입력 분기에 사용합니다.\n@return 현재 게임모드가 메인 메뉴 레벨로 판정하면 true입니다. 플레이 레벨이나 게임모드가 없으면 false입니다."))
    bool IsMainMenuLevel() const;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|New Game", meta=(ToolTip="실제 새 게임 설정 ID입니다. 기본 CFG_NEWGAME_001이며 NewGame DT에서 지급/자원/시간을 조회합니다.\n입력 자료형: 이름/ID.\nC++ 초기값: 문자열 \"CFG_NEWGAME_001\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName NewGameDefinitionID=TEXT("CFG_NEWGAME_001");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI", meta=(ToolTip="캐릭터 생성에 사용할 기존 UICCharacterCreateWidget 자식 WBP 클래스입니다. 인스턴스가 아니라 클래스 에셋을 지정합니다.\n입력 자료형: 클래스 선택 (UICCharacterCreateWidget).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요."))
    TSubclassOf<UICCharacterCreateWidget> CharacterCreateWidgetClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="실제 플레이 UI UICGameplayWidget 자식 WBP 클래스입니다. 클래스 에셋을 지정하고 Controller 상태/갱신 이벤트와 연결합니다.\n입력 자료형: 클래스 선택 (UICGameplayWidget).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSubclassOf<UICGameplayWidget> GameplayWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="IC|UI") TSoftClassPtr<UICQuestWidget> QuestWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="IC|UI") TSoftClassPtr<UICQuestTrackerWidget> QuestTrackerWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="IC|UI") TSubclassOf<UICNPCDialogueWidget> DialogueWidgetClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input", meta=(ToolTip="플레이 중 사용할 실제 IMC 에셋입니다. 로컬 플레이어 Enhanced Input에 MappingPriority로 등록합니다. 키 배정은 이 IMC에서 합니다.\n입력 자료형: 객체 참조 (UInputMappingContext).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UInputMappingContext> DefaultMappingContext;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input", meta=(ToolTip="IMC 등록 우선순위입니다. 로컬 Enhanced Input에서 다른 컨텍스트와 충돌할 때 사용하는 정수 우선순위입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 MappingPriority=0;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input", meta=(ToolTip="WASD 이동 IA 에셋입니다. Axis2D이며 IMC에서 W/S/A/D 축·Swizzle/Negate를 연결합니다.\n입력 자료형: 객체 참조 (UInputAction).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input", meta=(ToolTip="마우스 카메라 IA 에셋입니다. Axis2D 값을 사용하며 실제 키/축은 IMC에서 연결합니다.\n입력 자료형: 객체 참조 (UInputAction).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UInputAction> LookAction;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input", meta=(ToolTip="버튼 Input Action→명령/슬롯 연결 배열입니다. IMC의 실제 IA를 넣고 Move/Look는 별도 필드로 설정합니다. 중복 바인딩을 피합니다.\n입력 자료형: 구조/객체 (FICInputCommandBinding) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<FICInputCommandBinding> CommandActions;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input",meta=(ClampMin="0.05", ToolTip="Shift 달리기 유지 판정 실제 초입니다. 기본 0.2초이며 짧게 떼면 회피 요청, 유지하면 달리기 상태입니다. 입력/메뉴 중단 시 해제합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.2. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.05.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float SprintHoldSeconds=0.2f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input",meta=(ClampMin="0.05", ToolTip="TAB 유지 해제 기준 실제 초입니다. 현재 0.35초이며 짧게는 적 전환, 길게는 해제 요청입니다. 입력 중단 때 상태를 정리합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.35. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.05.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) float TargetHoldSeconds=0.35f;
    UPROPERTY(BlueprintReadOnly,Category="IC|Input", meta=(ToolTip="현재 Shift 유지 달리기 상태입니다. 입력 중단/메뉴/소유 변경 때 해제하고 실제 이동 속도에 반영합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bSprinting=false;
    UPROPERTY(BlueprintAssignable,Category="IC|UI", meta=(ToolTip="메뉴 변경 이벤트입니다. 새 키에 맞춰 UI를 표시하며 실제 시간 정지는 Controller가 관리합니다.\n입력 자료형: FICMenuChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICMenuChanged OnMenuChanged;
    UPROPERTY(BlueprintAssignable,Category="IC|UI", meta=(ToolTip="사용자 안내 문자열 변경 이벤트입니다. 표시용이며 지속 상태 원본과 구분합니다.\n입력 자료형: FICNoticeChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FICNoticeChanged OnNoticeChanged;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void FlushPressedKeys() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void ReceivedPlayer() override;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="실제 전투 실행을 요청하는 BP 이벤트입니다. Action/Slot을 실제 GAS·타깃·방어/회피 수신기에 연결해야 합니다. 수신기가 없으면 Controller는 미연결 안내를 표시합니다.\n입력 자료형: FPlayerCombatRequest.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerCombatRequest OnCombatRequest;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="생성/복원된 직업·첫 스킬을 알리는 이벤트입니다. 실제 전투/외형 초기화에 연결하고 초기품을 다시 지급하지 마세요.\n입력 자료형: FPlayerInitialized.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerInitialized OnPlayerInitialized;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="성장책 실제 증가 이벤트입니다. TargetID/Stat/Amount는 확정된 결과이며 효과를 다시 더하지 않습니다.\n입력 자료형: FPlayerStatBookApplied.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerStatBookApplied OnStatBookApplied;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="SP 고갈 상태 변화 이벤트입니다. 실제 이동 보정/공격 실행·UI 안내에 연결합니다.\n입력 자료형: FPlayerExhaustionChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerExhaustionChanged OnExhaustionChanged;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="수면 GameClock 점프 요청 이벤트입니다. 요청 분으로 일과/연출을 연결하며 SP 회복/시간 점프를 중복 적용하지 않습니다.\n입력 자료형: FPlayerSleepRequested.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerSleepRequested OnSleepRequested;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="장착 장비 기여 변경 이벤트입니다. Flat/Percent Modifiers를 실제 파생 계산에 한 번 연결하고 UI에서 상태를 재저장하지 않습니다.\n입력 자료형: FPlayerEquipmentStatsChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerEquipmentStatsChanged OnEquipmentStatsChanged;
    // Creation form fields; WorldState owns saved values after creation.
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="선택/저장 플레이어 이름입니다. 공백 제외 1~12자/줄바꿈 없음 규칙을 현재 생성/저장 검사에서 요구합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString PlayerName;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="외형 프리셋 0/1 인덱스입니다. 실제 외형 배열/성별 선택과 맞추고 기존 저장 선택값을 유지합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 Preset=0;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="선택/저장 직업 인덱스입니다. 0=전사,1=궁수,2=마법사,3=힐러입니다. Class enum의 None 포함 값과 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 Job=0;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="최초 Skill ID 기록입니다. 직업별 NewGame.StartingSkills에서 선택하며 실제 사용 가능한 스킬 콘텐츠/GAS 연결과 별개입니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName FirstSkill;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="컨트롤러의 플레이어 생성 완료 표시입니다. 지속 원본은 WorldState.bPlayerCreated이며 생성/최초 저장 흐름에서 갱신합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bCreated=false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="현재 전투 상태입니다. 메뉴/저장/생산 안전 판정에 사용하며 실제 전투 수신기에서 시작/종료를 연결합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bInCombat=false;
    UPROPERTY(BlueprintReadWrite, meta=(ToolTip="현재 대화 상태입니다. 저장/생산 안전 판정에 사용하며 대화 종료/위젯 제거 시 반드시 해제합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bInDialogue=false;
    UPROPERTY(BlueprintReadWrite, meta=(ToolTip="현재 이벤트 연출 상태입니다. 저장/생산 안전 판정에 사용하며 종료/레벨 전환 시 정리합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bInEvent=false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="현재 방어 입력 상태입니다. 실제 방어 성공/피해 감소는 OnCombatRequest 수신 전투 실행과 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bBlocking=false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="HP/MP/SP 표시/실행 상태입니다. 지속 원본과 참가자 캐시를 반영하고 직접 WBP에서 소모/저장을 중복 실행하지 않습니다.\n입력 자료형: 구조/객체 (FPlayerSurvivalState).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FPlayerSurvivalState Survival;
    UFUNCTION(BlueprintPure, Category="IC|Survival", meta=(ToolTip="현재 SP를 화면 표시용 정수로 반환합니다. 실제 자원 원본은 WorldState이며 소수 누적 시간은 따로 보존합니다.\n@return 현재 표시할 SP 정수입니다. 자료형: 정수.")) int32 GetDisplayedSP() const { IC_TRACE_FUNCTION(); return FMath::FloorToInt(FMath::Max(0.f,Survival.SP)); }
    UFUNCTION(BlueprintPure, Category="IC|Survival", meta=(ToolTip="최대 SP를 화면 표시용 정수로 반환합니다. 현재 SP와 구분합니다.\n@return 최대 SP 표시 정수입니다. 자료형: 정수.")) int32 GetDisplayedMaxSP() const { IC_TRACE_FUNCTION(); return FMath::FloorToInt(FMath::Max(0.f,Survival.MaxSP)); }
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Survival", meta=(ToolTip="실제 수면 게임 1분당 SP 회복입니다. 현재 Controller 수면 계산에서 사용하며 TimeSettings DT와 자동 동기화되지 않습니다.\n입력 자료형: 소수.\nC++ 초기값: 1.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float SleepSPRecoveryPerGameMinute=1.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Survival", meta=(ToolTip="SP 고갈 이동 속도 배율입니다. Controller에서는 실제 별도 설정, StatSettings에서는 규격이며 현재 자동 동기화되지 않습니다.\n입력 자료형: 소수.\nC++ 초기값: 0.65. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float ExhaustedMoveSpeedMultiplier=0.65f;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="기본 능력치 6종의 지속/표시 원본입니다. 장비/효과 최종 파생값과 구분하며 변경은 상태 API를 사용합니다.\n입력 자료형: 구조/객체 (FPlayerMainStats).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FPlayerMainStats MainStats;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="보유 골드 또는 보상 지급량입니다. 보유 원본은 WorldState.Gold이며 보상에서는 증가량입니다. 음수는 허용하지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 200. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 Gold=200;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="수동 저장 슬롯입니다. 1~4이며 자동 저장은 별도 0입니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 SelectedSaveSlot=1;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="로드 요청 슬롯입니다. 0=자동, 1~4=수동이며 실제 목록 bValid/백업을 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 SelectedLoadSlot=0;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="실제 저장 서비스에서 조회한 슬롯 목록입니다. 빈 슬롯/손상/백업 사용 여부를 구분하며 시험 슬롯 목록으로 대체하지 않습니다.\n입력 자료형: 구조/객체 (FICSaveSlotInfo) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<FICSaveSlotInfo> SaveSlots;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 확인 팝업 본문입니다. 직접 상태를 확정하는 값이 아니며 ConfirmModal 결과로 실제 동작을 요청합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString ModalText;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 시야/거리 안의 상호작용 안내 문자열입니다. 실제 물체 대상/사용 가능 여부를 입력 처리에서 다시 검사합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString InteractionHint;
    UFUNCTION(BlueprintPure,Category="IC|Input", meta=(ToolTip="현재 메뉴/팝업 상태에서 취소 버튼에 표시할 문구를 조회합니다.\n@return 현재 상태에 맞는 취소 문구입니다. 자료형: 문자열.")) FString MenuCancelLabel() const;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Items", meta=(ToolTip="지속 드롭을 표시할 AICDroppedItem 자식 BP 클래스입니다. 원본은 WorldState.Drops이며 이 클래스는 표시/줍기를 담당합니다.\n입력 자료형: 클래스 선택 (AICDroppedItem).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TSubclassOf<AICDroppedItem> DroppedItemClass;
    bool IsQuitAfterSave() const { IC_TRACE_FUNCTION(); return bExitAfterSave; }
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="저장 슬롯 선택 화면을 엽니다. 메뉴 정지 요청으로 월드 시간 진행을 멈추며 종료 후 저장 옵션을 함께 받을 수 있습니다.\n@param bQuitAfter true이면 저장 완료 성공 이후 종료 흐름을 요청합니다. 입력 핀. 자료형: 체크박스(true/false).")) void OpenSaveMenu(bool bQuitAfter=false);
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="불러오기 슬롯 선택 화면을 엽니다. 실제 로드는 슬롯 선택과 확인 후 수행합니다.")) void OpenLoadMenu();
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="저장/불러오기 세션 메뉴를 닫고 해당 메뉴의 정지 요청을 해제합니다. 다른 메뉴·팝업의 요청이 남으면 시간은 계속 정지합니다.")) void CloseSessionMenu();
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="선택한 슬롯의 수동 저장을 요청합니다. 저장 안전 조건과 비동기 완료 이벤트를 확인하세요.")) void RequestManualSave();
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="입력한 이름·직업·외형으로 실제 캐릭터 생성을 요청합니다. 새 게임 DT·장비·아이템 검사가 성공해야 WorldState를 확정합니다.")) void RequestCreatePlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="현재 상태에 맞는 종료 확인 흐름을 요청합니다. 저장 여부와 팝업 선택을 거쳐 종료합니다.")) void RequestQuit();
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="현재 선택한 저장 데이터의 삭제 확인 팝업을 요청합니다. 확인 전에는 저장 파일을 삭제하지 않습니다.\n@param bLegacy true는 이전 형식 저장, false는 현재 선택 슬롯 삭제 확인입니다. 입력 핀. 자료형: 체크박스(true/false).")) void RequestDelete(bool bLegacy=false);
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="열려 있는 확인 팝업의 수락/거절을 처리합니다. 팝업 종류에 따라 저장·로드·삭제·종료를 수행하므로 현재 팝업 상태를 확인하세요.\n@param bAccept true는 현재 확인 팝업 수락, false는 거절입니다. 입력 핀. 자료형: 체크박스(true/false).")) void ConfirmModal(bool bAccept);
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="추가 저장 없이 현재 종료 흐름을 실행합니다. 저장하지 않은 진행은 보존되지 않습니다.")) void QuitWithoutSave();
    void FinishSessionSave(int32 Slot,bool bSuccess);
    AICInteractionActor* FindInteractionTarget() const;
    AICNPCBase* FindNPCInteractionTarget() const;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 메뉴 키입니다. Create/Pause/Inventory/Equipment/Stats/Skills/Quest/Map/Party/Codex/Craft 등 기존 Controller 규칙을 사용합니다. SetMenu로 정지 요청/입력 모드를 함께 갱신하세요.\n입력 자료형: 문자열.\nC++ 초기값: 문자열 \"Create\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Menu=TEXT("Create");
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 사용자 안내 문구입니다. Show/OnNoticeChanged로 UI를 갱신하며 지속 플래그 원본으로 사용하지 않습니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Notice;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 열린 컨테이너 키입니다. 소유/접근 판정 뒤 설정하며 PlayerHome 공유 원본과 NPC 독립 키를 구분합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName OpenStorage;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 열린 작업대 종류입니다. Kitchen/Forge로 레시피 분류를 결정하며 작업대 상호작용에서 설정합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName ActiveStation;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 작업대 Anchor ID입니다. CraftRecipe 시작 위치 검사에 사용하며 메뉴만 임의로 열기 전에 유효한 작업대에서 설정해야 합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName ActiveWorkAnchor;
    UFUNCTION(meta=(ToolTip="생산 작업 변경 이벤트를 받는 내부 콜백입니다. 작업 상태와 안내 메시지로 현재 화면을 갱신합니다.\n@param JobID 생산 작업의 고유 GUID입니다. 시작에서는 출력이며 완료/취소에서는 동일 작업의 GUID를 전달합니다. 입력 핀. 자료형: 인스턴스 GUID.\n@param Message 처리 결과 안내 문구입니다. 실제 성공 여부와 함께 해석하세요. 입력 핀. 자료형: 문자열.")) void ProductionChanged(FGuid JobID,const FString& Message);
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 대상 선택 중인 Item ID입니다. 아직 소비된 상태가 아니며 ConfirmTarget/CancelTarget으로 종료합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName PendingItem;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 선택한 퀵슬롯 0~3입니다. -1은 가방 사용 등 퀵슬롯 없음입니다.\n입력 자료형: 정수.\nC++ 초기값: -1(미지정). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 PendingQuick=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="선택 가방 0부터 인덱스입니다. -1은 선택 없음이며 유효 범위를 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: -1(미지정). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 SelectedBag=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="선택 보관함 0부터 인덱스입니다. -1은 없음이며 현재 OpenStorage의 범위를 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: -1(미지정). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 SelectedStorage=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="가방 이동 시작 슬롯 0부터 인덱스입니다. -1은 이동 선택 없음이며 유효 배열 범위를 검사합니다.\n입력 자료형: 정수.\nC++ 초기값: -1(미지정). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 MoveFrom=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="현재 대상 선택 동료 키 슬롯입니다. -1은 없음, 0~3은 Z/X/C/V 대응이며 실제 WorldState 파티 ID를 조회합니다.\n입력 자료형: 정수.\nC++ 초기값: -1(미지정). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 PendingPartyKey=INDEX_NONE;
    bool bDirty=true;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="생성된 플레이 UI 인스턴스입니다. GameplayWidgetClass와 별개이며 제거 때 참조/이벤트를 정리합니다.\n입력 자료형: 객체 참조 (UICGameplayWidget).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UICGameplayWidget> Screen;
    UPROPERTY(Transient) TObjectPtr<UICQuestWidget> QuestScreen;
    UPROPERTY(Transient) TObjectPtr<UICQuestTrackerWidget> QuestTrackerScreen;
    UPROPERTY(Transient) TObjectPtr<UICNPCDialogueWidget> DialogueScreen;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI", meta=(ToolTip="생성된 캐릭터 생성 WBP 인스턴스입니다. 클래스 설정과 별개인 런타임 참조이며 닫기/제거 때 정리합니다.\n입력 자료형: 객체 참조 (UICCharacterCreateWidget).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UICCharacterCreateWidget> CharacterCreateScreen;
    UFUNCTION(BlueprintPure,Category="IC|Systems", meta=(ToolTip="이 컨트롤러의 실제 인벤토리 컴포넌트를 조회합니다. 아이템 변경은 이 컴포넌트의 거래 API를 사용하세요.\n@return 연결된 인벤토리 컴포넌트입니다. 없으면 None입니다. 자료형: UICInventoryComponent*.")) UICInventoryComponent* GetInventory() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems", meta=(ToolTip="이 컨트롤러의 레시피 조회·생산 요청 컴포넌트를 조회합니다.\n@return 연결된 제작 컴포넌트입니다. 없으면 None입니다. 자료형: UICCraftingComponent*.")) UICCraftingComponent* GetCrafting() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems", meta=(ToolTip="GameInstance의 지속 상태 원본 서브시스템을 조회합니다. 액터나 UI에 별도 원본을 만들지 마세요.\n@return WorldState 서브시스템입니다. GameInstance가 없으면 None입니다. 자료형: UICWorldStateSubsystem*.")) UICWorldStateSubsystem* GetWorldState() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems", meta=(ToolTip="GameInstance의 게임/플레이 시간과 공통 정지 요청 서브시스템을 조회합니다.\n@return 시간 서브시스템입니다. 연결되지 않으면 None입니다. 자료형: UICTimeSubsystem*.")) UICTimeSubsystem* GetTime() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems", meta=(ToolTip="GameInstance의 저장 안전 검사·슬롯 저장·로드 서브시스템을 조회합니다.\n@return 저장 서브시스템입니다. 연결되지 않으면 None입니다. 자료형: UICSaveSubsystem*.")) UICSaveSubsystem* GetSaves() const;
    UFUNCTION(BlueprintPure, meta=(ToolTip="플레이어 CHR_PLAYER의 표시용 상태 복사본을 조회합니다. 활성 실행 캐시와 현재 장착 정보를 조회 경로에서 반영합니다. 복사본을 수정해도 원본이 바뀌지 않습니다.\n@return 현재 플레이어 상태의 스냅샷입니다. 원본이 없으면 비어 있는 상태입니다. 자료형: 구조/객체 (FICCharacterState).")) FICCharacterState GetPlayerSnapshot() const;
    UFUNCTION(BlueprintPure,Category="IC|Creation", meta=(ToolTip="현재 단계에서 이름·직업·외형 선택을 변경할 수 있는지 검사합니다.\n@return 캐릭터 생성 선택을 수정할 수 있으면 true입니다. 자료형: 체크박스(true/false).")) bool CanEditCreation() const { IC_TRACE_FUNCTION(); return !bCreated && !bInitialSavePending; }
    UFUNCTION(BlueprintPure,Category="IC|Creation", meta=(ToolTip="현재 이름·직업·외형 선택과 단계가 생성 제출 조건을 만족하는지 검사합니다. 실제 DT 참조 검사는 생성 거래에서 다시 수행합니다.\n@return 생성 제출이 가능한 상태이면 true입니다. 자료형: 체크박스(true/false).")) bool CanSubmitCreation() const;
    /** Development-only helper used by the cheat manager to bypass the missing creation confirmation UI. */
    bool DebugCreatePlayer();
    bool ValidateCreationRequest(FString& OutReason) const;
    void ReportCreationFailure(const FString& Reason);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="현재 전투 여부를 컨트롤러에 반영합니다. 저장 안전 조건과 입력/UI 판단에 사용하며 실제 전투 실행 결과는 호출 측이 제공해야 합니다.\n@param Active 현재 실제 전투 중이면 true, 전투 종료면 false입니다. 입력 핀. 자료형: 체크박스(true/false).")) void SetCombatState(bool Active);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="계산한 최종 이동 속도를 컨트롤러와 플레이어 이동 경로에 반영합니다. 단위는 cm/초입니다. 걷기·달리기 입력 배율과 기본 이동 속도를 구분하세요.\n@param Speed 계산한 최종 이동 속도입니다. 단위는 cm/초입니다. 입력 핀. 자료형: 소수.")) void SetFinalMoveSpeed(float Speed);
    UFUNCTION(BlueprintCallable,Category="IC|Player", meta=(ToolTip="플레이어 골드를 지정한 총액으로 설정하는 거래를 요청합니다. 추가 지급량이 아니며 음수나 유효하지 않은 상태는 거부합니다.\n@param Amount 설정할 최종 보유 골드 정수입니다. 추가 지급량이 아닙니다. 입력 핀. 자료형: 정수.\n@return 골드 변경을 확정하면 true입니다. 검사 실패 시 원본을 변경하지 않습니다. 자료형: 체크박스(true/false).")) bool SetGold(int32 Amount);
    UFUNCTION(BlueprintPure,Category="IC|Player", meta=(ToolTip="현재 플레이어가 사망 단계인지 조회합니다. 동료의 Downed와 구분합니다.\n@return 플레이어 사망 상태이면 true입니다. 자료형: 체크박스(true/false).")) bool IsDead() const { IC_TRACE_FUNCTION(); return bDead; }
    UFUNCTION(BlueprintCallable, meta=(ToolTip="플레이어 사망 처리를 요청합니다. 게임 단계·입력·메뉴·저장 안전 상태를 사망 흐름으로 갱신합니다.")) void PlayerDied();
    UFUNCTION(BlueprintCallable, meta=(ToolTip="생산용 SP 소모를 요청합니다. 현재 플레이어 자원 원본에서 처리하며 생산 시작의 일반 거래 경로와 중복 호출하지 마세요.\n@param Amount 소모 요청 SP 양입니다. 0 이상의 유한값을 전달하세요. 입력 핀. 자료형: 소수.\n@return 실제로 소모한 SP 양입니다. 요청량과 같다고 가정하지 마세요. 자료형: 소수.")) float ConsumeProductionSP(float Amount);
    UFUNCTION(meta=(ToolTip="PlayClock 진행 이벤트를 받는 내부 콜백입니다. 플레이 시간 누적과 SP 감소를 처리하며 메뉴 정지 동안 이 진행이 발생하면 안 됩니다.\n@param Seconds 이번에 진행한 PlayClock 초입니다. 누적 총시간과 구분하며 메뉴 정지 중에는 진행하지 않습니다. 입력 핀. 자료형: 배정밀도 소수.")) void HandlePlaySecondsAdvanced(double Seconds);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="현재 수면 가능 조건을 검사하고 GameClock 시간 점프와 자원 회복을 요청합니다. PlayClock 생산 시간이 수면만으로 완료되지는 않습니다.\n@return 수면 처리를 수행하면 true입니다. 불가능한 상태이면 false입니다. 자료형: 체크박스(true/false).")) bool Sleep();
    UFUNCTION(BlueprintCallable, meta=(ToolTip="레시피 ID로 생산 시작을 요청합니다. 재료를 예약하고 SP를 지불한 뒤 PlayClock 시간이 지난 작업을 완료 거래로 처리합니다. 즉시 아이템을 만드는 함수로 사용하지 마세요.\n@param RecipeID 실제 Recipe DT의 RowName/ID입니다. 입력 핀. 자료형: 이름/ID.\n@return 작업 시작에 성공하면 true입니다. 완료 성공을 뜻하지 않습니다. 자료형: 체크박스(true/false).")) bool CraftRecipe(FName RecipeID);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="보관함 접근 상태를 검사하고 해당 ContainerID의 보관 화면을 엽니다. 플레이어 집 여부·접근 허용 값은 실제 소유/접근 판정 결과를 전달하세요.\n@param ContainerID 내용물을 조회/변경할 보관함의 안정적 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param bPlayerHome 해당 보관함이 실제 플레이어 집의 시설이면 true입니다. 입력 핀. 자료형: 체크박스(true/false).\n@param bAccessGranted 실제 소유/서비스 조건 검사에서 접근을 허용했으면 true입니다. 입력 핀. 자료형: 체크박스(true/false).")) void OpenContainer(FName ContainerID,bool bPlayerHome,bool bAccessGranted);
    UFUNCTION(BlueprintCallable,Category="IC|Input", meta=(ToolTip="입력 액션의 시작/실행 명령을 처리합니다. 슬롯 명령은 0~3이며 유지 입력은 ReleaseInputCommand도 연결해야 합니다. 전투 명령은 실제 OnCombatRequest 수신 구현이 필요합니다.\n@param Command 입력 명령 또는 기존 UI 명령 문자열입니다. 이 함수의 자료형과 허용 명령을 확인하세요. 입력 핀. 자료형: 선택 목록 (EICInputCommand).\n@param Slot 직업 스킬·동료 스킬·퀵슬롯의 0~3 번호입니다. 일반 명령은 기본값0을 사용합니다. 입력 핀. 자료형: 정수.")) void HandleInputCommand(EICInputCommand Command,int32 Slot=0);
    UFUNCTION(BlueprintCallable,Category="IC|Input", meta=(ToolTip="입력 액션의 Completed/Cancelled 해제를 처리합니다. 방어·달리기·Shift/TAB 유지 상태를 정리하여 키를 놓은 뒤 동작이 남지 않게 합니다.\n@param Command 입력 명령 또는 기존 UI 명령 문자열입니다. 이 함수의 자료형과 허용 명령을 확인하세요. 입력 핀. 자료형: 선택 목록 (EICInputCommand).\n@param Slot 해제하는 유지 입력의 슬롯 번호입니다. 시작 때 전달한 명령/슬롯과 맞추세요. 입력 핀. 자료형: 정수.")) void ReleaseInputCommand(EICInputCommand Command,int32 Slot=0);
    UFUNCTION(BlueprintCallable,Category="IC|UI", meta=(ToolTip="기존 화면의 문자열 명령과 선택 인덱스를 해석하는 UI 연결 경로입니다. 임의 문자열을 만들기보다 현재 위젯에서 사용하는 명령을 확인하세요. 게임 변경은 실제 시스템 API로 전달합니다.\n@param Command 입력 명령 또는 기존 UI 명령 문자열입니다. 이 함수의 자료형과 허용 명령을 확인하세요. 입력 핀. 자료형: 문자열.\n@param Index 현재 배열/가방/화면 선택의 0 기반 인덱스입니다. 정의 ID나 인스턴스 GUID와 구분합니다. 입력 핀. 자료형: 정수.")) void ExecuteUI(const FString& Command,int32 Index=0);
    UFUNCTION(BlueprintCallable,Category="IC|UI", meta=(ToolTip="현재 게임 메뉴를 전환하고 메뉴 정지·선택·대상 상태를 함께 갱신합니다. 빈 메뉴는 닫기 흐름에 사용합니다.\n@param NewMenu 전환할 기존 메뉴 키입니다. 임의 문자열은 연결된 화면이 없을 수 있습니다. 입력 핀. 자료형: 문자열.")) void SetMenu(const FString& NewMenu);
    UFUNCTION(BlueprintCallable,Category="IC|UI", meta=(ToolTip="현재 안내 문자열을 화면 메시지 상태와 갱신 이벤트로 전달합니다. 현지화 대사 원본을 관리하는 String Table 조회와 용도가 다릅니다.\n@param Text 현재 표시/입력할 문구입니다. FText는 현지화 가능 텍스트, FString은 직접 문자열입니다. 입력 핀. 자료형: 문자열.")) void Show(const FString& Text);
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="컨트롤러의 현재 슬롯 저장을 요청합니다. 실제 파일 저장 완료는 SaveFinished 이벤트에서 확인하세요.\n@return 저장 요청을 접수하면 true입니다. 비동기 저장 완료 성공을 뜻하지 않습니다. 자료형: 체크박스(true/false).")) bool SavePlayer();
    bool AutoSavePlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Session", meta=(ToolTip="컨트롤러의 현재 선택 슬롯을 로드합니다. 후보 상태를 검사하고 복원 가능한 실행 참여자에게 적용합니다.\n@return 검증과 로드 적용에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool LoadPlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Items", meta=(ToolTip="아이템 사용의 시작/대상 선택을 요청합니다. 직접 소모하지 말고 Prepare의 대상·효과 검사와 확정 거래를 통과해야 합니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Quick 사용을 요청한 퀵슬롯 0~3입니다. -1은 퀵슬롯 지정 없는 일반 아이템 선택입니다. 입력 핀. 자료형: 정수.")) void BeginItem(FName ID,int32 Quick=-1);
    UFUNCTION(BlueprintCallable,Category="IC|Items", meta=(ToolTip="선택한 캐릭터 ID로 현재 아이템 대상 선택을 확정합니다. 준비 이후 자원·인벤토리가 바뀌었는지 재검사합니다.\n@param TargetID 아이템 대상 캐릭터의 ID입니다. 플레이어는 CHR_PLAYER, NPC는 NPC 정의 ID를 사용합니다. 입력 핀. 자료형: 이름/ID.")) void ConfirmTarget(FName TargetID);
    UFUNCTION(BlueprintCallable,Category="IC|Items", meta=(ToolTip="0~3 동료 슬롯에서 캐릭터 ID를 찾아 현재 아이템 대상 선택을 확정합니다. 플레이어 자신의 선택 방식과 구분합니다.\n@param Slot 선택할 동료 슬롯 0~3입니다. 입력 핀. 자료형: 정수.")) void ConfirmPartyTarget(int32 Slot);
    void SyncSaveSafety();
    void RefreshSaveSlots();
    UFUNCTION(meta=(ToolTip="인벤토리 변경 이벤트의 내부 콜백입니다. 장비·퀵슬롯·가방 표시와 관련 화면을 갱신합니다.")) void InventoryChanged();
    UFUNCTION(meta=(ToolTip="인벤토리 안내 이벤트의 내부 콜백입니다. 결과 메시지를 컨트롤러 표시 경로로 전달합니다.\n@param Message 처리 결과 안내 문구입니다. 실제 성공 여부와 함께 해석하세요. 입력 핀. 자료형: 현지화 가능한 텍스트.")) void InventoryMessage(const FText& Message);
    UFUNCTION(meta=(ToolTip="아이템 지급 결과 이벤트의 내부 콜백입니다. 가방 수용 수량과 초과 수량을 안내하고 드롭 표시를 갱신합니다.\n@param ItemID 실제 Item DT의 RowName/ID입니다. 인스턴스 GUID와 구분합니다. 입력 핀. 자료형: 이름/ID.\n@param Accepted 이번 아이템 지급에서 가방에 수용한 정수 수량입니다. 입력 핀. 자료형: 정수.\n@param Overflow 이번 아이템 지급에서 가방을 초과한 정수 수량입니다. 월드 드롭 처리 여부도 확인하세요. 입력 핀. 자료형: 정수.")) void InventoryLootResult(FName ItemID,int32 Accepted,int32 Overflow);
    UFUNCTION(meta=(ToolTip="현재 WorldState의 유효 드롭을 실제 DropActor 표시와 맞춥니다. 저장 원본을 새 드롭으로 복제하여 중복 지급하면 안 됩니다.")) void SynchronizeDrops();
    UFUNCTION(meta=(ToolTip="캐릭터 상태 변경 이벤트의 내부 콜백입니다. 해당 ID의 자원·장비·파티 표시를 갱신합니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.")) void CharacterChanged(FName ID);
    UFUNCTION(meta=(ToolTip="시간 변경 이벤트의 내부 콜백입니다. 화면의 날짜·시간 표시와 현재 상태를 갱신합니다.")) void ClockChanged();
    UFUNCTION(meta=(ToolTip="저장 시작 이벤트의 내부 콜백입니다. 저장 중 표시와 입력 상태를 갱신합니다.\n@param Slot 저장을 시작한 슬롯 번호입니다. 0자동·1~4수동입니다. 입력 핀. 자료형: 정수.")) void SaveStarted(int32 Slot);
    UFUNCTION(meta=(ToolTip="저장 완료 이벤트의 내부 콜백입니다. 실제 성공 여부·실패 메시지를 표시하고 저장 후 종료 흐름을 처리합니다.\n@param Slot 저장 완료 결과의 슬롯 번호입니다. 0자동·1~4수동입니다. 입력 핀. 자료형: 정수.\n@param bSuccess 실제 처리 성공이면 true입니다. 비동기 저장의 완료 결과를 확인하는 기준입니다. 입력 핀. 자료형: 체크박스(true/false).\n@param Message 처리 결과 안내 문구입니다. 실제 성공 여부와 함께 해석하세요. 입력 핀. 자료형: 문자열.")) void SaveFinished(int32 Slot,bool bSuccess,const FString& Message);
    virtual bool ReadLiveState_Implementation(FName CharacterId,FICCharacterState& Snapshot) const override;
    virtual bool FlushToWorldState_Implementation(UICWorldStateSubsystem* WorldState) override;
    virtual bool CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
private:
    TSoftObjectPtr<UWorld> SessionGameplayMap;
    UPROPERTY(Transient,meta=(ToolTip="게임 중 컨트롤러가 연 저장·불러오기 WBP 인스턴스입니다. 메뉴 종료·맵 전환·액터 제거 시 화면과 구독을 정리합니다.")) TObjectPtr<UICSaveLoadWidget> SessionSaveLoadScreen;
    void LoadSelectedSessionSlot();
    void ExecuteCombatInput(FName Action,int32 Slot);
    FString LastEquipmentKey;
    TWeakObjectPtr<class AICCharacterBase> CombatTarget;
    double TargetOccludedAt=-1;
    UFUNCTION(meta=(ToolTip="GAS 실행 자원과 표시용 값을 동기화하는 내부 콜백입니다. 지속 상태 저장은 별도 거래에서 수행합니다.")) void CombatCacheChanged();
    void InitializeMainMenu();
    UPROPERTY(Transient, meta=(ToolTip="공통 컨트롤러가 생성한 메인 메뉴 WBP 인스턴스입니다. MainMenuWidgetClass와 별개인 실행 중 참조이며 EndPlay에서 화면을 제거하여 이벤트 구독을 정리합니다. 저장 데이터에 포함하지 않습니다.\n입력 자료형: 객체 참조(UICMainMenuWidget)."))
    TObjectPtr<UICMainMenuWidget> MainMenuWidget;
    FString SaveReturnMenu;
    FString LoadReturnMenu=TEXT("Create");
    FString QuitReturnMenu;
    FString ModalReturnMenu;
    FString ModalAction;
    int32 ModalSlot=INDEX_NONE;
    bool bExitAfterSave=false;
    bool bQuitAfterWrite=false;
    bool bSessionClosing=false;
    void ShowModal(const FString& Action,const FString& Text,int32 Slot=INDEX_NONE);
    void CompleteQuit();
    bool bItemTransactionInProgress=false;
    bool bDead=false;
    bool bInitialSavePending=false;
    bool bLastExhausted=false;
    void CreatePlayer();
    void FlushWorldState();
    void CancelTarget();
    void Interact();
    void OpenNPCDialogue(AICNPCBase* NPC);
    TMap<FGuid,TWeakObjectPtr<AICDroppedItem>> DropActors;
    void UpdateGamePhase();
    void TryInitializeGameplay();
    void BindPawnSystems();
    void UnbindPawnSystems();
    void UpdateScreens();
    void ApplyInputMapping();
    void RemoveInputMapping();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void CancelInputCommand(EICInputCommand Command,int32 Slot);
    void ResetHeldInput();
    bool bSprintKeyHeld=false;
    bool bTargetKeyHeld=false;
    bool bTargetHoldHandled=false;
    double SprintPressedAt=0;
    double TargetPressedAt=0;
    bool CanHandleGameplayInput() const;
    bool bGameplayInitialized=false;
    UPROPERTY(Transient, meta=(ToolTip="현재 컨트롤러가 이벤트 구독한 실제 인벤토리 컴포넌트입니다. Pawn 변경 때 이전 구독을 해제하고 새 컴포넌트에 연결합니다.\n입력 자료형: 객체 참조 (UICInventoryComponent).")) TObjectPtr<UICInventoryComponent> BoundInventory;
    UPROPERTY(Transient, meta=(ToolTip="현재 로컬 입력에 등록한 IMC 내부 참조입니다. 소유 Pawn/컨트롤러 변경 때 중복 등록/제거를 관리하며 직접 설정하지 않습니다.\n입력 자료형: 객체 참조 (UInputMappingContext).")) TObjectPtr<UInputMappingContext> AppliedMappingContext;
};
