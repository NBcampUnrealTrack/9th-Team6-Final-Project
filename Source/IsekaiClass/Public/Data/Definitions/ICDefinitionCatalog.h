#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICDefinitionCatalog.generated.h"

class UStringTable;

USTRUCT(BlueprintType, meta=(ToolTip="카탈로그의 DT 등록 항목 하나입니다. Type·RowStruct·Table을 함께 지정하고 같은 정의 ID를 여러 테이블에 중복 등록하지 마세요.", ShortToolTip="카탈로그의 DT 등록 항목 하나입니다."))
struct ISEKAICLASS_API FICDefinitionBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="현재 구조의 종류입니다. 종류별 필요한 대상/조건/참조를 작성하고 다른 종류의 ID를 섞지 않습니다.\n입력 자료형: 선택 목록 (EICDefinitionType).\nC++ 초기값: Item — 인벤토리 아이템 정의입니다.. 기존 DT 행/BP의 저장된 값은 해당 에셋 값이 우선합니다.")) EICDefinitionType Type = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="DT 네이티브 행 자료형입니다. Type과 Table.RowStruct가 같아야 조회/검사에 성공합니다.\n입력 자료형: 객체 참조 (UScriptStruct).")) TObjectPtr<UScriptStruct> RowStruct;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="등록/조회/검사 DT 참조 또는 경로입니다. 종류/RowStruct/행 ID를 맞추고 String Table 입력과 구분합니다.\n입력 자료형: Soft Object 에셋 참조 (UDataTable).")) TSoftObjectPtr<UDataTable> Table;
};

// The catalog is the sole registration point for static gameplay definitions.
UCLASS(BlueprintType, meta=(ToolTip="정적 정의 DT 50종과 String Table을 등록하는 DataAsset입니다. Tables의 정의 종류·RowStruct·DT가 서로 맞아야 합니다. Project Settings의 IsekaiClass Definitions에서 이 카탈로그를 지정하세요.", ShortToolTip="정적 정의 DT 50종과 String Table을 등록하는 DataAsset입니다."))
class ISEKAICLASS_API UICDefinitionCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ToolTip="카탈로그 DT 등록 목록입니다. 종류/RowStruct/에셋을 지정하고 같은 종류 ID를 중복시키지 않습니다.\n입력 자료형: 구조/객체 (FICDefinitionBinding) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<FICDefinitionBinding> Tables;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ToolTip="프로젝트 정의/대사 String Table 참조 목록입니다. 키 등록과 직접 FText 연결을 확인합니다. 등록만으로 모든 NameKey UI가 자동 해석되지는 않습니다.\n입력 자료형: Soft Object 에셋 참조 (UStringTable) 배열.\nBP에서는 조회 전용입니다. 상태 변경은 해당 시스템 API를 사용하세요.")) TArray<TSoftObjectPtr<UStringTable>> StringTables;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="IsekaiClass Definitions", ToolTip="프로젝트가 사용할 정의 카탈로그 설정입니다. DefaultGame.ini에 저장되며 GameInstance의 DefinitionSubsystem이 시작할 때 Catalog를 불러옵니다.", ShortToolTip="프로젝트가 사용할 정의 카탈로그 설정입니다."))
class ISEKAICLASS_API UICDefinitionSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category="Definitions", meta=(ToolTip="정적 정의 카탈로그 DataAsset 참조입니다. 프로젝트 설정에서 실제 DA_IC_DefinitionCatalog를 연결합니다. 없거나 로드 실패하면 조회가 실패합니다.\n입력 자료형: Soft Object 에셋 참조 (UICDefinitionCatalog).")) TSoftObjectPtr<UICDefinitionCatalog> Catalog;
};
