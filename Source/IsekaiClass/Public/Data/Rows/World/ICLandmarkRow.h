#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICLandmarkRow.generated.h"

class UTexture2D;

/** One prepared photo per region, registered by interaction. No completion reward or free camera state. */
USTRUCT(BlueprintType, meta=(ToolTip="명소의 지역·위치 앵커·도감 항목·사진 정의입니다. 명소 발견과 도감 등록 실행은 별도 연결 대상입니다.", ShortToolTip="명소의 지역·위치 앵커·도감 항목·사진 정의입니다."))
struct ISEKAICLASS_API FICLandmarkRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 명소 발견과 도감 등록 실행은 별도 연결 대상입니다.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark", meta=(ToolTip="Anchor DT의 위치 ID입니다. 좌표·지역·종류를 참조하며 액터 표시 이름을 넣지 않습니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 명소 발견과 도감 등록 실행은 별도 연결 대상입니다.")) FName AnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark", meta=(ToolTip="Codex DT ID입니다. 실제 획득/대화/발견 이벤트와 등록 규칙을 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 명소 발견과 도감 등록 실행은 별도 연결 대상입니다.")) FName CodexID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark", meta=(ToolTip="명소 Texture2D 사진입니다. 도감/명소 UI에서 사용하는 참조입니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\n소속 정의/시스템: 명소 발견과 도감 등록 실행은 별도 연결 대상입니다.")) TSoftObjectPtr<UTexture2D> Photo;
};
