#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICScheduleRow.generated.h"

UENUM(BlueprintType, meta=(ToolTip="NPC 일과 구간의 행동 종류입니다. 실제 이동·AI 실행 연결은 아직 필요합니다."))
enum class EICScheduleActivity : uint8 { Idle UMETA(ToolTip="해당 위치에서 대기합니다.\nNPC 일과 구간의 행동 종류입니다. 실제 이동·AI 실행 연결은 아직 필요합니다."), Move UMETA(ToolTip="출발 앵커에서 도착 앵커로 이동합니다.\nNPC 일과 구간의 행동 종류입니다. 실제 이동·AI 실행 연결은 아직 필요합니다."), Work UMETA(ToolTip="작업 위치에서 일과 행동을 수행합니다.\nNPC 일과 구간의 행동 종류입니다. 실제 이동·AI 실행 연결은 아직 필요합니다."), Sleep UMETA(ToolTip="수면 위치에서 수면 행동을 수행합니다.\nNPC 일과 구간의 행동 종류입니다. 실제 이동·AI 실행 연결은 아직 필요합니다.") };

/** A daily interval [StartGameMinute, EndGameMinute). Split intervals that cross midnight. */
USTRUCT(BlueprintType, meta=(ToolTip="하루 일과의 한 시간 구간입니다. 0~1440의 GameClock 분으로 시작/끝과 출발/도착 앵커·활동을 정의합니다. 겹침과 잘못된 구간은 검사합니다. 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.", ShortToolTip="하루 일과의 한 시간 구간입니다."))
struct ISEKAICLASS_API FICScheduleSegment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ToolTip="일과 안의 구간 고유 로컬 키입니다. 구간 상태/서비스/복구를 연결하므로 중복을 피합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) FName SegmentID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="0", ClampMax="1439", Units="min", ToolTip="일과 시작 하루 분입니다. 0=00:00, 480=08:00이며 종료보다 작은 시간으로 구간을 작성합니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n최댓값: 1439.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) int32 StartGameMinute = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="1", ClampMax="1440", Units="min", ToolTip="일과 종료 하루 분입니다. 1440은 자정 끝점이며 StartGameMinute보다 뒤인 구간을 작성합니다. 자정을 넘는 일정은 구간을 나눠 작성하세요.\n입력 자료형: 정수.\nC++ 초기값: 1440. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 1.\n최댓값: 1440.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) int32 EndGameMinute = 1440;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ToolTip="생활 생산/일과 활동 분류입니다. 상위 필드 타입에 맞춰 제작·요리·심기·수확·낚시·채집 또는 대기·이동·작업·수면을 선택합니다.\n입력 자료형: 선택 목록 (EICScheduleActivity).\nC++ 초기값: Idle — 해당 위치에서 대기합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) EICScheduleActivity Activity = EICScheduleActivity::Idle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ToolTip="일과 이동 출발 Anchor ID입니다. 도착 위치와 함께 작성하고 이동 실행을 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) FName DepartureAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ToolTip="일과 이동 목표 Anchor ID입니다. 실제 이동 경로가 존재하는 위치를 사용합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) FName DestinationAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ToolTip="일과 활동을 구분할 GameplayTag입니다. 실제 AI/연출 실행에서 해석할 태그를 등록합니다.\n입력 자료형: GameplayTag.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) FGameplayTag ActivityTag;
    // Work segments identify the offered service; moving and sleeping never count as on duty.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ToolTip="NPC/일과 구간에서 제공할 Service ID 목록입니다. 영업/조건 실행 연결도 필요합니다.\n입력 자료형: 이름/ID 배열.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) TArray<FName> ServiceIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="일과 연출 ResourceReference ID입니다. 소품/애니메이션을 확인하고 실제 실행에 연결합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 실제 NPC 일과/이동 실행은 아직 연결 대상입니다.")) FName ActivityResourceID;
};

USTRUCT(BlueprintType, meta=(ToolTip="NPC의 하루 일과 구간 목록과 경로 실패 복구 규칙입니다. NPC.ScheduleID로 연결합니다. 실제 일과 AI 실행은 아직 연결 대상입니다.", ShortToolTip="NPC의 하루 일과 구간 목록과 경로 실패 복구 규칙입니다."))
struct ISEKAICLASS_API FICScheduleRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ToolTip="하루 일과 구간 배열입니다. 시간 겹침/누락·이동 앵커·서비스를 검사합니다.\n입력 자료형: 구조/객체 (FICScheduleSegment) 배열.\n소속 정의/시스템: 실제 일과 AI 실행은 아직 연결 대상입니다.")) TArray<FICScheduleSegment> Segments;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="0", Units="s", ToolTip="일과 이동 경로 실패 복구 지연 PlayClock 초입니다. 실제 AI 경로 실행은 별도 연결입니다.\n입력 자료형: 배정밀도 소수.\nC++ 초기값: 5.0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n최솟값: 0.\n소속 정의/시스템: 실제 일과 AI 실행은 아직 연결 대상입니다.")) double PathFailureRecoveryPlaySeconds = 5.0;
    // Recovery is permitted only off screen at a safe destination anchor. Current path/position is never row data.
};
