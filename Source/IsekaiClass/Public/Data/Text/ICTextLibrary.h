#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ICTextLibrary.generated.h"

class UStringTable;

USTRUCT(BlueprintType, meta=(ToolTip="String Table 조회 결과입니다. bSuccess를 먼저 확인하세요. 성공 Text는 현지화 식별자를 유지하고 실패하면 부분 텍스트 없이 FailureReason을 반환합니다.", ShortToolTip="String Table 조회 결과입니다."))
struct ISEKAICLASS_API FICTextLookupResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회/준비 성공입니다. true일 때만 정의/후보 출력을 사용하고 false이면 원인 문자열을 확인합니다. 준비 성공과 원본 거래 확정 성공은 별개입니다.\n입력 자료형: 체크박스(true/false).\nC++ 초기값: 꺼짐(false). 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="String Table에서 해석한 FText입니다. 현지화 식별자를 유지하고 실패 시 부분 문자열을 표시하지 않습니다.\n입력 자료형: 현지화 가능한 텍스트.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FText Text;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="실패한 조회의 사람이 읽을 원인입니다. bSuccess가 false일 때 표시/로그에 사용하며 실패한 부분 출력은 사용하지 않습니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString FailureReason;
};

UCLASS(meta=(ToolTip="String Table 텍스트 조회와 에디터 원문 일괄 교체 함수입니다. 텍스트를 FString으로 재생성하지 않아 현지화 식별자를 유지하며 누락된 키는 실패를 반환합니다.", ShortToolTip="String Table 텍스트 조회와 에디터 원문 일괄 교체 함수입니다."))
class ISEKAICLASS_API UICTextLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Keeps String Table localization identity. A failed lookup never returns partial text.
    UFUNCTION(BlueprintPure, Category="IC|Text", meta=(ToolTip="String Table의 키를 현지화 FText로 조회합니다. 문자열로 다시 만들지 않아 테이블 식별자를 유지합니다. 누락된 에셋/키는 실패를 반환합니다.\n@param Table 조회할 실제 DT 또는 String Table 에셋입니다. 함수가 요구하는 RowStruct/에셋 종류와 맞아야 합니다. 입력 핀. 자료형: UStringTable*.\n@param Key 조회할 String Table 키입니다. 대소문자와 철자를 원문 등록 키와 맞추세요. 입력 핀. 자료형: 문자열.\n@return bSuccess·Text·FailureReason을 가진 조회 결과입니다. bSuccess가 true일 때 Text를 사용하세요. 자료형: 구조/객체 (FICTextLookupResult)."))
    static FICTextLookupResult ResolveText(UStringTable* Table, const FString& Key);
    // Editor authoring supports an empty table and validates the whole input first.
    UFUNCTION(BlueprintCallable, Category="IC|Text|Authoring", meta=(DevelopmentOnly, ToolTip="String Table 원문 전체를 Sources의 키/문구 목록으로 교체합니다. 목록에 없는 기존 키도 제거되므로 부분 변경용으로 호출하지 마세요. 에디터에서 호출한 뒤 에셋 저장이 필요합니다.\n@param Table 조회할 실제 DT 또는 String Table 에셋입니다. 함수가 요구하는 RowStruct/에셋 종류와 맞아야 합니다. 입력 핀. 자료형: UStringTable*.\n@param Sources String Table을 대체할 전체 키→원문 목록입니다. 목록에 없는 기존 키는 제거됩니다. 입력 핀. 자료형: 문자열 → 문자열 맵.\n@return 교체 작업이 가능한 테이블이면 true입니다. 자료형: 체크박스(true/false)."))
    static bool ReplaceTextSources(UStringTable* Table, const TMap<FString,FString>& Sources);
};
