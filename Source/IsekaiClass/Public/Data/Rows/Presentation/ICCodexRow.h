#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCodexRow.generated.h"

class UTexture2D;

UENUM(BlueprintType, meta=(ToolTip="도감 대상의 종류입니다. Target 정의 종류와 일치시켜야 합니다."))
enum class EICCodexCategory : uint8 { Item UMETA(ToolTip="아이템 도감입니다.\n도감 대상의 종류입니다. Target 정의 종류와 일치시켜야 합니다."), NPC UMETA(ToolTip="NPC 도감입니다.\n도감 대상의 종류입니다. Target 정의 종류와 일치시켜야 합니다."), Landmark UMETA(ToolTip="명소 도감입니다.\n도감 대상의 종류입니다. Target 정의 종류와 일치시켜야 합니다.") };

UENUM(BlueprintType, meta=(ToolTip="도감 최초 등록의 사건입니다. 발생 이벤트와 WorldState 등록 원본 연결이 필요합니다."))
enum class EICCodexRegistration : uint8 { FirstItemInspectionOrAcquisition UMETA(ToolTip="아이템을 처음 조사하거나 획득할 때 등록하는 규칙입니다.\n도감 최초 등록의 사건입니다. 발생 이벤트와 WorldState 등록 원본 연결이 필요합니다."), FirstNPCConversation UMETA(ToolTip="NPC와 처음 대화할 때 등록하는 규칙입니다.\n도감 최초 등록의 사건입니다. 발생 이벤트와 WorldState 등록 원본 연결이 필요합니다."), LandmarkInteraction UMETA(ToolTip="명소와 상호작용할 때 등록하는 규칙입니다.\n도감 최초 등록의 사건입니다. 발생 이벤트와 WorldState 등록 원본 연결이 필요합니다.") };

/** Definition and display only. Discovery, actual acquisition and affinity history are separate saved records. */
USTRUCT(BlueprintType, meta=(ToolTip="아이템·NPC·명소 도감 항목의 대상·등록 조건·순서·이미지 정의입니다. 이미 등록한 상태는 WorldState에 보존하며 도감 실행/UI는 별도 연결 대상입니다.", ShortToolTip="아이템·NPC·명소 도감 항목의 대상·등록 조건·순서·이미지 정의입니다."))
struct ISEKAICLASS_API FICCodexRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex", meta=(ToolTip="목록/도감/아이템 분류입니다. 실행 종류 Kind와 다르며 분류만 바꿔 효과가 생성되지 않습니다.\n입력 자료형: 선택 목록 (EICCodexCategory).\nC++ 초기값: Item — 아이템 도감입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 이미 등록한 상태는 WorldState에 보존하며 도감 실행/UI는 별도 연결 대상입니다.")) EICCodexCategory Category = EICCodexCategory::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex", meta=(ToolTip="도감 대상 정의 종류/ID입니다. Category에 맞는 Item/NPC/Landmark 참조를 지정합니다.\n입력 자료형: 구조/객체 (FICDefinitionRef).\n소속 정의/시스템: 이미 등록한 상태는 WorldState에 보존하며 도감 실행/UI는 별도 연결 대상입니다.")) FICDefinitionRef Subject;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex", meta=(ToolTip="도감 등록 사건 종류입니다. 첫 아이템 확인/획득·첫 NPC 대화·명소 상호작용과 실제 이벤트를 연결합니다.\n입력 자료형: 선택 목록 (EICCodexRegistration).\nC++ 초기값: FirstItemInspectionOrAcquisition — 아이템을 처음 조사하거나 획득할 때 등록하는 규칙입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 이미 등록한 상태는 WorldState에 보존하며 도감 실행/UI는 별도 연결 대상입니다.")) EICCodexRegistration Registration = EICCodexRegistration::FirstItemInspectionOrAcquisition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex", meta=(ToolTip="표시 순서 정수 키입니다. 실제 UI 정렬 연결을 확인하고 배열 순서/ID와 구분합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 이미 등록한 상태는 WorldState에 보존하며 도감 실행/UI는 별도 연결 대상입니다.")) int32 DisplayOrder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex", meta=(ToolTip="표시 Texture2D 이미지입니다. 실제 표시 비율/좌표와 파일 참조를 확인합니다.\n입력 자료형: Soft Object 에셋 참조 (UTexture2D).\n소속 정의/시스템: 이미 등록한 상태는 WorldState에 보존하며 도감 실행/UI는 별도 연결 대상입니다.")) TSoftObjectPtr<UTexture2D> Image;
};
