#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICAnchorRow.generated.h"

UENUM(BlueprintType, meta=(ToolTip="앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."))
enum class EICAnchorType : uint8
{
    PlayerStart UMETA(ToolTip="플레이어 시작 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Home UMETA(ToolTip="집의 기준 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Transit UMETA(ToolTip="일과 이동 경유 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Work UMETA(ToolTip="일과 작업 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Bed UMETA(ToolTip="수면 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Kitchen UMETA(ToolTip="요리 작업 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Crafting UMETA(ToolTip="제작 작업 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), FarmPlot UMETA(ToolTip="경작 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."),
    StorageInstallation UMETA(ToolTip="보관함 설치 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), WarpArrival UMETA(ToolTip="워프 도착 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Landmark UMETA(ToolTip="명소 상호작용 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), FishingPoint UMETA(ToolTip="낚시 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Resource UMETA(ToolTip="채집 자원 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), MonsterOrigin UMETA(ToolTip="몬스터의 원점/복귀 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요."), Quest UMETA(ToolTip="퀘스트 사건 위치입니다.\n앵커가 나타내는 월드 위치의 용도입니다. DT 좌표와 실제 레벨 배치 및 등록을 맞추세요.")
};

/** Stable level placement and interaction anchor, not a saved moving actor position. */
USTRUCT(BlueprintType, meta=(ToolTip="월드에서 안정적으로 참조할 위치의 종류·지역·집·소유 NPC·좌표·도착 공간·SmartObject 정의입니다. NPC 초기 위치와 생산 참조 검사는 사용하지만 모든 배치/도착 안전 검사 실행은 연결 대상입니다.", ShortToolTip="월드에서 안정적으로 참조할 위치의 종류·지역·집·소유 NPC·좌표·도착 공간·SmartObject 정의입니다."))
struct ISEKAICLASS_API FICAnchorRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICAnchorType).\nC++ 초기값: Transit — 일과 이동 경유 위치입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICAnchorType Type = EICAnchorType::Transit;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ToolTip="Home DT의 주거 ID입니다. 초기 주거 정의와 현재 소유권/설치 기록을 구분하세요.\n입력 자료형: 이름/ID.")) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ToolTip="초기 소유 NPC ID입니다. 현재 소유권은 WorldState로 유지하며 지역/집/앵커 정의를 맞춥니다.\n입력 자료형: 이름/ID.")) FName OwnerNPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ToolTip="이전 저장 데이터/레벨 미로딩 때만 쓰는 위치·회전 예비값입니다. 런타임 NPC 위치와 방향은 같은 ID를 가진 레벨 앵커 액터의 Transform을 사용하므로, 앵커를 옮긴 뒤 이 값을 다시 입력할 필요가 없습니다.\n입력 자료형: 위치·회전·배율.\nC++ 초기값: FTransform::Identity. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) FTransform WorldTransform = FTransform::Identity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ToolTip="실내 층 로컬 키입니다. 앵커/지도 층을 같은 키로 연결하고 지역/지도 정의와 맞춥니다.\n입력 자료형: 이름/ID.")) FName InteriorFloorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ToolTip="도착 안전 위치로 작성/검증했다는 정의입니다. 실제 캡슐 충돌 검사까지 자동 수행하는 체크가 아닙니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) bool bSafeForArrival = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ClampMin="0", Units="cm", ToolTip="도착 캡슐 반지름 cm입니다. 42이면 지름 84cm이며 실제 안전 공간 검사를 연결합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 42.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double ClearanceRadiusCm = 42.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anchor", meta=(ClampMin="0", Units="cm", ToolTip="도착 캡슐 반높이 cm입니다. 96이면 전체 높이 192cm이며 실제 도착 충돌/안전 검사 연결이 필요합니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 96.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) double ClearanceHalfHeightCm = 96.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SmartObject", meta=(AllowedClasses="/Script/SmartObjectsModule.SmartObjectDefinition", ToolTip="앵커 SmartObject 정의 소프트 참조입니다. 해당 슬롯/실제 AI 상호작용 실행을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UObject).\n허용 에셋 종류: /Script/SmartObjectsModule.SmartObjectDefinition.")) TSoftObjectPtr<UObject> SmartObjectDefinition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SmartObject", meta=(ClampMin="0", ToolTip="SmartObject 0부터 시작하는 슬롯 번호입니다. 정의에 존재하는 슬롯과 작업 위치를 맞춥니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.")) int32 SmartObjectSlotIndex = 0;
};
