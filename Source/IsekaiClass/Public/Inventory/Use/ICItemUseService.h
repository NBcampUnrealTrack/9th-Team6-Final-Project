#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICItemUseService.generated.h"
class UICInventoryComponent;
class UICDefinitionSubsystem;

USTRUCT(BlueprintType, meta=(ToolTip="아이템 사용 사전 준비 결과입니다. Prepare는 실제 원본을 변경하지 않습니다. 성공한 Character와 Inventory 후보를 같은 WorldState 거래로 확정해야 효과와 소모가 함께 적용됩니다.", ShortToolTip="아이템 사용 사전 준비 결과입니다."))
struct ISEKAICLASS_API FICItemUseResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회/준비 성공입니다. true일 때만 정의/후보 출력을 사용하고 false이면 원인 문자열을 확인합니다. 준비 성공과 원본 거래 확정 성공은 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bSuccess=false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName ItemID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="아이템 사용 대상 캐릭터 ID입니다. 플레이어 CHR_PLAYER와 실제 NPC ID를 사용하고 CompanionN 임의 별칭을 넣지 않습니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName TargetID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="오류가 발생한 필드 이름/경로입니다. 해당 DT 행/구조를 열어 원인과 함께 수정합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Field;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회/아이템/제작 실패 원인입니다. 실패 때 표시/로그로 확인하고 원본 변경을 계속하지 않습니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Reason;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="아이템 사용 준비 후 캐릭터 후보입니다. Prepare 자체는 원본을 바꾸지 않으며 Inventory 후보와 함께 같은 거래로 확정해야 합니다.\n입력 자료형: 구조/객체 (FICCharacterState).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICCharacterState Character;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="캐릭터의 인벤토리 컴포넌트 또는 WorldState 인벤토리 후보/원본입니다. 실제 컴포넌트는 지속 원본을 조회하며 WBP에 별도 보관 원본을 만들지 않습니다.\n입력 자료형: 구조/객체 (FICInventoryState).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICInventoryState Inventory;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="성장책이 올릴 기본 능력치입니다. 힘/민첩/지능/체력/매력/행운 중 하나이며 None은 유효한 성장 효과가 아닙니다.\n입력 자료형: 선택 목록 (EPlayerMainStat).\nC++ 초기값: None — 기본 능력치를 지정하지 않습니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EPlayerMainStat GrowthStat=EPlayerMainStat::None;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="성장 정수 증가량입니다. 정의 규격 +1/+3/+5를 상한까지 적용합니다. ItemUseResult에서는 실제 증가량입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) int32 GrowthAmount=0;
};

UCLASS(meta=(ToolTip="회복·성장책·부활·해제를 검증하고 소모/효과 후보를 준비합니다. 실제 확정은 Controller의 공통 거래 경로가 처리합니다. 상태 효과 적용 실행기가 없으면 소모 없이 실패합니다.", ShortToolTip="회복·성장책·부활·해제를 검증하고 소모/효과 후보를 준비합니다."))
class ISEKAICLASS_API UICItemUseService : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Preparation never mutates live state. Effects and consumption commit together.
    UFUNCTION(BlueprintCallable,Category="IC|Items", meta=(ToolTip="아이템 사용 가능 여부와 효과 적용 후보를 계산합니다. 현재 인벤토리·정의·대상을 검사하지만 여기서 아이템을 소모하거나 WorldState를 확정하지 않습니다. 확정 직전에 재검사하고 단일 거래로 적용해야 합니다.\n@param Inventory 거래에 사용할 실제 인벤토리 컴포넌트입니다. 현재 WorldState에 연결된 컴포넌트를 전달하세요. 입력 핀. 자료형: UICInventoryComponent*.\n@param Definitions 실제 정의 카탈로그를 사용하는 DefinitionSubsystem입니다. 임시 기본 데이터로 대체하지 마세요. 입력 핀. 자료형: UICDefinitionSubsystem*.\n@param ItemID 실제 Item DT의 RowName/ID입니다. 인스턴스 GUID와 구분합니다. 입력 핀. 자료형: 이름/ID.\n@param Target 아이템 효과를 계산할 캐릭터 상태 스냅샷입니다. 확정 전 현재 상태를 다시 검사하세요. 입력 핀. 자료형: 구조/객체 (FICCharacterState).\n@return bSuccess·후보 상태·실패 원인을 가진 준비 결과입니다. 준비 성공을 사용 완료로 해석하지 마세요. 자료형: 구조/객체 (FICItemUseResult)."))
    static FICItemUseResult Prepare(UICInventoryComponent* Inventory,UICDefinitionSubsystem* Definitions,FName ItemID,const FICCharacterState& Target);
};
