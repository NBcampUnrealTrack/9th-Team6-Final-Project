#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICRegionRow.generated.h"

class USoundBase;
class UWorld;

USTRUCT(BlueprintType, meta=(ToolTip="지역 레벨·경계·기본 위치·앵커/스포너/NPC·보스·지도·BGM 정의입니다. 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.", ShortToolTip="지역 레벨·경계·기본 위치·앵커/스포너/NPC·보스·지도·BGM 정의입니다."))
struct ISEKAICLASS_API FICRegionRow : public FICDefinitionRow
{
    GENERATED_BODY()

    // R01-R08 share the same World Partition world. A region row does not introduce a travel lock.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역 UWorld 레벨 소프트 참조입니다. 실제 스트리밍/레벨 전환과 배치 실행을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UWorld).\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TSoftObjectPtr<UWorld> World;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(Units="cm", ToolTip="지역의 월드 축 정렬 경계 박스입니다. cm 좌표이며 실제 레벨/앵커 위치를 포함하게 작성합니다.\n입력 자료형: FBox.\nC++ 초기값: FBox(ForceInit). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) FBox BoundsCm = FBox(ForceInit);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역 평면 경계 꼭짓점 배열입니다. cm 좌표이며 순서/자기 교차/지역 경계 검사 규칙을 확인합니다.\n입력 자료형: 2차원 좌표/크기 배열.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TArray<FVector2D> BoundaryPolygonCm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역의 기본 도착/대표 위치 Anchor ID입니다. 해당 지역에 속한 유효한 앵커를 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) FName DefaultAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역의 Anchor ID 목록입니다. 각 앵커 RegionID와 상위 지역을 맞추세요.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TArray<FName> AnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역의 Spawner DT ID 목록입니다. 슬롯 키/셀 상태의 실제 스폰 제어 연결은 별도입니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TArray<FName> SpawnerIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역의 Warp DT ID 목록입니다. 실제 발견/해금은 WorldState에 기록합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TArray<FName> WarpIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="NPC DT의 정의 ID 목록입니다. 모든 참조가 존재해야 하며 중복을 피합니다. 액터 스폰 여부와 정적 정의 존재는 별개입니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TArray<FName> NPCIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역 보스 Monster ID입니다. 소속 지역·보스 종류·진행 조건을 맞춥니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) FName RegionBossMonsterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="Landmark DT ID입니다. 지역·앵커·도감 참조를 맞춥니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) FName LandmarkID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ToolTip="지역 지도 Map ID입니다. 이미지·좌표 범위·층을 함께 작성합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) FName TownMapID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region", meta=(ClampMin="0", ToolTip="지역 난이도/진행 정렬용 정수입니다. 실제 진행 해금과 별개이며 비교/정렬 실행 연결이 필요합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) int32 DifficultyOrder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ToolTip="지역 탐험 배경음입니다. 지역 진입/전투 종료 전환에 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TSoftObjectPtr<USoundBase> ExplorationBGM;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ToolTip="지역 전투 배경음입니다. 실제 전투 상태 변화와 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (USoundBase).\n소속 정의/시스템: 조회/참조 검사는 지원하지만 지역 스트리밍/실제 배치/음악 전환 실행은 별도 연결 대상입니다.")) TSoftObjectPtr<USoundBase> CombatBGM;
};
