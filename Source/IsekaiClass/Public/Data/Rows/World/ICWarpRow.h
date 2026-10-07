#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICWarpRow.generated.h"

/** Visit unlock and both endpoint services are runtime checks; gold prices/discounts belong to the service definition. */
USTRUCT(BlueprintType, meta=(ToolTip="워프 서비스·발견 앵커·도착 앵커·발견 거리·이용 조건 정의입니다. 활성화된 워프 기록은 WorldState에 저장하고 실제 이동/비용 거래는 별도 실행 경로에서 처리합니다.", ShortToolTip="워프 서비스·발견 앵커·도착 앵커·발견 거리·이용 조건 정의입니다."))
struct ISEKAICLASS_API FICWarpRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp", meta=(ToolTip="Service DT ID입니다. 상점/워프 등 종류와 연결 참조를 맞추세요.\n입력 자료형: 이름/ID.")) FName ServiceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp", meta=(ToolTip="방문/발견 판정 중심 Anchor ID입니다. 도착 앵커와 별개이며 실제 발견 판정 연결이 필요합니다.\n입력 자료형: 이름/ID.")) FName VisitAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp", meta=(ToolTip="워프 후 도착 Anchor ID입니다. 안전 공간과 지역을 확인합니다. 실제 워프 이동/비용 실행은 별도 연결 대상입니다.\n입력 자료형: 이름/ID.")) FName ArrivalAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp", meta=(ClampMin="0", Units="cm", ToolTip="워프 발견 중심 반지름 cm입니다. 250=2.5m이며 실제 발견/해금 실행은 별도입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 250.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double VisitRadiusCm = 250.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warp", meta=(ToolTip="대사/서비스/스폰 등 이용 조건입니다. 참조 검사는 제공되지만 모든 런타임 조건 평가가 구현된 것은 아닙니다.\n입력 자료형: 구조/객체 (FICConditionGroup).")) FICConditionGroup Availability;
};
