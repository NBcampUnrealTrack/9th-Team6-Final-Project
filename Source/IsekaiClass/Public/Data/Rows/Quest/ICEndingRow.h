#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICEndingRow.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEndingPresentation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(AllowedClasses="/Script/LevelSequence.LevelSequence"))
    FSoftObjectPath CinematicSequence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftClassPtr<UUserWidget> CreditsWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftClassPtr<UUserWidget> ResultWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    FString CreditsTextKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    FString ResultTitleKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    FString ResultDescriptionKey;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEndingRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements")
    FName FinalBossID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements")
    FName EndingQuestID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements")
    FName ProgressionID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HiddenEnding")
    TArray<FName> HiddenAffinityNPCIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NormalEnding")
    FName NormalEndingRewardID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NormalEnding")
    FName WorldReturnAnchorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NormalEnding")
    FICEndingPresentation NormalPresentation;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HiddenEnding")
    FICEndingPresentation HiddenPresentation;
};
