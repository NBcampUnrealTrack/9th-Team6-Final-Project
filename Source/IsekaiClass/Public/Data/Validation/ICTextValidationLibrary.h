#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Validation/ICDefinitionLibrary.h"
#include "ICTextValidationLibrary.generated.h"

class UStringTable;
class UDataTable;

UCLASS(meta=(ToolTip="대사 그래프와 String Table 키를 함께 검사하는 도구입니다. 누락된 본문·선택지 키와 잘못된 그래프 연결을 확인합니다.", ShortToolTip="대사 그래프와 String Table 키를 함께 검사하는 도구입니다."))
class ISEKAICLASS_API UICTextValidationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Includes structural checks and reports Table / Row / Field / Reason.
    UFUNCTION(BlueprintCallable, Category="IC|Data|Validation", meta=(ToolTip="Dialogue DT의 그래프 연결과 본문·선택지 String Table 키를 함께 검사합니다. 대사 표시 전에 누락 키와 잘못된 연결을 수정하세요.\n@param DialogueTable 대사 그래프를 담은 Dialogue DT입니다. FICDialogueRow 구조의 테이블을 전달하세요. 입력 핀. 자료형: UDataTable*.\n@param Strings 대사 본문·선택지의 원문이 들어 있는 String Table입니다. 입력 핀. 자료형: UStringTable*.\n@return 발견한 대사/텍스트 오류·경고 목록입니다. 자료형: 구조/객체 (FICDefinitionIssue) 배열."))
    static TArray<FICDefinitionIssue> ValidateDialogueText(UDataTable* DialogueTable, UStringTable* Strings);
};
