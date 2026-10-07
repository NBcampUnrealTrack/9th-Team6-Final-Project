#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICProductionTypes.generated.h"


UENUM(BlueprintType, meta=(ToolTip="생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."))
enum class EICLifeActivity : uint8
{
    Crafting UMETA(ToolTip="제작 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."),
    Cooking UMETA(ToolTip="요리 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."),
    Planting UMETA(ToolTip="씨앗 심기 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."),
    Harvesting UMETA(ToolTip="작물 수확 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."),
    Fishing UMETA(ToolTip="낚시 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."),
    Woodcutting UMETA(ToolTip="벌목 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."),
    Mining UMETA(ToolTip="채광 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다."),
    Gathering UMETA(ToolTip="채집 활동입니다.\n생산 작업의 활동 분류입니다. 현재 일반 레시피 제작/요리 외 활동의 전용 실행 연결은 필요합니다.")
};

UENUM(BlueprintType, meta=(ToolTip="생산 완료 결과의 형태입니다. 현재 레시피 경로는 BagItems만 실행하고 다른 종류는 전용 실행이 필요합니다."))
enum class EICProductionResultKind : uint8
{
    BagItems UMETA(ToolTip="결과 아이템을 가방에 지급합니다. 공간 부족 시 작업을 Ready 상태로 보존합니다.\n생산 완료 결과의 형태입니다. 현재 레시피 경로는 BagItems만 실행하고 다른 종류는 전용 실행이 필요합니다."),
    InstallStorage UMETA(ToolTip="보관함 설치 결과입니다.\n생산 완료 결과의 형태입니다. 현재 레시피 경로는 BagItems만 실행하고 다른 종류는 전용 실행이 필요합니다."),
    PlantCrop UMETA(ToolTip="작물을 심는 결과입니다.\n생산 완료 결과의 형태입니다. 현재 레시피 경로는 BagItems만 실행하고 다른 종류는 전용 실행이 필요합니다."),
    HarvestCrop UMETA(ToolTip="작물을 수확하는 결과입니다.\n생산 완료 결과의 형태입니다. 현재 레시피 경로는 BagItems만 실행하고 다른 종류는 전용 실행이 필요합니다."),
    FishingCatch UMETA(ToolTip="낚시 획득 결과입니다.\n생산 완료 결과의 형태입니다. 현재 레시피 경로는 BagItems만 실행하고 다른 종류는 전용 실행이 필요합니다."),
    GatherResource UMETA(ToolTip="자원 채집 결과입니다.\n생산 완료 결과의 형태입니다. 현재 레시피 경로는 BagItems만 실행하고 다른 종류는 전용 실행이 필요합니다.")
};

/** Tools are checked in the bag; they are never equipped, consumed or worn out. */
USTRUCT(BlueprintType, meta=(ToolTip="작업에 필요한 도구 아이템과 보유 수량입니다. 재료와 달리 현재 레시피 시작 경로에서는 존재만 검사하며 소모하지 않습니다.", ShortToolTip="작업에 필요한 도구 아이템과 보유 수량입니다."))
struct ISEKAICLASS_API FICProductionToolRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tool", meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 재료와 달리 현재 레시피 시작 경로에서는 존재만 검사하며 소모하지 않습니다."))
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tool", meta=(ClampMin="1", ToolTip="정수 수량입니다. 요구/지급 정의에서는 양수, 보유 인스턴스에서는 0이 빈 슬롯입니다. 음수 상태는 거부합니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 재료와 달리 현재 레시피 시작 경로에서는 존재만 검사하며 소모하지 않습니다."))
    int32 Count = 1;
};

UENUM(BlueprintType, meta=(ToolTip="재료 소모 시점의 정적 규칙입니다. 현재 레시피 실행기는 Recipe.Ingredients를 예약 후 완료 시 소모합니다."))
enum class EICProductionMaterialTiming : uint8
{
    
    ConsumeOnSuccess UMETA(ToolTip="성공 완료 거래에서 재료를 소모하는 규칙입니다.\n재료 소모 시점의 정적 규칙입니다. 현재 레시피 실행기는 Recipe.Ingredients를 예약 후 완료 시 소모합니다."),
    
    ConsumeAtStart UMETA(ToolTip="시작 거래에서 재료를 소모하는 규칙입니다. 현재 레시피 실행에 자동 연결된 것으로 생각하지 마세요.\n재료 소모 시점의 정적 규칙입니다. 현재 레시피 실행기는 Recipe.Ingredients를 예약 후 완료 시 소모합니다.")
};

USTRUCT(BlueprintType, meta=(ToolTip="생산 재료와 소모 시점 정의입니다. 현재 레시피 실행기는 Recipe.Ingredients를 성공 시 소모할 재료로 예약하므로 Production.Materials의 모든 소모 방식이 자동 실행되는 것은 아닙니다.", ShortToolTip="생산 재료와 소모 시점 정의입니다."))
struct ISEKAICLASS_API FICProductionMaterialRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material", meta=(ToolTip="재료 묶음 또는 드롭 표시 아이템입니다. Production에서는 Item ID/수량, DropActor에서는 실제 인스턴스 표시 값이며 원본은 WorldState입니다.\n입력 자료형: 구조/객체 (FICItemAmount).\n소속 정의/시스템: 현재 레시피 실행기는 Recipe.Ingredients를 성공 시 소모할 재료로 예약하므로 Production.Materials의 모든 소모 방식이 자동 실행되는 것은 아닙니다."))
    FICItemAmount Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material", meta=(ToolTip="재료 소모 시점 정의입니다. 성공 시 소모와 시작 시 소모를 구분합니다. 현재 레시피 실행은 Ingredients를 성공 시 소모로 예약하며 이 생산 필드를 전부 자동 실행하지 않습니다.\n입력 자료형: 선택 목록 (EICProductionMaterialTiming).\nC++ 초기값: ConsumeOnSuccess — 성공 완료 거래에서 재료를 소모하는 규칙입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 현재 레시피 실행기는 Recipe.Ingredients를 성공 시 소모할 재료로 예약하므로 Production.Materials의 모든 소모 방식이 자동 실행되는 것은 아닙니다."))
    EICProductionMaterialTiming ConsumptionTiming = EICProductionMaterialTiming::ConsumeOnSuccess;
};
