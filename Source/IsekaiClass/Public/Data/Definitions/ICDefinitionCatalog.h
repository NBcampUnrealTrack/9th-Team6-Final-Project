#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICDefinitionCatalog.generated.h"

class UStringTable;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDefinitionBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICDefinitionType Type = EICDefinitionType::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UScriptStruct> RowStruct;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UDataTable> Table;
};


UCLASS(BlueprintType)
class ISEKAICLASS_API UICDefinitionCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FICDefinitionBinding> Tables;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TSoftObjectPtr<UStringTable>> StringTables;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="IsekaiClass Definitions"))
class ISEKAICLASS_API UICDefinitionSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category="Definitions") TSoftObjectPtr<UICDefinitionCatalog> Catalog;
};
