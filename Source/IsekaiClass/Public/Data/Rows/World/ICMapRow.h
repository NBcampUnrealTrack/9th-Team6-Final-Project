#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICMapRow.generated.h"

class UTexture2D;

UENUM(BlueprintType, meta=(ToolTip="지도의 화면 용도입니다. 지도 이미지·좌표 변환·마커 표시의 실행 연결이 필요합니다."))
enum class EICMapType : uint8 { Large UMETA(ToolTip="큰 지도 화면입니다.\n지도의 화면 용도입니다. 지도 이미지·좌표 변환·마커 표시의 실행 연결이 필요합니다."), Town UMETA(ToolTip="마을 지도 화면입니다.\n지도의 화면 용도입니다. 지도 이미지·좌표 변환·마커 표시의 실행 연결이 필요합니다."), Minimap UMETA(ToolTip="플레이 화면의 미니맵입니다.\n지도의 화면 용도입니다. 지도 이미지·좌표 변환·마커 표시의 실행 연결이 필요합니다.") };

UENUM(BlueprintType, meta=(ToolTip="지도 마커의 대상 분류입니다. 표시 여부와 위치는 현재 WorldState/액터에서 조회해야 합니다."))
enum class EICMapMarkerType : uint8 { Player UMETA(ToolTip="플레이어 마커입니다.\n지도 마커의 대상 분류입니다. 표시 여부와 위치는 현재 WorldState/액터에서 조회해야 합니다."), Companion UMETA(ToolTip="동료 마커입니다.\n지도 마커의 대상 분류입니다. 표시 여부와 위치는 현재 WorldState/액터에서 조회해야 합니다."), Enemy UMETA(ToolTip="적 마커입니다.\n지도 마커의 대상 분류입니다. 표시 여부와 위치는 현재 WorldState/액터에서 조회해야 합니다."), Shop UMETA(ToolTip="상점 마커입니다.\n지도 마커의 대상 분류입니다. 표시 여부와 위치는 현재 WorldState/액터에서 조회해야 합니다."), Warp UMETA(ToolTip="워프 마커입니다.\n지도 마커의 대상 분류입니다. 표시 여부와 위치는 현재 WorldState/액터에서 조회해야 합니다."), Quest UMETA(ToolTip="퀘스트 마커입니다.\n지도 마커의 대상 분류입니다. 표시 여부와 위치는 현재 WorldState/액터에서 조회해야 합니다.") };

/** All floors share the same horizontal discovery record. Axis vectors are orthonormal world XY directions. */
USTRUCT(BlueprintType, meta=(ToolTip="지도 한 층의 층 ID·이름 키·이미지와 월드 좌표 대응 영역입니다. 실제 지도 좌표 변환/UI 표시 연결은 별도입니다.", ShortToolTip="지도 한 층의 층 ID·이름 키·이미지와 월드 좌표 대응 영역입니다."))
struct ISEKAICLASS_API FICMapLayerDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layer", meta=(ToolTip="실내 층 로컬 키입니다. 앵커/지도 층을 같은 키로 연결하고 지역/지도 정의와 맞춥니다.\n입력 자료형: 이름/ID.")) FName InteriorFloorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layer", meta=(ToolTip="지도 층 표시 이름 String Table 키입니다. 실제 지도 UI에서 해석해야 합니다.\n입력 자료형: 문자열.")) FString DisplayNameKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layer", meta=(ToolTip="지도 Texture2D입니다. 월드 경계·축·UV 변환과 함께 설정합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).")) TSoftObjectPtr<UTexture2D> MapTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm", ToolTip="지도 기준 월드 평면 cm 원점입니다. MapOriginUV와 같은 위치를 나타냅니다.\n입력 자료형: 2차원 좌표/크기.\nC++ 초기값: FVector2D::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector2D WorldOriginCm = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(ToolTip="지도 U 방향에 해당하는 월드 평면 축입니다. V축/UV 원점·스케일과 실제 좌표 변환을 맞춥니다.\n입력 자료형: 2차원 좌표/크기.\nC++ 초기값: FVector2D(1.0, 0.0). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector2D WorldAxisU = FVector2D(1.0, 0.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(ToolTip="지도 V 방향에 해당하는 월드 평면 축입니다. U축과 독립인 방향을 사용해 뒤집힘/회전을 맞춥니다.\n입력 자료형: 2차원 좌표/크기.\nC++ 초기값: FVector2D(0.0, 1.0). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector2D WorldAxisV = FVector2D(0.0, 1.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(ToolTip="WorldOriginCm에 대응하는 지도 UV 좌표입니다. 0~1 이미지 좌표 규칙과 실제 변환 실행을 맞춥니다.\n입력 자료형: 2차원 좌표/크기.\nC++ 초기값: FVector2D::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector2D MapOriginUV = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm", ToolTip="UV 1 증가에 대응하는 월드 cm입니다. 100000=1km이며 실제 지도 축/변환 실행과 맞춥니다.\n입력 자료형: 2차원 좌표/크기.\nC++ 초기값: FVector2D(100000.0, 100000.0). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector2D WorldCmPerUV = FVector2D(100000.0, 100000.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm", ToolTip="지도 적용 영역 평면 최소 월드 cm 좌표입니다. 이미지/최대 좌표와 맞춥니다.\n입력 자료형: 2차원 좌표/크기.\nC++ 초기값: FVector2D::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector2D WorldBoundsMinCm = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection", meta=(Units="cm", ToolTip="지도 적용 영역 평면 최대 월드 cm 좌표입니다. 최소 좌표보다 큰 유효 범위를 작성합니다.\n입력 자료형: 2차원 좌표/크기.\nC++ 초기값: FVector2D::ZeroVector. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FVector2D WorldBoundsMaxCm = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType, meta=(ToolTip="지도 종류·소속 지역·층별 이미지·마커·확대 범위의 정적 정의입니다. 발견 상태는 WorldState, 이미지 표시와 좌표 변환은 지도 실행/UI 측 책임입니다.", ShortToolTip="지도 종류·소속 지역·층별 이미지·마커·확대 범위의 정적 정의입니다."))
struct ISEKAICLASS_API FICMapRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICMapType).\nC++ 초기값: Large — 큰 지도 화면입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICMapType Type = EICMapType::Large;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ToolTip="지도 층 목록입니다. 층 키·텍스처·월드 좌표 변환을 작성하고 실제 지도 UI에 연결합니다.\n입력 자료형: 구조/객체 (FICMapLayerDefinition) 배열.")) TArray<FICMapLayerDefinition> Layers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ToolTip="지도에 표시할 마커 종류 목록입니다. 실제 현재 위치/발견/퀘스트 UI 연결이 필요합니다.\n입력 자료형: 선택 목록 (EICMapMarkerType) 배열.")) TArray<EICMapMarkerType> VisibleMarkers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ClampMin="0.01", ToolTip="지도 최소 확대 배율입니다. 양수/MaximumZoom 이하이며 실제 UI 제한과 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 1.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.01.")) double MinimumZoom = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ClampMin="0.01", ToolTip="지도 최대 확대 배율입니다. MinimumZoom 이상이며 실제 지도 UI에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 4.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.01.")) double MaximumZoom = 4.0;
    // Large: Warp/Quest; Town: Player/Shop/Warp/Quest; Minimap: all six. Every location marker requires discovery.
    // Shop markers use service anchors; quest NPC markers use the live actor or current schedule anchor.
};
