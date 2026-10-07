#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Engine/DataTable.h"
#include "Player/Stats/ICPlayerStats.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICInventoryTypes.generated.h"

UENUM(BlueprintType, meta=(ToolTip="런타임 아이템 표시·처리 분류입니다. 실제 정적 정의는 Item DT에서 조회합니다."))
enum class EPlayerItemKind : uint8 { Consumable UMETA(ToolTip="사용 가능한 소비 아이템입니다.\n런타임 아이템 표시·처리 분류입니다. 실제 정적 정의는 Item DT에서 조회합니다."), Material UMETA(ToolTip="생산 재료입니다.\n런타임 아이템 표시·처리 분류입니다. 실제 정적 정의는 Item DT에서 조회합니다."), Equipment UMETA(ToolTip="장착 장비입니다.\n런타임 아이템 표시·처리 분류입니다. 실제 정적 정의는 Item DT에서 조회합니다."), Quest UMETA(ToolTip="퀘스트 전용 아이템입니다.\n런타임 아이템 표시·처리 분류입니다. 실제 정적 정의는 Item DT에서 조회합니다."), Expansion UMETA(ToolTip="가방 확장권입니다.\n런타임 아이템 표시·처리 분류입니다. 실제 정적 정의는 Item DT에서 조회합니다.") };
UENUM(BlueprintType, meta=(ToolTip="장비를 장착할 위치입니다. 일반 가방 인덱스와 다른 선택값입니다."))
enum class EPlayerEquipSlot : uint8 { Weapon UMETA(ToolTip="무기 슬롯입니다.\n장비를 장착할 위치입니다. 일반 가방 인덱스와 다른 선택값입니다."), Clothes UMETA(ToolTip="의복 슬롯입니다.\n장비를 장착할 위치입니다. 일반 가방 인덱스와 다른 선택값입니다."), Ring UMETA(ToolTip="반지 슬롯입니다.\n장비를 장착할 위치입니다. 일반 가방 인덱스와 다른 선택값입니다."), Necklace UMETA(ToolTip="목걸이 슬롯입니다.\n장비를 장착할 위치입니다. 일반 가방 인덱스와 다른 선택값입니다."), None UMETA(ToolTip="장비 슬롯이 없는 값입니다.\n장비를 장착할 위치입니다. 일반 가방 인덱스와 다른 선택값입니다.") };
UENUM(BlueprintType, meta=(ToolTip="인벤토리에서 조회할 탭 분류입니다."))
enum class EPlayerItemTab : uint8 { Normal UMETA(ToolTip="일반 가방 탭입니다.\n인벤토리에서 조회할 탭 분류입니다."), Quest UMETA(ToolTip="퀘스트 아이템 탭입니다. 일반 가방 이동·폐기 규칙과 구분하세요.\n인벤토리에서 조회할 탭 분류입니다.") };
UENUM(BlueprintType, meta=(ToolTip="인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."))
enum class EPlayerItemCategory : uint8 { Consumable UMETA(ToolTip="일반 소비 아이템입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Food UMETA(ToolTip="음식입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Book UMETA(ToolTip="책입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Material UMETA(ToolTip="일반 재료입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Gathering UMETA(ToolTip="채집물입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Fish UMETA(ToolTip="물고기 아이템입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Crafted UMETA(ToolTip="제작 결과물입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Quest UMETA(ToolTip="퀘스트 물품입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Ticket UMETA(ToolTip="확장권 등 교환권입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Equipment UMETA(ToolTip="장비입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다."), Revival UMETA(ToolTip="부활 아이템입니다.\n인벤토리 표시·정렬용 아이템 세부 분류입니다. 사용 효과는 별도 Effect 값이 결정합니다.") };
UENUM(BlueprintType, meta=(ToolTip="아이템 등급입니다. 장비 옵션 풀의 등급별 규칙과 일치시켜야 합니다."))
enum class EPlayerItemGrade : uint8 { Common UMETA(ToolTip="일반 등급입니다.\n아이템 등급입니다. 장비 옵션 풀의 등급별 규칙과 일치시켜야 합니다."), Uncommon UMETA(ToolTip="고급 등급입니다.\n아이템 등급입니다. 장비 옵션 풀의 등급별 규칙과 일치시켜야 합니다."), Rare UMETA(ToolTip="희귀 등급입니다.\n아이템 등급입니다. 장비 옵션 풀의 등급별 규칙과 일치시켜야 합니다."), Epic UMETA(ToolTip="영웅 등급입니다.\n아이템 등급입니다. 장비 옵션 풀의 등급별 규칙과 일치시켜야 합니다."), Legendary UMETA(ToolTip="전설 등급입니다.\n아이템 등급입니다. 장비 옵션 풀의 등급별 규칙과 일치시켜야 합니다.") };

class UStaticMesh;

USTRUCT(BlueprintType, meta=(ToolTip="아이템의 공통 보관/표시/사용 메타데이터입니다. 실제 DT 행은 FICItemRow를 선택하세요. 일부 옛 장비 수치 필드는 호환 표시/검사용이며 실제 장비 규칙은 Equipment/Enhancement/Option DT를 사용합니다.", ShortToolTip="아이템의 공통 보관/표시/사용 메타데이터입니다."))
struct FPlayerItemDefinition : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.")) FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="실제 표시 FText 이름입니다. String Table 참조로 현지화 식별자를 유지합니다. NameKey 입력과 FText 연결은 별개입니다.\n입력 자료형: 현지화 가능한 텍스트.")) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="보통/퀘스트 인벤토리 구분입니다. 퀘스트품은 별도 QuestItems 원본과 버리기/판매 제한을 사용합니다.\n입력 자료형: 선택 목록 (EPlayerItemTab).\nC++ 초기값: Normal — 일반 가방 탭입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerItemTab Tab=EPlayerItemTab::Normal;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="목록/도감/아이템 분류입니다. 실행 종류 Kind와 다르며 분류만 바꿔 효과가 생성되지 않습니다.\n입력 자료형: 선택 목록 (EPlayerItemCategory).\nC++ 초기값: Material — 일반 재료입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerItemCategory Category=EPlayerItemCategory::Material;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="정의/아이템/효과 분류입니다. 선택한 종류가 요구하는 참조/수치/대상 규칙을 함께 작성합니다.\n입력 자료형: 선택 목록 (EPlayerItemKind).\nC++ 초기값: Material — 생산 재료입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerItemKind Kind=EPlayerItemKind::Material;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="무기/의복/반지/목걸이 위치입니다. 장비는 None을 쓰지 않고 Equipment.Slot과 같아야 합니다.\n입력 자료형: 선택 목록 (EPlayerEquipSlot).\nC++ 초기값: None — 장비 슬롯이 없는 값입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerEquipSlot EquipSlot=EPlayerEquipSlot::None;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="한 슬롯 최대 수량입니다. 1~99이며 장비는 1입니다. 강화/옵션이 다른 장비 인스턴스를 합치지 않습니다.\n입력 자료형: 정수.\nC++ 초기값: 99. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 StackLimit=99;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="체크 시 버리기 허용입니다. 퀘스트품은 해제하며 잠금/예약은 별도 검사합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bCanDiscard=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="판매 허용 정의입니다. 퀘스트품은 해제하며 실제 상점 거래 연결은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bCanSell=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="선물 허용 정의입니다. 실제 대상/선호/관계 거래 연결은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bCanGift=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="1개 구매 기본 정수 가격입니다. 음수는 금지하며 할인/비용 확정은 실제 상점 경로입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 BuyPrice=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="1개 판매 기본 정수 가격입니다. 판매 허용/잠금/예약 검사와 실제 거래가 필요합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 SellPrice=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="직업 제한입니다. Equipment 열거형 None은 제한 없음, 공통 아이템 정수 -1은 제한 없음/0~3은 전사·궁수·마법사·힐러입니다. 실제 장착은 Equipment를 확인합니다.\n입력 자료형: 정수.\nC++ 초기값: -1(미지정). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 RequiredClass=INDEX_NONE;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="성장책이 올릴 기본 능력치입니다. 힘/민첩/지능/체력/매력/행운 중 하나이며 None은 유효한 성장 효과가 아닙니다.\n입력 자료형: 선택 목록 (EPlayerMainStat).\nC++ 초기값: None — 기본 능력치를 지정하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerMainStat GrowthStat=EPlayerMainStat::None;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="성장 정수 증가량입니다. 정의 규격 +1/+3/+5를 상한까지 적용합니다. ItemUseResult에서는 실제 증가량입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 GrowthAmount=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="고정 HP 회복량입니다. MaxHP×RestoreHPRatio와 합산 후 상한에서 제한합니다. 음수는 실패이며 효과 없음 소비 정책을 확인합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float RestoreHP=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="고정 SP 회복량입니다. MaxSP×RestoreSPRatio와 합산 후 상한에서 제한합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float RestoreSP=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="새 아이템 기본 등급입니다. 실제 장비는 Equipment.DefaultGrade로 같은 등급의 옵션 풀을 선택합니다.\n입력 자료형: 선택 목록 (EPlayerItemGrade).\nC++ 초기값: Common — 일반 등급입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerItemGrade DefaultGrade=EPlayerItemGrade::Common;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="공통 아이템 강화 상한 호환/검사 값입니다. 실제 규칙은 Enhancement DT이며 이 값도 상태 검사에 사용하므로 맞춰 작성합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 MaxEnhanceLevel=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="공통 아이템 옵션 개수 검사 상한입니다. 실제 추첨은 등급별 OptionCount이며 저장 옵션 수가 이 값을 넘으면 실패합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 MaxRandomOptions=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="OptionPool DT ID입니다. 등급별 후보/개수를 조회하며 누락 또는 지원하지 않는 중복 설정이면 장비 생성이 실패합니다.\n입력 자료형: 이름/ID.")) FName OptionPoolID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="StatSettings에서는 기본 공격 공식 항, Item에서는 옛 장비 표시/호환 값입니다. 실제 장비 기여는 Equipment.BaseModifiers입니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float BaseAttack=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="StatSettings에서는 기본 방어 공식 항, Item에서는 옛 장비 표시/호환 값입니다. 실제 장비 기여는 Equipment.BaseModifiers입니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float BaseDefense=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="옛 장비 단계당 공격 증가 호환 값입니다. 현재 강화는 Enhancement의 원본 비율/능력치 목록을 사용합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float EnhanceAttackPerLevel=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="옛 장비 단계당 방어 증가 호환 값입니다. 현재 강화는 Enhancement DT를 사용합니다.\n입력 자료형: 소수.\nC++ 초기값: 0.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float EnhanceDefensePerLevel=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="옛 기본 강화 비용 호환 값입니다. 현재 비용은 단계별 GoldCost입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) int32 EnhanceBaseCost=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="옛 기본 강화 확률 호환 값입니다. 현재 확률은 단계별 SuccessProbability입니다.\n입력 자료형: 소수.\nC++ 초기값: 1.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) float EnhanceBaseSuccessRate=1.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="공통 아이템 장비 메시의 호환/표시 참조입니다. Equipment.WeaponMesh/소켓 규칙을 맞춥니다.\n입력 자료형: Soft Object 에셋 참조 (UStaticMesh).")) TSoftObjectPtr<UStaticMesh> EquipmentMesh;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, meta=(ToolTip="장비 부착 소켓 이름입니다. 실제 메시/스켈레톤에 존재해야 하며 정의 ID와 다릅니다.\n입력 자료형: 이름/ID.")) FName AttachSocket;
    FText GetLabel() const { IC_TRACE_FUNCTION(); return DisplayName; }
};

USTRUCT(BlueprintType, meta=(ToolTip="보유한 아이템 묶음 하나의 저장 상태입니다. ItemID는 정의, InstanceID는 고유 인스턴스입니다. 수량·등급·강화·옵션·소유·잠금이 저장되며 옵션을 다시 추첨해 복원하면 안 됩니다.", ShortToolTip="보유한 아이템 묶음 하나의 저장 상태입니다."))
struct FPlayerItemInstance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="보유 아이템/효과의 고유 GUID입니다. 같은 Item/Effect 정의의 여러 인스턴스를 구분하며 저장 때 유지합니다.\n입력 자료형: 인스턴스 GUID.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FGuid InstanceID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="정수 수량입니다. 요구/지급 정의에서는 양수, 보유 인스턴스에서는 0이 빈 슬롯입니다. 음수 상태는 거부합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Count=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="정의 후보 등급 또는 저장 인스턴스 등급입니다. 인스턴스 0~4는 Common/Uncommon/Rare/Epic/Legendary입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Grade=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="현재 강화 단계입니다. 0은 미강화이며 DT 단계 규칙/상한을 검사하고 저장 때 보존합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) int32 Upgrade=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="풀에서는 후보/가중치, 인스턴스에서는 Option ID→실제 수치 맵입니다. 복원 때 재추첨하지 않습니다.\n입력 자료형: 이름/ID → 소수 맵.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TMap<FName,float> Options;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="보유 아이템 소유 ID입니다. 기본 CHR_PLAYER이며 NPC 소유 장비는 실제 ID로 구분합니다.\n입력 자료형: 이름/ID.\nC++ 초기값: 문자열 \"CHR_PLAYER\". 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) FName OwnerID=TEXT("CHR_PLAYER");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="보유 아이템 잠금입니다. 체크 시 일반 소비/이동/버리기 등의 제한 검사에 사용하며 제작 예약과 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) bool bLocked=false;
    bool IsEmpty() const { IC_TRACE_FUNCTION(); return ItemID.IsNone()||Count<=0; }
};

USTRUCT(BlueprintType, meta=(ToolTip="슬롯 배열 형태의 컨테이너입니다. 비어 있는 슬롯도 배열 크기로 용량을 표현하므로 내용물 수와 배열 길이를 구분하세요.", ShortToolTip="슬롯 배열 형태의 컨테이너입니다."))
struct FPlayerContainer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame, meta=(ToolTip="스포너 정의 슬롯 목록 또는 컨테이너 인스턴스 슬롯 배열입니다. 빈 컨테이너 칸도 용량에 포함하며 스폰 인덱스는 중복시키지 않습니다.\n입력 자료형: 구조/객체 (FPlayerItemInstance) 배열.\n저장 직렬화 대상입니다. 실행 캐시와 중복 원본을 만들지 마세요.")) TArray<FPlayerItemInstance> Slots;
};
