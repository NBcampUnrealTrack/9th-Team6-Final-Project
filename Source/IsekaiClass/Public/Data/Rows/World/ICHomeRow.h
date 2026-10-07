#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICHomeRow.generated.h"

UENUM(BlueprintType, meta=(ToolTip="집 정의의 종류입니다. 실제 소유권은 WorldState가 원본입니다."))
enum class EICHomeType : uint8 { Player UMETA(ToolTip="플레이어 집입니다.\n집 정의의 종류입니다. 실제 소유권은 WorldState가 원본입니다."), NPC UMETA(ToolTip="NPC 집입니다.\n집 정의의 종류입니다. 실제 소유권은 WorldState가 원본입니다."), Inn UMETA(ToolTip="여관입니다.\n집 정의의 종류입니다. 실제 소유권은 WorldState가 원본입니다.") };

UENUM(BlueprintType, meta=(ToolTip="집 안의 보관 시설 분류입니다. 실제 내용물은 ContainerID로 관리합니다."))
enum class EICHomeContainerType : uint8 { Storage UMETA(ToolTip="일반 보관함입니다.\n집 안의 보관 시설 분류입니다. 실제 내용물은 ContainerID로 관리합니다."), Wardrobe UMETA(ToolTip="옷장 분류입니다.\n집 안의 보관 시설 분류입니다. 실제 내용물은 ContainerID로 관리합니다."), Bookshelf UMETA(ToolTip="책장 분류입니다.\n집 안의 보관 시설 분류입니다. 실제 내용물은 ContainerID로 관리합니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="집 내부 컨테이너의 로컬 ID·종류·아이템·접근 관계 조건입니다. 소유권과 변경된 내용물은 WorldState의 인벤토리에 저장합니다. 실제 집 접근 판정 연결은 별도입니다.", ShortToolTip="집 내부 컨테이너의 로컬 ID·종류·아이템·접근 관계 조건입니다."))
struct ISEKAICLASS_API FICHomeContainerDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ToolTip="컨테이너의 안정적인 저장 키입니다. 집/공유 그룹/소유자와 함께 설정하고 여러 물체가 같은 내용물을 공유할 때만 같은 키를 사용합니다.\n입력 자료형: 이름/ID.")) FName ContainerID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICHomeContainerType).\nC++ 초기값: Storage — 일반 보관함입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICHomeContainerType Type = EICHomeContainerType::Storage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ToolTip="Anchor DT의 위치 ID입니다. 좌표·지역·종류를 참조하며 액터 표시 이름을 넣지 않습니다.\n입력 자료형: 이름/ID.")) FName AnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ClampMin="1", ToolTip="컨테이너 슬롯 용량입니다. 양수이며 초기품 스택/내용물 수와 맞춥니다.\n입력 자료형: 정수.\nC++ 초기값: 1. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.")) int32 SlotCapacity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ToolTip="NPC 컨테이너 최초 Item ID/수량입니다. 이후 변경 내용은 WorldState가 원본이며 초기 목록을 매번 재지급하지 않습니다.\n입력 자료형: 구조/객체 (FICItemAmount) 배열.")) TArray<FICItemAmount> InitialItems;
    // Initial items are seeded once. Removed contents and equipment instances are WorldState, never regenerated here.
};

USTRUCT(BlueprintType, meta=(ToolTip="주거의 소유자·출입/수면/작업 위치·컨테이너·보관함 설치·경작 공간 정의입니다. 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.", ShortToolTip="주거의 소유자·출입/수면/작업 위치·컨테이너·보관함 설치·경작 공간 정의입니다."))
struct ISEKAICLASS_API FICHomeRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICHomeType).\nC++ 초기값: Player — 플레이어 집입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) EICHomeType Type = EICHomeType::Player;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="Region DT의 소속 지역 ID입니다. 레벨 파일 이름과 구분합니다. 필요한 행에서 None이면 지역 누락 검사가 안내합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="초기 소유 NPC ID입니다. 현재 소유권은 WorldState로 유지하며 지역/집/앵커 정의를 맞춥니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) FName OwnerNPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="집 입구 Anchor ID입니다. 지역/도착 공간과 실제 출입 실행을 확인합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) FName EntranceAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="수면 위치 Anchor ID 목록입니다. 실제 주거/여관/수면 대상 판정을 연결해야 합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) TArray<FName> BedAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="주거 조리대 Anchor ID 목록입니다. 실제 생산 허용 위치와 배치를 맞춥니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) TArray<FName> KitchenAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="집의 제작대 Anchor ID 목록입니다. 실제 작업 위치와 생산 허용 목록을 맞춥니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) TArray<FName> CraftingAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home", meta=(ToolTip="여관 Service ID입니다. 실제 비용/영업/수면 거래 연결이 필요합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) FName InnServiceID;
    // NPC container permission uses current affinity; bed use is a separate occupancy/service check.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPCContainers", meta=(ClampMin="0", ClampMax="100", ToolTip="NPC 집 컨테이너 접근 관계 기준입니다. 0~100이며 실제 소유자/관계/접근 판정 연결이 필요합니다.\n입력 자료형: 정수.\nC++ 초기값: 100. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 100.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) int32 ContainerAccessAffinity = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPCContainers", meta=(ToolTip="NPC 집 컨테이너 정의 목록입니다. 로컬 키·종류·초기품·관계 조건을 작성하고 실제 소유/접근 실행과 연결합니다.\n입력 자료형: 구조/객체 (FICHomeContainerDefinition) 배열.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) TArray<FICHomeContainerDefinition> NPCContainers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(EditCondition="Type == EICHomeType::Player", ToolTip="집의 보관함 설치 고정 Anchor ID 목록입니다. 최대 설치 수/중복 원본 검사와 연결합니다.\n입력 자료형: 이름/ID 배열.\n편집 활성 조건: Type == EICHomeType::Player.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) TArray<FName> StorageInstallAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(EditCondition="Type == EICHomeType::Player", ToolTip="주거 경작지 Anchor ID 목록입니다. Crop.AllowedPlotAnchorIDs와 실제 농사 배치를 맞춥니다.\n입력 자료형: 이름/ID 배열.\n편집 활성 조건: Type == EICHomeType::Player.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) TArray<FName> FarmPlotAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(ClampMin="1", ToolTip="집 보관함 하나당 슬롯 용량 규격입니다. 현재 40칸이며 실제 설치 생성/저장 검사와 일치시킵니다.\n입력 자료형: 정수.\nC++ 초기값: 40. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) int32 StorageSlotsPerInstallation = 40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(ClampMin="1", ClampMax="4", ToolTip="최대 설치 보관함 규격입니다. 현재 검사도 상한 4를 사용하며 실제 설치 실행은 별도입니다.\n입력 자료형: 정수.\nC++ 초기값: 4. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 4.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) int32 MaximumStorageInstallations = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(ToolTip="여러 주거/컨테이너가 같은 보관함 원본을 공유할 그룹 키입니다. 독립 보관함에는 같은 키를 임의 복사하지 않습니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 런타임 소유권/설치 기록은 WorldState가 원본이며 주거 실행 연결은 아직 남아 있습니다.")) FName SharedStorageGroupID;
};
