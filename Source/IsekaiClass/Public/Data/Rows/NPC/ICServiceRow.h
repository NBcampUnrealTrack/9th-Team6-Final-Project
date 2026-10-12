#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICServiceRow.generated.h"

UENUM(BlueprintType, meta=(ToolTip="서비스 실행 분류입니다. 종류에 맞는 Shop/Warp/Home 정의와 서비스 거래를 연결해야 합니다."))
enum class EICServiceType : uint8 { Shop UMETA(ToolTip="아이템 구매·판매 상점입니다.\n서비스 실행 분류입니다. 종류에 맞는 Shop/Warp/Home 정의와 서비스 거래를 연결해야 합니다."), Enhancement UMETA(ToolTip="장비 강화 서비스입니다.\n서비스 실행 분류입니다. 종류에 맞는 Shop/Warp/Home 정의와 서비스 거래를 연결해야 합니다."), Warp UMETA(ToolTip="워프 이동 서비스입니다.\n서비스 실행 분류입니다. 종류에 맞는 Shop/Warp/Home 정의와 서비스 거래를 연결해야 합니다."), Inn UMETA(ToolTip="여관 수면 서비스입니다.\n서비스 실행 분류입니다. 종류에 맞는 Shop/Warp/Home 정의와 서비스 거래를 연결해야 합니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="서비스의 하루 영업 시간 구간입니다. 하루 기준 GameClock 분으로 작성합니다. 현재 서비스 실행 및 영업 판정은 별도 연결 대상입니다.", ShortToolTip="서비스의 하루 영업 시간 구간입니다."))
struct ISEKAICLASS_API FICServiceHours
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hours", meta=(ClampMin="0", ClampMax="1439", Units="min", ToolTip="서비스 여는 하루 분입니다. 360=06:00이며 닫는 시간/자정 경계를 함께 작성합니다.\n입력 자료형: 정수.\nC++ 초기값: 360. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1439.\n소속 정의/시스템: 현재 서비스 실행 및 영업 판정은 별도 연결 대상입니다.")) int32 OpenGameMinute = 360;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hours", meta=(ClampMin="1", ClampMax="1440", Units="min", ToolTip="서비스 닫는 하루 GameClock 분입니다. 1200=20:00이며 0~1440 구간 규칙/개장 시각과 함께 작성합니다.\n입력 자료형: 정수.\nC++ 초기값: 1200. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 1440.\n소속 정의/시스템: 현재 서비스 실행 및 영업 판정은 별도 연결 대상입니다.")) int32 CloseGameMinute = 1200;
};

/** Highest satisfied MinimumAffinity tier supplies the discount. Current affinity is looked up at transaction time. */
USTRUCT(BlueprintType, meta=(ToolTip="NPC 관계가 기준 이상일 때 적용할 할인 비율입니다. DiscountRatio 0.10은 10% 할인입니다. 실제 서비스 가격 거래는 아직 연결 대상입니다.", ShortToolTip="NPC 관계가 기준 이상일 때 적용할 할인 비율입니다."))
struct ISEKAICLASS_API FICAffinityDiscountTier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price", meta=(ClampMin="0", ClampMax="100", ToolTip="할인 적용 관계 하한입니다. 현재 관계가 이 값 이상인 경우의 규칙이며 실제 가격 계산 연결이 필요합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 100.\n소속 정의/시스템: 실제 서비스 가격 거래는 아직 연결 대상입니다.")) int32 MinimumAffinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price", meta=(ClampMin="0", ClampMax="1", ToolTip="가격 할인 비율입니다. 0.10은 10% 할인, 1은 100% 할인입니다. 실제 반올림/관계 가격 계산 연결이 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1.\n소속 정의/시스템: 실제 서비스 가격 거래는 아직 연결 대상입니다.")) double DiscountRatio = 0.0;
};

/** Both service hours and scheduled work at WorkAnchorID must match. A moving NPC's location is not the map marker. */
USTRUCT(BlueprintType, meta=(ToolTip="상점·강화·워프·여관 서비스의 NPC·작업 위치·영업·가격·조건 정의입니다. 종류에 맞는 Shop/Warp/Home 참조를 지정하세요. 일반 서비스 실행 연결은 남아 있습니다.", ShortToolTip="상점·강화·워프·여관 서비스의 NPC·작업 위치·영업·가격·조건 정의입니다."))
struct ISEKAICLASS_API FICServiceRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICServiceType).\nC++ 초기값: Shop — 아이템 구매·판매 상점입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) EICServiceType Type = EICServiceType::Shop;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="NPC DT의 정의 ID입니다. R08 지역 규격의 같은 RowName/ID를 지정하고 액터 이름·인스턴스 GUID와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="작업/서비스 Anchor ID입니다. 생산 시작은 정의 존재와 Production.WorkAnchorIDs 허용 목록을 검사합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FName WorkAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="영업 시간 구간 목록입니다. 겹침/0~1440/자정 분할과 실제 서비스 판정 실행을 확인합니다.\n입력 자료형: 구조/객체 (FICServiceHours) 배열.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) TArray<FICServiceHours> OpeningHours;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="Shop DT ID입니다. Shop.ServiceID와 서비스 ShopID를 서로 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FName ShopID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="Warp DT ID입니다. Warp.ServiceID와 서비스 종류를 맞춥니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FName WarpID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="Home DT의 주거 ID입니다. 초기 주거 정의와 현재 소유권/설치 기록을 구분하세요.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service", meta=(ToolTip="대사/서비스/스폰 등 이용 조건입니다. 참조 검사는 제공되지만 모든 런타임 조건 평가가 구현된 것은 아닙니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FICConditionGroup Availability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price", meta=(ClampMin="0", ToolTip="서비스 기본 정수 골드 비용입니다. 실제 할인/예약/확정 거래는 연결 대상입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) int32 BaseCostGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price", meta=(ToolTip="관계 임계값별 할인 목록입니다. 실제 현재 관계/가격 계산과 거래 실행은 연결 대상입니다.\n입력 자료형: 구조/객체 (FICAffinityDiscountTier) 배열.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) TArray<FICAffinityDiscountTier> AffinityDiscounts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="아이콘 ResourceReference ID입니다. 실제 파일 참조와 UI 표시를 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 일반 서비스 실행 연결은 남아 있습니다.")) FName IconResourceID;
};
