#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICProgressionRow.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICProgressionStageDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    FName StageID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    FString DescriptionKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    TArray<FICConditionGroup> EntryConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    TArray<FName> QuestIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    TArray<FICConditionGroup> CompletionConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stage")
    TArray<FName> NextStageIDs;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICProgressionRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression")
    FName InitialStageID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression")
    TArray<FICProgressionStageDefinition> Stages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FinalBoss")
    FName FinalBossID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FinalBoss")
    FName EndingQuestID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FinalBoss")
    TArray<FICConditionGroup> FinalBossUnlockConditions;
};
