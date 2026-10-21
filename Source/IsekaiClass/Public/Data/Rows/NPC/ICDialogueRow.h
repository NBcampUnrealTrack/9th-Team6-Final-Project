#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICDialogueRow.generated.h"

class UAnimMontage;


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName ChoiceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FString TextKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NextNodeID;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName SpeakerNPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FString TextKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NextNodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FICDialogueChoice> Choices;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<UAnimMontage> GestureMontage;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName EntryNodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FICConditionGroup Availability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") int32 SelectionPriority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FICDialogueNode> Nodes;
};
