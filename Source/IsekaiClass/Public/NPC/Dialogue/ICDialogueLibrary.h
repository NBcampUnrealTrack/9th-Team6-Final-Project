#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Rows/NPC/ICDialogueRow.h"
#include "ICDialogueLibrary.generated.h"

class UDataTable;
class UStringTable;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueDefinitionResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FICDialogueRow Definition;
    UPROPERTY(BlueprintReadOnly) FString FailureReason;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICResolvedDialogueChoice
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName ChoiceID;
    UPROPERTY(BlueprintReadOnly) FText Text;
    UPROPERTY(BlueprintReadOnly) FName NextNodeID;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueNodeResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FName DialogueID;
    UPROPERTY(BlueprintReadOnly) FName NodeID;
    UPROPERTY(BlueprintReadOnly) FName SpeakerNPCID;
    UPROPERTY(BlueprintReadOnly) FText Text;
    UPROPERTY(BlueprintReadOnly) FName NextNodeID;
    UPROPERTY(BlueprintReadOnly) TArray<FICResolvedDialogueChoice> Choices;
    UPROPERTY(BlueprintReadOnly) FString FailureReason;
};

UCLASS()
class ISEKAICLASS_API UICDialogueLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="IC|Dialogue")
    static FICDialogueDefinitionResult GetDialogueDefinition(UDataTable* Table, FName DialogueID, bool bAllowTestContent = false);


    UFUNCTION(BlueprintPure, Category="IC|Dialogue")
    static FICDialogueNodeResult ResolveDialogueNode(UDataTable* Table, UStringTable* Strings, FName DialogueID, FName NodeID, bool bAllowTestContent = false);
};
