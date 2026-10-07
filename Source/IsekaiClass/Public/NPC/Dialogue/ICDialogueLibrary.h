#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Rows/NPC/ICDialogueRow.h"
#include "ICDialogueLibrary.generated.h"

class UDataTable;
class UStringTable;

USTRUCT(BlueprintType, meta=(ToolTip="대사 정의 조회 결과입니다. 성공할 때만 Definition을 사용하고 실패 원인은 FailureReason으로 확인합니다.", ShortToolTip="대사 정의 조회 결과입니다."))
struct ISEKAICLASS_API FICDialogueDefinitionResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회/준비 성공입니다. true일 때만 정의/후보 출력을 사용하고 false이면 원인 문자열을 확인합니다. 준비 성공과 원본 거래 확정 성공은 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회 성공 때 사용할 정적 정의 출력입니다. bSuccess가 false이면 초기화된 출력을 게임 변경에 사용하지 않습니다.\n입력 자료형: 구조/객체 (FICDialogueRow).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FICDialogueRow Definition;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="실패한 조회의 사람이 읽을 원인입니다. bSuccess가 false일 때 표시/로그에 사용하며 실패한 부분 출력은 사용하지 않습니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString FailureReason;
};

USTRUCT(BlueprintType, meta=(ToolTip="String Table에서 본문을 해석한 표시 선택지입니다. ChoiceID와 NextNodeID는 대사 그래프 연결용 로컬 키입니다.", ShortToolTip="String Table에서 본문을 해석한 표시 선택지입니다."))
struct ISEKAICLASS_API FICResolvedDialogueChoice
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="대사 노드 안의 선택지 로컬 키입니다. 같은 노드에서 중복시키지 않으며 선택지 문장 키/다음 노드와 구분합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName ChoiceID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="String Table에서 해석한 FText입니다. 현지화 식별자를 유지하고 실패 시 부분 문자열을 표시하지 않습니다.\n입력 자료형: 현지화 가능한 텍스트.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FText Text;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="같은 대사 안의 다음 로컬 Node ID입니다. None은 다음 노드 연결 없음이며 선택지 유무와 종료 흐름을 함께 확인합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName NextNodeID;
};

USTRUCT(BlueprintType, meta=(ToolTip="대사 노드의 해석된 본문·화자·다음 노드·선택지 결과입니다. bSuccess가 false이면 부분 표시를 진행하지 마세요.", ShortToolTip="대사 노드의 해석된 본문·화자·다음 노드·선택지 결과입니다."))
struct ISEKAICLASS_API FICDialogueNodeResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회/준비 성공입니다. true일 때만 정의/후보 출력을 사용하고 false이면 원인 문자열을 확인합니다. 준비 성공과 원본 거래 확정 성공은 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="Dialogue DT ID입니다. 같은 대사 내부 NodeID/ChoiceID와 구분합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName DialogueID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="대사 안의 노드 로컬 키입니다. Nodes 안에서 고유해야 하며 Dialogue ID와 다릅니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName NodeID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="해당 대사 노드 화자 NPC ID입니다. 상위 NPCID와 별개 화자를 정의할 수 있으며 실제 NPC 참조를 검사합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName SpeakerNPCID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="String Table에서 해석한 FText입니다. 현지화 식별자를 유지하고 실패 시 부분 문자열을 표시하지 않습니다.\n입력 자료형: 현지화 가능한 텍스트.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FText Text;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="같은 대사 안의 다음 로컬 Node ID입니다. None은 다음 노드 연결 없음이며 선택지 유무와 종료 흐름을 함께 확인합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName NextNodeID;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="노드의 표시 선택지 목록입니다. 선택지 ID·String Table 키·다음 노드를 작성합니다. 보상/상태 변경을 직접 실행하는 목록이 아닙니다.\n입력 자료형: 구조/객체 (FICResolvedDialogueChoice) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<FICResolvedDialogueChoice> Choices;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="실패한 조회의 사람이 읽을 원인입니다. bSuccess가 false일 때 표시/로그에 사용하며 실패한 부분 출력은 사용하지 않습니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString FailureReason;
};

UCLASS(meta=(ToolTip="대사 DT 조회와 String Table 본문/선택지 해석 함수입니다. 조회 성공 여부를 확인해야 하며 이 함수 자체가 퀘스트·보상·관계 변경을 실행하지 않습니다.", ShortToolTip="대사 DT 조회와 String Table 본문/선택지 해석 함수입니다."))
class ISEKAICLASS_API UICDialogueLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="IC|Dialogue", meta=(ToolTip="Dialogue DT에서 ID/RowName·폐기·시험용·그래프 연결을 검사해 대사 정의를 조회합니다.\n@param Table 조회할 실제 DT 또는 String Table 에셋입니다. 함수가 요구하는 RowStruct/에셋 종류와 맞아야 합니다. 입력 핀. 자료형: UDataTable*.\n@param DialogueID Dialogue DT의 RowName/ID입니다. 입력 핀. 자료형: 이름/ID.\n@param bAllowTestContent 시험용 표시가 있는 대사를 허용할지 지정합니다. 실제 플레이에서는 false를 사용하세요. 입력 핀. 자료형: 체크박스(true/false).\n@return bSuccess·Definition·FailureReason을 가진 결과입니다. 실패 정의를 표시하지 마세요. 자료형: 구조/객체 (FICDialogueDefinitionResult)."))
    static FICDialogueDefinitionResult GetDialogueDefinition(UDataTable* Table, FName DialogueID, bool bAllowTestContent = false);

    // None selects EntryNodeID. This resolves presentation only; it evaluates no WorldState conditions.
    UFUNCTION(BlueprintPure, Category="IC|Dialogue", meta=(ToolTip="대사 정의의 NodeID를 찾아 화자·본문·선택지 String Table 키를 현지화 FText로 해석합니다. 누락 참조나 키는 실패합니다.\n@param Table 조회할 실제 DT 또는 String Table 에셋입니다. 함수가 요구하는 RowStruct/에셋 종류와 맞아야 합니다. 입력 핀. 자료형: UDataTable*.\n@param Strings 대사 본문·선택지의 원문이 들어 있는 String Table입니다. 입력 핀. 자료형: UStringTable*.\n@param DialogueID Dialogue DT의 RowName/ID입니다. 입력 핀. 자료형: 이름/ID.\n@param NodeID 선택한 대사 정의 내부의 노드 ID입니다. 빈 값 처리와 시작 노드 선택은 조회 결과를 확인하세요. 입력 핀. 자료형: 이름/ID.\n@param bAllowTestContent 시험용 표시가 있는 대사를 허용할지 지정합니다. 실제 플레이에서는 false를 사용하세요. 입력 핀. 자료형: 체크박스(true/false).\n@return bSuccess·노드·본문·선택지·FailureReason을 가진 결과입니다. 자료형: 구조/객체 (FICDialogueNodeResult)."))
    static FICDialogueNodeResult ResolveDialogueNode(UDataTable* Table, UStringTable* Strings, FName DialogueID, FName NodeID, bool bAllowTestContent = false);
};
