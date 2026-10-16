#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICDialogueRow.generated.h"

class UAnimMontage;

/** A presentation choice can select another dialogue node; it cannot grant rewards or alter quest/affinity/ending state. */
USTRUCT(BlueprintType, meta=(ToolTip="표시할 선택지의 ID·String Table 키·다음 노드입니다. 선택지는 대사 분기에만 사용하며 보상·퀘스트·관계 상태를 직접 변경하지 않습니다.", ShortToolTip="표시할 선택지의 ID·String Table 키·다음 노드입니다."))
struct ISEKAICLASS_API FICDialogueChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="대사 노드 안의 선택지 로컬 키입니다. 같은 노드에서 중복시키지 않으며 선택지 문장 키/다음 노드와 구분합니다.\n입력 자료형: 이름/ID.")) FName ChoiceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="대사/선택지의 String Table 키입니다. 예: DLG_R08_001.LINE_001. 원문을 직접 넣지 말고 문자열 테이블에 같은 키를 등록합니다. 누락이면 노드 조회가 실패합니다.\n입력 자료형: 문자열.")) FString TextKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="같은 대사 안의 다음 로컬 Node ID입니다. None은 다음 노드 연결 없음이며 선택지 유무와 종료 흐름을 함께 확인합니다.\n입력 자료형: 이름/ID.")) FName NextNodeID;
};

USTRUCT(BlueprintType, meta=(ToolTip="대사의 한 노드입니다. 노드 ID·화자·String Table 키·다음 노드/선택지·제스처를 작성합니다. String Table 조회는 현지화 식별자를 유지합니다.", ShortToolTip="대사의 한 노드입니다."))
struct ISEKAICLASS_API FICDialogueNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="대사 안의 노드 로컬 키입니다. Nodes 안에서 고유해야 하며 Dialogue ID와 다릅니다.\n입력 자료형: 이름/ID.")) FName NodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="해당 대사 노드 화자 NPC ID입니다. 상위 NPCID와 별개 화자를 정의할 수 있으며 실제 NPC 참조를 검사합니다.\n입력 자료형: 이름/ID.")) FName SpeakerNPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="대사/선택지의 String Table 키입니다. 예: DLG_R08_001.LINE_001. 원문을 직접 넣지 말고 문자열 테이블에 같은 키를 등록합니다. 누락이면 노드 조회가 실패합니다.\n입력 자료형: 문자열.")) FString TextKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="같은 대사 안의 다음 로컬 Node ID입니다. None은 다음 노드 연결 없음이며 선택지 유무와 종료 흐름을 함께 확인합니다.\n입력 자료형: 이름/ID.")) FName NextNodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="노드의 표시 선택지 목록입니다. 선택지 ID·String Table 키·다음 노드를 작성합니다. 보상/상태 변경을 직접 실행하는 목록이 아닙니다.\n입력 자료형: 구조/객체 (FICDialogueChoice) 배열.")) TArray<FICDialogueChoice> Choices;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ToolTip="대사 제스처 AnimMontage입니다. 조회만으로 재생되지 않으므로 액터/대화 실행을 연결합니다.\n입력 자료형: Soft Object 에셋 참조 (UAnimMontage).")) TSoftObjectPtr<UAnimMontage> GestureMontage;
};

USTRUCT(BlueprintType, meta=(ToolTip="NPC 대사 그래프의 정적 정의입니다. EntryNodeID가 Nodes 안에 존재해야 하고 연결 노드·선택지·텍스트 키 검사를 지원합니다. 대화 UI와 일반 조건 판정의 실행 연결은 별도입니다.", ShortToolTip="NPC 대사 그래프의 정적 정의입니다."))
struct ISEKAICLASS_API FICDialogueRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="NPC DT의 정의 ID입니다. R08 지역 규격의 같은 RowName/ID를 지정하고 액터 이름·인스턴스 GUID와 구분합니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 대화 UI와 일반 조건 판정의 실행 연결은 별도입니다.")) FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="대사 시작 로컬 Node ID입니다. Nodes에 반드시 존재하며 대사 정의 ID와 다릅니다.\n입력 자료형: 이름/ID.\n소속 정의/시스템: 대화 UI와 일반 조건 판정의 실행 연결은 별도입니다.")) FName EntryNodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="대사/서비스/스폰 등 이용 조건입니다. 참조 검사는 제공되지만 모든 런타임 조건 평가가 구현된 것은 아닙니다.\n입력 자료형: 구조/객체 (FICConditionGroup).\n소속 정의/시스템: 대화 UI와 일반 조건 판정의 실행 연결은 별도입니다.")) FICConditionGroup Availability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="여러 대사의 선택 우선순위 키입니다. 실제 조건/정렬 선택기는 별도 연결 대상입니다.\n입력 자료형: 정수.\nC++ 초기값: 0. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\n소속 정의/시스템: 대화 UI와 일반 조건 판정의 실행 연결은 별도입니다.")) int32 SelectionPriority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ToolTip="대사 그래프 노드 목록입니다. EntryNodeID/NextNodeID/선택지 연결과 모든 TextKey를 검사합니다.\n입력 자료형: 구조/객체 (FICDialogueNode) 배열.\n소속 정의/시스템: 대화 UI와 일반 조건 판정의 실행 연결은 별도입니다.")) TArray<FICDialogueNode> Nodes;
};
