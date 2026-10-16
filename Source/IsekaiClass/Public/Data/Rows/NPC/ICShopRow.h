#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICShopRow.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="상점에 표시할 아이템과 순서입니다. Item DT의 가격/판매 규칙과 함께 사용하도록 정의하며 현재 상점 구매·판매 거래는 별도 연결 대상입니다.", ShortToolTip="상점에 표시할 아이템과 순서입니다."))
struct ISEKAICLASS_API FICShopEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop", meta=(ToolTip="Item DT 정의 ID입니다. 예: ITM_CONS_001. 인스턴스 GUID·Equipment 정의 ID와 구분합니다. 지급/소모는 정의와 보관 규칙을 검사합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: Item DT의 가격/판매 규칙과 함께 사용하도록 정의하며 현재 상점 구매·판매 거래는 별도 연결 대상입니다.")) FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop", meta=(ToolTip="표시 순서 정수 키입니다. 실제 UI 정렬 연결을 확인하고 배열 순서/ID와 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: Item DT의 가격/판매 규칙과 함께 사용하도록 정의하며 현재 상점 구매·판매 거래는 별도 연결 대상입니다.")) int32 DisplayOrder = 0;
};

/** A fixed, unlimited stock list. Item definitions own buy/sell prices; the connected service owns affinity discounts. */
USTRUCT(BlueprintType, meta=(ToolTip="서비스가 제공하는 상점의 판매 아이템 목록입니다. ServiceID와 Service.ShopID가 서로 연결되어야 합니다. 실제 상점 거래 실행은 아직 연결 대상입니다.", ShortToolTip="서비스가 제공하는 상점의 판매 아이템 목록입니다."))
struct ISEKAICLASS_API FICShopRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop", meta=(ToolTip="Service DT ID입니다. 상점/워프 등 종류와 연결 참조를 맞추세요.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 상점 거래 실행은 아직 연결 대상입니다.")) FName ServiceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop", meta=(ToolTip="상점/드롭 항목 목록입니다. 상점은 Item/표시 순서, 드롭은 후보 수량/확률/가중치이며 상위 타입을 확인합니다.\n입력 자료형: 구조/객체 (FICShopEntry) 배열.\n소속 정의/시스템: 실제 상점 거래 실행은 아직 연결 대상입니다.")) TArray<FICShopEntry> Entries;
};
