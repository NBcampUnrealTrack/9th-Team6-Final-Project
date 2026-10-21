#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ICTextLibrary.generated.h"

class UStringTable;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICTextLookupResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FText Text;
    UPROPERTY(BlueprintReadOnly) FString FailureReason;
};

UCLASS()
class ISEKAICLASS_API UICTextLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:

    UFUNCTION(BlueprintPure, Category="IC|Text")
    static FICTextLookupResult ResolveText(UStringTable* Table, const FString& Key);

    UFUNCTION(BlueprintCallable, Category="IC|Text|Authoring", meta=(DevelopmentOnly))
    static bool ReplaceTextSources(UStringTable* Table, const TMap<FString,FString>& Sources);
};
