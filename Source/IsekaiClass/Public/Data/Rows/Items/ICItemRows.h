#pragma once
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICItemRows.generated.h"

class UTexture2D;

UENUM(BlueprintType, meta=(ToolTip="아이템 사용의 허용 대상 분류입니다. Prepare에서 검사한 뒤 확정 직전에 상태를 다시 검사합니다."))
enum class EICItemTarget : uint8 { None UMETA(ToolTip="선택 대상이 없는 아이템입니다.\n아이템 사용의 허용 대상 분류입니다. Prepare에서 검사한 뒤 확정 직전에 상태를 다시 검사합니다."), PlayerOnly UMETA(ToolTip="플레이어 자신만 허용합니다.\n아이템 사용의 허용 대상 분류입니다. Prepare에서 검사한 뒤 확정 직전에 상태를 다시 검사합니다."), LivingParty UMETA(ToolTip="살아 있는 파티원을 허용합니다.\n아이템 사용의 허용 대상 분류입니다. Prepare에서 검사한 뒤 확정 직전에 상태를 다시 검사합니다."), DownedCompanion UMETA(ToolTip="Downed 동료만 허용합니다.\n아이템 사용의 허용 대상 분류입니다. Prepare에서 검사한 뒤 확정 직전에 상태를 다시 검사합니다."), NonCompanionRecruit UMETA(ToolTip="현재 동료 모드가 아닌 영입 가능 NPC를 허용합니다.\n아이템 사용의 허용 대상 분류입니다. Prepare에서 검사한 뒤 확정 직전에 상태를 다시 검사합니다.") };
UENUM(BlueprintType, meta=(ToolTip="아이템의 실제 사용 효과 분류입니다. 제작 재료·장비처럼 사용 효과가 없는 아이템은 None을 사용합니다."))
enum class EICItemEffect : uint8 { None UMETA(ToolTip="직접 사용 효과가 없습니다.\n아이템의 실제 사용 효과 분류입니다. 제작 재료·장비처럼 사용 효과가 없는 아이템은 None을 사용합니다."), Recovery UMETA(ToolTip="HP/MP 회복 효과입니다. 상한과 유효 대상 검사 후 소모와 함께 확정합니다.\n아이템의 실제 사용 효과 분류입니다. 제작 재료·장비처럼 사용 효과가 없는 아이템은 None을 사용합니다."), GrowthBook UMETA(ToolTip="지정 기본 능력치를 올리는 성장책입니다. 상한과 증가량을 검사합니다.\n아이템의 실제 사용 효과 분류입니다. 제작 재료·장비처럼 사용 효과가 없는 아이템은 None을 사용합니다."), Revival UMETA(ToolTip="Downed 동료를 부활시키는 효과입니다.\n아이템의 실제 사용 효과 분류입니다. 제작 재료·장비처럼 사용 효과가 없는 아이템은 None을 사용합니다."), Dispel UMETA(ToolTip="해제 가능한 상태 효과를 제거합니다.\n아이템의 실제 사용 효과 분류입니다. 제작 재료·장비처럼 사용 효과가 없는 아이템은 None을 사용합니다."), Expansion UMETA(ToolTip="가방 확장권 효과입니다. 가방의 확장 상한을 검사합니다.\n아이템의 실제 사용 효과 분류입니다. 제작 재료·장비처럼 사용 효과가 없는 아이템은 None을 사용합니다.") };

// Shared inventory metadata plus explicit, typed gameplay effects and definition links.
USTRUCT(BlueprintType, meta=(ToolTip="Item DT의 실제 아이템 정의입니다. 아이템 ID·보관 규칙·사용 대상·효과·장비/작물 참조를 작성합니다. 회복·성장·부활·해제는 준비 후 WorldState 거래로 함께 확정하며 상태 효과 적용 실행기는 미연결이면 소모 없이 실패합니다.", ShortToolTip="Item DT의 실제 아이템 정의입니다."))
struct ISEKAICLASS_API FICItemRow : public FPlayerItemDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Texture2D 아이콘입니다. 실제 UI/표시 실행에서 이 참조를 사용하도록 연결합니다. 비면 전용 아이콘이 없습니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).")) TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="아이템 대상 제한입니다. 플레이어/살아 있는 파티/Downed 동료/비동료 영입을 구분하고 생존·모드·슬롯을 검사합니다.\n입력 자료형: 선택 목록 (EICItemTarget).\nC++ 초기값: None — 선택 대상이 없는 아이템입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICItemTarget UseTarget = EICItemTarget::None;
    // Dialogue use may additionally target a living recruit outside the party.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="체크하면 LivingParty 아이템을 파티 밖 살아 있는 영입 NPC에게도 허용합니다. 해제하면 플레이어/파티만 허용하며 주민/배경 NPC 대상 허용이 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bAllowNonCompanionRecruitUse = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="사용 효과입니다. 회복/성장/부활/해제는 준비/거래 경로입니다. Expansion은 별도 티켓 확장 경로이며 None은 사용 실패입니다.\n입력 자료형: 선택 목록 (EICItemEffect).\nC++ 초기값: None — 직접 사용 효과가 없습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICItemEffect UseEffect = EICItemEffect::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="아이템 적용 StatusEffect ID 목록입니다. 현재 모든 참조를 검사한 뒤 적용 실행기가 미연결이면 전체 실패하며 소모/회복도 적용하지 않습니다.\n입력 자료형: 이름/ID 배열.")) TArray<FName> StatusEffectIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="해제 대상 태그 필터입니다. 현재 비면 모든 해제 가능한 유해 효과, 값이 있으면 효과 Tags/GrantedStateTags 중 하나 이상 일치해야 합니다. AllowedCureItemIDs도 검사합니다.\n입력 자료형: GameplayTag 목록.")) FGameplayTagContainer DispelTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="고정 MP 회복량입니다. MaxMP×RestoreMPRatio와 합산 후 상한에서 제한합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double RestoreMP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1", ToolTip="최대 HP 회복 비율입니다. 0.25=25%이며 고정 RestoreHP와 합산합니다. 예: HP 60/100, 고정 0, 비율 0.25이면 85입니다. 비정상 값이면 소모 없이 실패합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.")) double RestoreHPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1", ToolTip="최대 MP 회복 비율입니다. 0.25=MaxMP의 25%이며 고정 RestoreMP와 합산합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.")) double RestoreMPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1", ToolTip="최대 SP 회복 비율입니다. 0.25=MaxSP의 25%이며 고정 RestoreSP와 합산합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.")) double RestoreSPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1", ToolTip="Downed 파티 동료 부활 HP 비율입니다. 0보다 크고 1 이하이며 0.5=50%입니다. 현재 아이템은 실제 Downed 파티 동료만 허용합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.")) double RevivalHPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1", ToolTip="아이템 부활의 최소 MP 비율입니다. 현재 MP와 MaxMP×비율 중 큰 값을 사용해 MP를 낮추지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.")) double RevivalMinimumMPRatio = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Equipment DT 규칙 ID입니다. Item ID와 별개입니다. 예: EQU_WPN_001은 EQU_WPN_001을 연결하고 Equipment.ItemID는 원래 아이템을 가리킵니다.\n입력 자료형: 이름/ID.")) FName EquipmentID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Crop DT 정의 ID입니다. 씨앗 Item.CropID 또는 실제 심어진 CropState에서 정적 성장 규칙을 찾습니다.\n입력 자료형: 이름/ID.")) FName CropID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="도구 분류 태그입니다. 현재 레시피는 Item ID/수량을 검사하므로 태그만으로 자동 요구 충족되지 않습니다.\n입력 자료형: GameplayTag 목록.")) FGameplayTagContainer ToolTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="성장 상한에서도 책 소비를 허용합니다. 해제하면 상한에서 소모 없이 실패하며 현재 능력치보다 낮은 비정상 상한은 거부합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bConsumeAtGrowthCap = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="체크하면 회복/해제 등 변화가 없을 때 소모하지 않습니다. 해제하면 무효과 소비를 허용합니다. 성장 상한 예외는 bConsumeAtGrowthCap를 확인합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 켜짐(true). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bRejectWhenNoEffect = true;
};

USTRUCT(BlueprintType, meta=(ToolTip="장비 아이템과 연결되는 장착·기본 능력치·강화·옵션 정의입니다. Item.EquipmentID로 이 행을 참조하고 이 행의 ItemID는 원래 아이템을 가리켜야 합니다. 장비 생성·강화·통계 조회에서 사용합니다.", ShortToolTip="장비 아이템과 연결되는 장착·기본 능력치·강화·옵션 정의입니다."))
struct ISEKAICLASS_API FICEquipmentRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.")) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="장착 슬롯입니다. Weapon/Clothes/Ring/Necklace를 선택하고 원래 Item.EquipSlot과 일치해야 합니다. None이면 유효한 장비가 아닙니다.\n입력 자료형: 선택 목록 (EPlayerEquipSlot).\nC++ 초기값: None — 장비 슬롯이 없는 값입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerEquipSlot Slot = EPlayerEquipSlot::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="검·활·지팡이·완드 계열 분류입니다. 직업/장착/옵션 제한과 함께 확인합니다.\n입력 자료형: 선택 목록 (EICWeaponFamily).\nC++ 초기값: None — 무기 계열이 없는 장비 또는 미지정 값입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICWeaponFamily WeaponFamily = EICWeaponFamily::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="직업 제한입니다. Equipment 열거형 None은 제한 없음, 공통 아이템 정수 -1은 제한 없음/0~3은 전사·궁수·마법사·힐러입니다. 실제 장착은 Equipment를 확인합니다.\n입력 자료형: 선택 목록 (EICCharacterClass).\nC++ 초기값: None — 직업을 지정하지 않은 상태입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICCharacterClass RequiredClass = EICCharacterClass::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="장비의 기본 Flat/Percent 기여입니다. 강화는 EnhancedStats에 포함된 Flat에 원본 증가 비율을 누적합니다.\n입력 자료형: 구조/객체 (FICStatModifier) 배열.")) TArray<FICStatModifier> BaseModifiers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="장비 StaticMesh 참조입니다. AttachSocket·AttachmentTransform과 실제 표시 실행을 함께 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UStaticMesh).")) TSoftObjectPtr<UStaticMesh> WeaponMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="장비 부착 소켓 이름입니다. 실제 메시/스켈레톤에 존재해야 하며 정의 ID와 다릅니다.\n입력 자료형: 이름/ID.")) FName AttachSocket;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="소켓 기준 장비 상대 위치(cm)·회전(도)·배율입니다. 월드 스폰 좌표와 다릅니다.\n입력 자료형: 위치·회전·배율.")) FTransform AttachmentTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Enhancement DT 규칙 ID입니다. 최대 단계·단계별 비용/확률/원본 증가율을 조회합니다.\n입력 자료형: 이름/ID.")) FName EnhancementID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="OptionPool DT ID입니다. 등급별 후보/개수를 조회하며 누락 또는 지원하지 않는 중복 설정이면 장비 생성이 실패합니다.\n입력 자료형: 이름/ID.")) FName OptionPoolID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="새 아이템 기본 등급입니다. 실제 장비는 Equipment.DefaultGrade로 같은 등급의 옵션 풀을 선택합니다.\n입력 자료형: 선택 목록 (EPlayerItemGrade).\nC++ 초기값: Common — 일반 등급입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EPlayerItemGrade DefaultGrade = EPlayerItemGrade::Common;
};

USTRUCT(BlueprintType, meta=(ToolTip="강화 목표 단계 하나의 비용·성공 확률·원본 기본 능력치 증가 비율입니다. 실패해도 시도 비용은 소모되지만 단계는 유지됩니다. 확정은 아이템과 골드를 한 거래로 검사합니다.", ShortToolTip="강화 목표 단계 하나의 비용·성공 확률·원본 기본 능력치 증가 비율입니다."))
struct ISEKAICLASS_API FICEnhancementStep
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10", ToolTip="도달 강화 단계입니다. 1~10이며 다음 목표는 Upgrade+1입니다. 중복/누락 없이 작성합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10.")) int32 TargetLevel = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="강화 단계 정수 골드 비용입니다. 0이면 무료입니다. 확률 실패에도 비용은 소모되며 아이템/골드를 함께 확정합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) int32 GoldCost = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1", ToolTip="강화 성공 확률입니다. 0.8=80%입니다. 실패해도 GoldCost는 소모되고 단계는 유지됩니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.")) double SuccessProbability = 1;
    // Percentage of the ORIGINAL specified base stat, never compounded per attempt.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="원본 Flat 능력치의 단계별 추가 비율입니다. 0.05=5%이며 복리로 계산하지 않습니다. 원본 10, 두 단계 각각 0.1이면 총 12입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.05. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double OriginalBaseBonusRatio = 0.05;
};

USTRUCT(BlueprintType, meta=(ToolTip="장비의 최대 강화 단계·강화할 능력치·단계별 규칙입니다. 1부터 MaximumLevel까지 빠짐없이 작성하고 Equipment.EnhancementID로 연결합니다.", ShortToolTip="장비의 최대 강화 단계·강화할 능력치·단계별 규칙입니다."))
struct ISEKAICLASS_API FICEnhancementRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="10", ToolTip="강화 최대 단계입니다. 1~10이며 1부터 이 값까지 규칙이 필요합니다.\n입력 자료형: 정수.\nC++ 초기값: 10. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 10.")) int32 MaximumLevel = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="강화할 능력치 목록입니다. 목록 밖의 기본 기여는 강화되어도 유지됩니다.\n입력 자료형: 선택 목록 (EICStat) 배열.")) TArray<EICStat> EnhancedStats;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="강화 단계 또는 보스 패턴 단계 목록입니다. 강화는 목표 레벨별, 패턴은 시간/Skill별 규칙을 작성합니다.\n입력 자료형: 구조/객체 (FICEnhancementStep) 배열.")) TArray<FICEnhancementStep> Steps;
};

USTRUCT(BlueprintType, meta=(ToolTip="장비 옵션 하나의 능력치·고정/비율 구분·최솟값/최댓값·장착 제한입니다. 생성된 인스턴스에는 실제 Option ID와 추첨 수치를 저장합니다.", ShortToolTip="장비 옵션 하나의 능력치·고정/비율 구분·최솟값/최댓값·장착 제한입니다."))
struct ISEKAICLASS_API FICOptionRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="효과/조건/옵션 능력치 종류입니다. 기본/파생 능력치를 구분하며 None 허용 여부는 상위 규칙을 확인합니다.\n입력 자료형: 선택 목록 (EICStat).\nC++ 초기값: None — 능력치 대상을 지정하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICStat Stat = EICStat::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="옵션을 Percent 기여로 넣을지 선택합니다. 체크=가산 비율, 해제=Flat입니다. 치명타 확률처럼 값 자체가 비율인 Flat도 있으므로 Stat 단위를 확인합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bPercentage = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="옵션 추첨 최솟값입니다. MaximumValue 이하 유한 수치이며 비율/확률 0.01=1%입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) double MinimumValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="옵션 추첨 최댓값입니다. MinimumValue 이상이며 저장 수치도 범위 안이어야 합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) double MaximumValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="옵션 허용 슬롯입니다. 현재 비면 제한 없음, 값이 있으면 Gear.Slot이 포함되어야 합니다.\n입력 자료형: 선택 목록 (EPlayerEquipSlot) 배열.")) TArray<EPlayerEquipSlot> AllowedSlots;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="옵션 허용 무기 계열입니다. 현재 비면 제한 없음, 값이 있으면 Gear.WeaponFamily가 포함되어야 합니다.\n입력 자료형: 선택 목록 (EICWeaponFamily) 배열.")) TArray<EICWeaponFamily> AllowedWeaponFamilies;
};

USTRUCT(BlueprintType, meta=(ToolTip="옵션 풀 안의 후보 하나입니다. OptionID의 Option DT 행과 가중치를 지정합니다. 선택 확률은 양수 가중치 합에 대한 비율입니다.", ShortToolTip="옵션 풀 안의 후보 하나입니다."))
struct ISEKAICLASS_API FICWeightedOption
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Option DT ID입니다. 예: OPT_ATTACK_SPEED. 임의 옵션 이름을 넣지 말고 수치 범위를 확인합니다.\n입력 자료형: 이름/ID.")) FName OptionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ToolTip="후보 상대 가중치입니다. 후보 1과 3이면 각각 25%/75%입니다. 0은 선택 기여 없음이며 음수/비정상 값은 사용하지 않습니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double Weight = 1;
};

USTRUCT(BlueprintType, meta=(ToolTip="특정 장비 등급의 옵션 개수와 후보 목록입니다. 현재 생성기는 중복 없이 가중치로 추첨하며 지정 개수만큼 유효한 후보가 있어야 합니다.", ShortToolTip="특정 장비 등급의 옵션 개수와 후보 목록입니다."))
struct ISEKAICLASS_API FICGradeOptionPool
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="정의 후보 등급 또는 저장 인스턴스 등급입니다. 인스턴스 0~4는 Common/Uncommon/Rare/Epic/Legendary입니다.\n입력 자료형: 선택 목록 (EPlayerItemGrade).\nC++ 초기값: Common — 일반 등급입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 생성기는 중복 없이 가중치로 추첨하며 지정 개수만큼 유효한 후보가 있어야 합니다.")) EPlayerItemGrade Grade = EPlayerItemGrade::Common;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="4", ToolTip="등급별 추첨 개수 0~4입니다. 0은 없음이며 중복 없이 뽑을 양수 가중치 후보가 충분해야 합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 4.\n소속 정의/시스템: 현재 생성기는 중복 없이 가중치로 추첨하며 지정 개수만큼 유효한 후보가 있어야 합니다.")) int32 OptionCount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="풀에서는 후보/가중치, 인스턴스에서는 Option ID→실제 수치 맵입니다. 복원 때 재추첨하지 않습니다.\n입력 자료형: 구조/객체 (FICWeightedOption) 배열.\n소속 정의/시스템: 현재 생성기는 중복 없이 가중치로 추첨하며 지정 개수만큼 유효한 후보가 있어야 합니다.")) TArray<FICWeightedOption> Options;
};

USTRUCT(BlueprintType, meta=(ToolTip="장비 등급별 옵션 추첨 규칙입니다. Equipment.OptionPoolID로 연결합니다. 현재 생성 경로는 중복 옵션을 허용하는 설정을 지원하지 않아 해당 설정이면 생성을 거부합니다.", ShortToolTip="장비 등급별 옵션 추첨 규칙입니다."))
struct ISEKAICLASS_API FICOptionPoolRow : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="등급별 옵션 규칙입니다. 실제 DefaultGrade 항목을 작성하고 등급을 중복시키지 않습니다.\n입력 자료형: 구조/객체 (FICGradeOptionPool) 배열.\n소속 정의/시스템: 현재 생성 경로는 중복 옵션을 허용하는 설정을 지원하지 않아 해당 설정이면 생성을 거부합니다.")) TArray<FICGradeOptionPool> Grades;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="같은 옵션 중복 추첨 정의입니다. 현재 체크된 풀은 생성 실패하므로 실제 풀은 해제합니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 생성 경로는 중복 옵션을 허용하는 설정을 지원하지 않아 해당 설정이면 생성을 거부합니다.")) bool bAllowDuplicateOptions = false;
};
