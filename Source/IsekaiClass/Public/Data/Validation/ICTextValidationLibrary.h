#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Validation/ICDefinitionLibrary.h"
#include "ICTextValidationLibrary.generated.h"

class UStringTable;
class UDataTable;

UCLASS()
class ISEKAICLASS_API UICTextValidationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:

    UFUNCTION(BlueprintCallable, Category="IC|Data|Validation")
    static TArray<FICDefinitionIssue> ValidateDialogueText(UDataTable* DialogueTable, UStringTable* Strings);
};
