#pragma once
#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICQuestStateTypes.generated.h"

USTRUCT(BlueprintType)
struct FICQuestState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bAccepted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bCompleted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, int32> Objectives;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bRewardClaimed = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid RewardTransactionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 CompletionCount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double AcceptedAtGameMinute = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double CompletedAtGameMinute = 0;
};
