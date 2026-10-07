#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICResourceRow.generated.h"

/** Resource-type definition. Per-placement depletion/regeneration timestamps are saved separately. */
USTRUCT(BlueprintType, meta=(ToolTip="벌목·채광·채집 자원의 배치·생산·획득물·재생 시간·외형 정의입니다. 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다.", ShortToolTip="벌목·채광·채집 자원의 배치·생산·획득물·재생 시간·외형 정의입니다."))
struct ISEKAICLASS_API FICResourceRow : public FICDefinitionRow
{
    GENERATED_BODY()

    /** Only Woodcutting, Mining and Gathering are valid resource activities. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="생활 생산/일과 활동 분류입니다. 상위 필드 타입에 맞춰 제작·요리·심기·수확·낚시·채집 또는 대기·이동·작업·수면을 선택합니다.\n입력 자료형: 선택 목록 (EICLifeActivity).\nC++ 초기값: Gathering — 채집 활동입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    EICLifeActivity Activity = EICLifeActivity::Gathering;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    FName RegionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="Production DT ID입니다. 현재 레시피 실행은 제작 시간/SP/결과 종류를 연결 규칙과 비교합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    FName ProductionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="자원 배치 Anchor ID 목록입니다. 각 지점과 ResourcePoint 상태를 연결하는 실행기가 필요합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    TArray<FName> PlacementAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ToolTip="자원 획득 Item ID/수량 목록입니다. 실제 채집 결과/재생/비용 거래 연결은 별도입니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    TArray<FICItemAmount> Yields;

    /** Starts only after the complete yield was successfully granted. Sleep does not advance it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ClampMin="0.0", Units="s", ToolTip="자원 고갈 후 재생 PlayClock 초입니다. 메뉴 중 진행되지 않도록 ResourcePoint 원본과 실제 채집 실행을 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 300.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.0.\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    double RegenerationPlaySeconds = 300.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="자원 표시 AActor 자식 BP입니다. 지속 고갈/재생 원본과 표시를 구분합니다.\n입력 자료형: Soft Class 에셋 참조 (AActor).\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    TSoftClassPtr<AActor> ResourceActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="고갈되지 않은 자원 지점 메시입니다. 실제 자원 원본 상태와 외형 전환을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UStaticMesh).\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    TSoftObjectPtr<UStaticMesh> AvailableMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="고갈된 자원 표시 메시입니다. 실제 ResourcePoint 상태 변화에 외형 전환을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UStaticMesh).\n소속 정의/시스템: 고갈/재생 원본은 WorldState이며 현재 자원 채집 실행은 별도 연결 대상입니다."))
    TSoftObjectPtr<UStaticMesh> DepletedMesh;
};
