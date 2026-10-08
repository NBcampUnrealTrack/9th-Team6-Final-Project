#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "Data/Rows/Items/ICItemRows.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerInventoryMessage, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerLootResult, FName, ItemID, int32, Accepted, int32, Remainder);

UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent, ToolTip="플레이어 캐릭터가 소유하는 인벤토리 실행 컴포넌트입니다. 실제 원본은 WorldState이고 아이템 정의는 DefinitionSubsystem에서 조회합니다. 변경 실패 시 후보 상태를 폐기하며 예약 재료/잠금/장비 참조를 검사합니다.", ShortToolTip="플레이어 캐릭터가 소유하는 인벤토리 실행 컴포넌트입니다."))
class ISEKAICLASS_API UICInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICInventoryComponent();
    class UICDefinitionSubsystem* GetDefinitions() const;
    void BindDefinitions(class UICDefinitionSubsystem* Service) { DefinitionService=Service; }
    void BeginStaging(class UICDefinitionSubsystem* Service,int32 Class);
    void BindWorldState(class UICWorldStateSubsystem* InWorldState);
    UFUNCTION(BlueprintPure, meta=(ToolTip="현재 인벤토리 원본의 복사본을 조회합니다. 정상 플레이에서는 연결된 WorldState를 사용하고 준비 중에는 명시적 준비 상태를 사용합니다. 복사본 수정은 변경 거래가 아닙니다.\n@return 현재 인벤토리 스냅샷입니다. 자료형: 구조/객체 (FICInventoryState).")) FICInventoryState GetSnapshot() const;
    bool ApplySnapshot(const FICInventoryState& Snapshot) { IC_TRACE_FUNCTION(); return Commit(Snapshot); }
    bool ValidateSnapshot(const FICInventoryState& Snapshot) const;
    bool BuildConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const;
    bool BuildQuestItemConsumption(FName ID, int32 Count, FICInventoryState& OutSnapshot) const;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="인벤토리 원본 변경 뒤 발생하는 갱신 이벤트입니다. 위젯 제거/Pawn 변경 때 구독을 해제해 중복 알림을 막으세요.\n입력 자료형: FPlayerInventoryChanged.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerInventoryChanged OnChanged;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="인벤토리 성공/실패 안내 FText 이벤트입니다. 텍스트 표시용이며 아이템 확정 여부는 함수 결과를 확인합니다.\n입력 자료형: FPlayerInventoryMessage.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerInventoryMessage OnMessage;
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="아이템 획득 결과 이벤트입니다. ItemID/수용량/초과량을 안내하며 초과 드롭 거래는 이미 확정된 결과입니다.\n입력 자료형: FPlayerLootResult.\nBP에서 이벤트를 구독할 수 있습니다. 위젯 닫기/EndPlay에서 구독을 정리해 중복 수신을 막으세요.")) FPlayerLootResult OnLootResult;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="현재 플레이어 직업 0~3입니다. 전사/궁수/마법사/힐러 순서이며 Equipment.RequiredClass와 장착 때 비교합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 CurrentClass = 0;
    const struct FICItemRow* Find(FName ID) const;
    UFUNCTION(BlueprintPure,Category="IC|Items", meta=(ToolTip="Item DT를 검증하여 현재 인벤토리 표시/처리 구조로 변환합니다. 누락·폐기·잘못된 연결을 임시 기본 아이템으로 대체하지 않습니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Definition 조회된 정의의 출력 복사본입니다. 성공 결과를 먼저 검사하세요. 복사본 수정은 DT 원본을 변경하지 않습니다. 입력/출력 핀. 자료형: 구조/객체 (FPlayerItemDefinition).\n@return 조회와 변환에 성공하면 true입니다. false일 때 Definition을 사용하지 마세요. 자료형: 체크박스(true/false).")) bool GetItemDefinition(FName ID,FPlayerItemDefinition& Definition) const;
    UFUNCTION(BlueprintCallable, meta=(ToolTip="인벤토리를 초기 기본 상태로 재설정합니다. 현재 내용물이 제거되므로 새 게임/명시적 초기화 경로에서만 사용하세요.")) void ResetInventory();
    UFUNCTION(BlueprintCallable, meta=(ToolTip="실제 Item DT 정의의 아이템 지급을 시도합니다. 공간 초과 처리와 WorldState 드롭 저장을 함께 수행할 수 있습니다. 반환값은 성공 수량이 아닌 가방에 수용하지 못한 수량입니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Count 아이템 정수 수량입니다. 양수 값을 사용하세요. 입력 핀. 자료형: 정수.\n@return 가방에 들어가지 못한 남은 수량입니다. 0이면 가방에 전부 들어갔고, 초과 드롭 처리 여부도 결과 이벤트를 확인하세요. 자료형: 정수.")) int32 TryAddItem(FName ID, int32 Count);
    int32 TryAddItemInstance(const FPlayerItemInstance& Item);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="아이템을 지급합니다. bAllowPartial=false는 전량 수용 가능해야 확정하며, true는 일부 지급/초과 드롭을 처리할 수 있습니다. 부분 허용에서는 false여도 일부 변경이 발생할 수 있으므로 전량 거래에는 false 옵션을 사용하세요.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Count 아이템 정수 수량입니다. 양수 값을 사용하세요. 입력 핀. 자료형: 정수.\n@param bAllowPartial true는 일부 수용/초과 처리를 허용하며 false는 전량 수용 가능해야 확정합니다. 입력 핀. 자료형: 체크박스(true/false).\n@return 요청 전량을 가방에 수용했으면 true입니다. 부분 허용 시 false를 전체 롤백으로 해석하지 마세요. 자료형: 체크박스(true/false).")) bool AddItem(FName ID, int32 Count, bool bAllowPartial = true);
    UFUNCTION(BlueprintPure, meta=(ToolTip="현재 인벤토리에서 해당 Item ID의 수량을 합산합니다. 예약된 생산 재료의 사용 가능 수량과 단순 보유 수량을 구분하세요.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@return 현재 집계된 아이템 수량입니다. 없는 아이템은 0입니다. 자료형: 정수.")) int32 CountItem(FName ID) const;
    UFUNCTION(BlueprintCallable, meta=(ToolTip="Item ID의 지정 수량을 소비하는 거래를 수행합니다. 잠금·예약·수량·후보 상태 검사가 실패하면 부분 소비하지 않습니다.\n@param ID 조회/변경할 정의 또는 캐릭터 ID입니다. 함수가 요구하는 종류의 ID를 사용하고 RowName·ID 규칙을 지키세요. 입력 핀. 자료형: 이름/ID.\n@param Count 아이템 정수 수량입니다. 양수 값을 사용하세요. 입력 핀. 자료형: 정수.\n@return 소비 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool Consume(FName ID, int32 Count);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방의 두 0 기반 슬롯을 이동/교환 또는 허용된 스택 병합으로 처리합니다. 잠금과 생산 예약 상태를 검사합니다.\n@param From 이동 시작의 0 기반 가방 슬롯 인덱스입니다. 입력 핀. 자료형: 정수.\n@param To 이동 목적지의 0 기반 가방 슬롯 인덱스입니다. 입력 핀. 자료형: 정수.\n@return 이동 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool MoveSlot(int32 From, int32 To);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방 슬롯의 스택에서 지정 수량을 분리합니다. 유효 수량·빈 공간·잠금·예약을 검사합니다.\n@param From 이동 시작의 0 기반 가방 슬롯 인덱스입니다. 입력 핀. 자료형: 정수.\n@param Amount 현재 스택에서 분리할 양수 정수 수량입니다. 전체 수량보다 작은 값을 사용합니다. 입력 핀. 자료형: 정수.\n@return 분리 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool SplitStack(int32 From, int32 Amount);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방 슬롯의 잠금 상태를 변경합니다. 잠금은 폐기·소모 등 실수를 막는 상태이며 소유권과 다릅니다.\n@param Index 현재 배열/가방/화면 선택의 0 기반 인덱스입니다. 정의 ID나 인스턴스 GUID와 구분합니다. 입력 핀. 자료형: 정수.\n@return 잠금 변경을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool ToggleLock(int32 Index);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방 슬롯의 폐기 거래를 수행합니다. 퀘스트·잠금·예약 등 허용 조건을 검사합니다.\n@param Index 현재 배열/가방/화면 선택의 0 기반 인덱스입니다. 정의 ID나 인스턴스 GUID와 구분합니다. 입력 핀. 자료형: 정수.\n@return 폐기를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool Discard(int32 Index);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방 슬롯의 장비를 장착합니다. 직업·무기 계열·장비 참조·기존 장비 회수 공간을 함께 검사합니다.\n@param Index 현재 배열/가방/화면 선택의 0 기반 인덱스입니다. 정의 ID나 인스턴스 GUID와 구분합니다. 입력 핀. 자료형: 정수.\n@return 장착 교체 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool Equip(int32 Index);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="장비 슬롯을 해제하고 해당 인스턴스를 가방에 되돌립니다. 가방 공간과 후보 상태 검사가 실패하면 장비를 유지합니다.\n@param Slot 해제할 장비 위치 열거형입니다. Weapon/Clothes/Ring/Necklace를 사용합니다. 입력 핀. 자료형: 선택 목록 (EPlayerEquipSlot).\n@return 장비 해제 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool Unequip(EPlayerEquipSlot Slot);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방 장비의 다음 강화 단계를 시도합니다. 장비·강화 DT·재료·골드·최대 단계를 검사하고 시도 비용을 한 거래로 확정합니다. 확률 실패에서는 비용이 소모되고 강화 단계는 유지됩니다.\n@param BagIndex 실제 일반 가방의 0 기반 슬롯 인덱스입니다. 입력 핀. 자료형: 정수.\n@param Gold 강화에 사용할 현재 골드의 참조 입력/출력입니다. 시도 비용 확정 후 갱신되므로 실패 반환 시에도 변화를 확인하세요. 입력/출력 핀. 자료형: 정수.\n@return 강화 성공이면 true입니다. false는 사전 검사 실패와 비용을 지불한 확률 실패를 모두 포함하므로 메시지와 변경된 골드를 확인하세요. 자료형: 체크박스(true/false).")) bool EnhanceEquipment(int32 BagIndex,UPARAM(ref) int32& Gold);
    UFUNCTION(BlueprintPure, meta=(ToolTip="현재 장착 장비의 기본 수치·강화·추첨 옵션을 합산합니다. Modifiers의 고정값/비율 구분을 실제 파생 계산의 기준으로 사용하세요.\n@return 장비 보정 스냅샷입니다. 기본 캐릭터 능력치와 별개입니다. 자료형: 구조/객체 (FPlayerEquipmentStats).")) FPlayerEquipmentStats GetEquipmentStats() const;
    UFUNCTION(BlueprintCallable, meta=(ToolTip="일반 가방의 아이템을 현재 정렬 규칙으로 정리합니다. 인스턴스 GUID와 퀵슬롯 연결을 보존하고 잠금/예약 검사를 유지합니다.")) void SortBag();
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방 아이템 인스턴스를 0~3 퀵슬롯에 연결합니다. 가방 인덱스 자체를 저장하지 않고 인스턴스 GUID를 사용합니다.\n@param QuickIndex 퀵슬롯 0~3입니다. 화면의 1~4 키와 한 칸 차이입니다. 입력 핀. 자료형: 정수.\n@param BagIndex 실제 일반 가방의 0 기반 슬롯 인덱스입니다. 입력 핀. 자료형: 정수.\n@return 퀵슬롯 연결을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool BindQuickSlot(int32 QuickIndex, int32 BagIndex);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="현재 확장권 정의와 가방 확장 상한을 검사해 티켓 소비와 슬롯 증가를 함께 확정합니다.\n@return 확장 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool ExpandUsingTicket();
    UFUNCTION(BlueprintCallable, meta=(ToolTip="ContainerID의 보관함 원본이 없으면 생성합니다. 현재 보관함 기본 용량은 40칸이며 기존 내용물을 새로 덮어쓰지 않습니다.\n@param ContainerID 내용물을 조회/변경할 보관함의 안정적 ID입니다. 입력 핀. 자료형: 이름/ID.")) void EnsureStorage(FName ContainerID);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="가방과 ContainerID 보관함 사이에서 아이템을 이동합니다. 방향별 Index 의미·공간·잠금·생산 예약을 검사하고 함께 확정합니다.\n@param ContainerID 내용물을 조회/변경할 보관함의 안정적 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Index bToStorage=true이면 가방 슬롯, false이면 보관함 슬롯의 0 기반 인덱스입니다. 입력 핀. 자료형: 정수.\n@param bToStorage true는 가방에서 보관함으로, false는 보관함에서 가방으로 옮깁니다. 입력 핀. 자료형: 체크박스(true/false).\n@return 이동 거래를 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool Transfer(FName ContainerID, int32 Index, bool bToStorage);
    UFUNCTION(BlueprintCallable, meta=(ToolTip="여러 재료 비용을 소비하고 하나의 결과 Item ID/수량을 지급하는 즉시 거래입니다. 생산 대기 작업은 StartRecipe를 사용하세요. 참조·수량·공간 검사가 실패하면 부분 소비/지급하지 않습니다.\n@param Costs 소모할 재료 Item ID→양수 수량 목록입니다. 중복/공간/예약 검사를 통과해야 합니다. 입력 핀. 자료형: 이름/ID → 정수 맵.\n@param ResultID 지급할 Item DT의 정의 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param ResultCount 지급할 양수 아이템 수량입니다. 입력 핀. 자료형: 정수.\n@return 전체 교환을 확정하면 true입니다. 자료형: 체크박스(true/false).")) bool ExchangeItems(const TMap<FName, int32>& Costs, FName ResultID, int32 ResultCount);
    UFUNCTION(BlueprintPure, meta=(ToolTip="현재 연결된 실제 DT를 검사하여 필요한 아이템/레시피 정의가 유효한지 확인합니다. 오류 안내는 컴포넌트 메시지 경로로 전달합니다.\n@return 필요 정의 검사에 성공하면 true입니다. 자료형: 체크박스(true/false).")) bool ValidateDefinitions() const;
    bool ValidateState() const;
private:
    UPROPERTY(meta=(ToolTip="저장 원본 또는 WorldStateSubsystem 참조입니다. 상태를 반영/검증한 뒤 서비스로 접근하고 독립 WBP 원본을 만들지 않습니다.\n입력 자료형: 객체 참조 (class UICWorldStateSubsystem).")) TObjectPtr<class UICWorldStateSubsystem> WorldState;
    UPROPERTY(meta=(ToolTip="인벤토리의 명시적 정의 서브시스템 참조입니다. 없으면 GameInstance에서 조회하며 하드코딩 시험 정의로 대체하지 않습니다.\n입력 자료형: 객체 참조 (class UICDefinitionSubsystem).")) TObjectPtr<class UICDefinitionSubsystem> DefinitionService;
    FICInventoryState StagingState;
    bool bStaging=false;
    bool Commit(const FICInventoryState& Snapshot);
    UFUNCTION(meta=(ToolTip="WorldState 인벤토리 변경의 내부 콜백입니다. 컴포넌트 표시 상태와 BP 변경 이벤트를 갱신합니다.")) void WorldInventoryChanged();
    int32 Insert(FPlayerContainer& Container, FName ID, int32 Count, const FPlayerItemInstance* Original = nullptr) const;
    bool Remove(FPlayerContainer& Container, FName ID, int32 Count) const;
    bool RollRandomOptions(FPlayerItemInstance& Item,const FICItemRow& Definition) const;
    bool Fail(const TCHAR* Text);
    friend class UICProductionSubsystem;
    friend class FICRewardService;
    friend class AICDroppedItem;
    bool CommitLoot(const FICInventoryState& Inventory,FName ItemID,int32 Overflow);
};
