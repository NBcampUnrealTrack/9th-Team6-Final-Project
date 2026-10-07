#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCropRow.generated.h"

USTRUCT(BlueprintType, meta=(ToolTip="작물의 한 성장 단계입니다. 시간은 GameClock 분 기준이며 수확 가능 여부와 외형을 정의합니다. 실제 작물 성장 실행은 아직 연결 대상입니다.", ShortToolTip="작물의 한 성장 단계입니다."))
struct ISEKAICLASS_API FICCropStageDefinition
{
    GENERATED_BODY()

    /** Unique inside the owning crop; this is not a global definition ID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="같은 작물/진행 그래프 안의 고유 로컬 단계 키입니다. 전역 DT ID와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 작물 성장 실행은 아직 연결 대상입니다."))
    FName StageID;

    /** Sequential stage duration. The final harvestable stage has duration zero and never expires. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ClampMin="0.0", ToolTip="작물 단계 성장 GameClock 분입니다. 수면 시간 점프와 경과 반영을 실제 농사 실행기에 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 0.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n소속 정의/시스템: 실제 작물 성장 실행은 아직 연결 대상입니다."))
    double DurationGameMinutes = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage", meta=(ToolTip="이 작물 단계에서 수확 허용 정의입니다. 실제 수확/영수증 판정 연결은 별도입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 작물 성장 실행은 아직 연결 대상입니다."))
    bool bHarvestable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="메시 에셋 또는 표시 컴포넌트입니다. 필드 타입에 맞게 연결하고 스켈레톤·크기·축을 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (UStaticMesh).\n소속 정의/시스템: 실제 작물 성장 실행은 아직 연결 대상입니다."))
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="성장 단계 VFX 소프트 경로입니다. 실제 성장 변화/표시 실행에 연결합니다.\n입력 자료형: FSoftObjectPath.\n소속 정의/시스템: 실제 작물 성장 실행은 아직 연결 대상입니다."))
    FSoftObjectPath StageVFX;
};

/** No watering, fertilizer, rot or abandonment failure. A mature crop remains until successful harvest. */
USTRUCT(BlueprintType, meta=(ToolTip="씨앗·심기/수확 생산·성장 단계·수확물·허용 경작 앵커 정의입니다. 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다.", ShortToolTip="씨앗·심기/수확 생산·성장 단계·수확물·허용 경작 앵커 정의입니다."))
struct ISEKAICLASS_API FICCropRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop", meta=(ToolTip="씨앗 Item ID입니다. 씨앗 아이템 CropID와 이 작물 ID를 서로 맞춥니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다."))
    FName SeedItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop", meta=(ToolTip="심기 Production ID입니다. 실제 심기 전용 실행 연결은 남아 있습니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다."))
    FName PlantingProductionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop", meta=(ToolTip="수확 Production ID입니다. 실제 수확 전용 실행 연결은 남아 있습니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다."))
    FName HarvestProductionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop", meta=(ToolTip="작물 단계 순서 목록입니다. 단계 로컬 ID·게임 분·수확 가능/외형을 작성하며 실제 성장 실행은 별도입니다.\n입력 자료형: 구조/객체 (FICCropStageDefinition) 배열.\n소속 정의/시스템: 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다."))
    TArray<FICCropStageDefinition> GrowthStages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop", meta=(ToolTip="수확 Item ID/수량 목록입니다. 실제 수확 비용/아이템/영수증 거래 연결이 필요합니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.\n소속 정의/시스템: 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다."))
    TArray<FICItemAmount> HarvestYields;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crop", meta=(ToolTip="작물 심기 허용 경작 Anchor ID 목록입니다. 실제 작물/경작지 상태는 WorldState입니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다."))
    TArray<FName> AllowedPlotAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="Texture2D 아이콘입니다. 실제 UI/표시 실행에서 이 참조를 사용하도록 연결합니다. 비면 전용 아이콘이 없습니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\n소속 정의/시스템: 심은 시각과 현재 상태는 WorldState의 CropState에 저장합니다. 실제 농사 실행은 아직 연결 대상입니다."))
    TSoftObjectPtr<UTexture2D> Icon;
};
