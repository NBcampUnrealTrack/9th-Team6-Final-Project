#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICDefinitionLibrary.generated.h"

UENUM(BlueprintType)
enum class EICDefinitionSeverity : uint8 { Warning , Error  };

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDefinitionIssue
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EICDefinitionSeverity Severity = EICDefinitionSeverity::Error;
    UPROPERTY(BlueprintReadOnly) FString Table;
    UPROPERTY(BlueprintReadOnly) FName Row;
    UPROPERTY(BlueprintReadOnly) FString Field;
    UPROPERTY(BlueprintReadOnly) FString Reason;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDefinitionSchema
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EICDefinitionType Type = EICDefinitionType::Settings;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UScriptStruct> RowStruct;
    UPROPERTY(BlueprintReadOnly) FString Folder;
    UPROPERTY(BlueprintReadOnly) FName AssetName;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDefinitionTable
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICDefinitionType Type = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UDataTable> Table;
};

UCLASS()
class ISEKAICLASS_API UICDefinitionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="IC|Data") static TArray<FICDefinitionSchema> GetDefinitionSchemas();

    UFUNCTION(BlueprintCallable, Category="IC|Data")
    static bool ValidateTables(const TArray<FICDefinitionTable>& Tables, bool bRequireCompleteContent, TArray<FICDefinitionIssue>& Issues);

    UFUNCTION(BlueprintCallable, Category="IC|Data")
    static TArray<FICDefinitionIssue> GetValidationIssues(const TArray<FICDefinitionTable>& Tables, bool bRequireCompleteContent);
    static bool IsDefinitionIDValid(FName ID);
};
