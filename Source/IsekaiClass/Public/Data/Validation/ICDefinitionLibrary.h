#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICDefinitionLibrary.generated.h"

UENUM(BlueprintType, meta=(ToolTip="데이터 검사 메시지의 심각도입니다."))
enum class EICDefinitionSeverity : uint8 { Warning UMETA(ToolTip="작성 중 미완성이나 권장 사항입니다. 경고 내용을 확인하되 구조 오류와 구분합니다.\n데이터 검사 메시지의 심각도입니다."), Error UMETA(ToolTip="자료형·필수값·참조·범위 오류입니다. 실행에 사용하기 전에 수정해야 합니다.\n데이터 검사 메시지의 심각도입니다.") };

USTRUCT(BlueprintType, meta=(ToolTip="데이터 검사 메시지 하나입니다. Severity와 Table·Row·Field·Reason을 보고 DT 행을 수정합니다. Warning은 콘텐츠 작성 중 미완성 안내, Error는 구조/참조 오류입니다.", ShortToolTip="데이터 검사 메시지 하나입니다."))
struct ISEKAICLASS_API FICDefinitionIssue
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="검사 메시지 경고/오류입니다. Error는 구조/참조 수정 필요, Warning은 작성 중 미완성 안내 등을 확인합니다.\n입력 자료형: 선택 목록 (EICDefinitionSeverity).\nC++ 초기값: Error — 자료형·필수값·참조·범위 오류입니다. 실행에 사용하기 전에 수정해야 합니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICDefinitionSeverity Severity = EICDefinitionSeverity::Error;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="등록/조회/검사 DT 참조 또는 경로입니다. 종류/RowStruct/행 ID를 맞추고 String Table 입력과 구분합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Table;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회/검사 대상 DT RowName입니다. 같은 행 ID와 일치 여부를 확인합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName Row;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="오류가 발생한 필드 이름/경로입니다. 해당 DT 행/구조를 열어 원인과 함께 수정합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Field;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="조회/아이템/제작 실패 원인입니다. 실패 때 표시/로그로 확인하고 원본 변경을 계속하지 않습니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Reason;
};

USTRUCT(BlueprintType, meta=(ToolTip="지원하는 DT 종류의 네이티브 RowStruct와 기본 생성 폴더/이름입니다. 에디터에서 DT 생성 시 구조체를 고르는 기준으로 사용합니다.", ShortToolTip="지원하는 DT 종류의 네이티브 RowStruct와 기본 생성 폴더/이름입니다."))
struct ISEKAICLASS_API FICDefinitionSchema
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICDefinitionType).\nC++ 초기값: Settings — 새 게임·시간·능력치·전투·인벤토리·파티·지도·저장 설정 DT의 공통 분류입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) EICDefinitionType Type = EICDefinitionType::Settings;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="DT 네이티브 행 자료형입니다. Type과 Table.RowStruct가 같아야 조회/검사에 성공합니다.\n입력 자료형: 객체 참조 (UScriptStruct).\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TObjectPtr<UScriptStruct> RowStruct;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="DT 권장 생성 콘텐츠 경로입니다. /Game 경로를 사용하며 AssetName/RowStruct와 함께 확인합니다.\n입력 자료형: 문자열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FString Folder;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="권장 DT 에셋 이름입니다. Folder와 RowStruct를 함께 사용해 해당 종류의 DT를 생성합니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) FName AssetName;
};

USTRUCT(BlueprintType, meta=(ToolTip="검사기에 넘길 DT와 정의 종류 묶음입니다. Type과 실제 Table.RowStruct가 일치해야 합니다.", ShortToolTip="검사기에 넘길 DT와 정의 종류 묶음입니다."))
struct ISEKAICLASS_API FICDefinitionTable
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICDefinitionType).\nC++ 초기값: Item — 인벤토리 아이템 정의입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICDefinitionType Type = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="등록/조회/검사 DT 참조 또는 경로입니다. 종류/RowStruct/행 ID를 맞추고 String Table 입력과 구분합니다.\n입력 자료형: 객체 참조 (UDataTable).")) TObjectPtr<UDataTable> Table;
};

UCLASS(meta=(ToolTip="DT 종류/RowStruct/추천 에셋 경로 목록과 정의 검사 함수입니다. 편집 중 검사와 콘텐츠 완성 검사를 구분하며 오류마다 테이블·행·필드·원인을 제공합니다.", ShortToolTip="DT 종류/RowStruct/추천 에셋 경로 목록과 정의 검사 함수입니다."))
class ISEKAICLASS_API UICDefinitionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="IC|Data", meta=(ToolTip="지원하는 DT 50종의 RowStruct·추천 에셋 이름·경로·정의 종류를 조회합니다. 새 DT를 만들 때 RowStruct 선택과 카탈로그 등록의 기준으로 사용하세요.\n@return 지원 스키마 목록입니다. 실제 DT 행 내용과 다른 메타 정보입니다. 자료형: 구조/객체 (FICDefinitionSchema) 배열.")) static TArray<FICDefinitionSchema> GetDefinitionSchemas();
    // Partial authoring keeps unresolved references as warnings. Release checks require complete content.
    UFUNCTION(BlueprintCallable, Category="IC|Data", meta=(ToolTip="주어진 DT 집합의 ID·필수값·자료형·범위·참조·순환·콘텐츠 수량을 검사합니다. bRequireCompleteContent=false는 작성 중 검사, true는 필수 콘텐츠 완성 검사입니다.\n@param Tables 검사할 정의 DT 등록 목록입니다. 종류·테이블을 맞추고 참조 대상 DT도 포함하세요. 입력 핀. 자료형: 구조/객체 (FICDefinitionTable) 배열.\n@param bRequireCompleteContent true는 필수 콘텐츠 수량까지 검사하고 false는 작성 중 데이터 규격·참조를 검사합니다. 입력 핀. 자료형: 체크박스(true/false).\n@param Issues 출력 오류/경고 목록입니다. Severity·Table·Row·Field·Reason으로 수정 위치를 찾으세요. 입력/출력 핀. 자료형: 구조/객체 (FICDefinitionIssue) 배열.\n@return Error가 없으면 true입니다. Warning도 Issues에서 확인하세요. 자료형: 체크박스(true/false)."))
    static bool ValidateTables(const TArray<FICDefinitionTable>& Tables, bool bRequireCompleteContent, TArray<FICDefinitionIssue>& Issues);
    // Always exposes diagnostics, including failed validation, to Python/editor tools.
    UFUNCTION(BlueprintCallable, Category="IC|Data", meta=(ToolTip="정의 DT 집합을 검사하여 오류/경고 목록만 반환합니다. 작성 중/완성 검사 모드를 지정할 수 있습니다.\n@param Tables 검사할 정의 DT 등록 목록입니다. 종류·테이블을 맞추고 참조 대상 DT도 포함하세요. 입력 핀. 자료형: 구조/객체 (FICDefinitionTable) 배열.\n@param bRequireCompleteContent true는 필수 콘텐츠 수량까지 검사하고 false는 작성 중 데이터 규격·참조를 검사합니다. 입력 핀. 자료형: 체크박스(true/false).\n@return 테이블·행·필드·원인을 가진 검사 메시지 목록입니다. 자료형: 구조/객체 (FICDefinitionIssue) 배열."))
    static TArray<FICDefinitionIssue> GetValidationIssues(const TArray<FICDefinitionTable>& Tables, bool bRequireCompleteContent);
    static bool IsDefinitionIDValid(FName ID);
};
