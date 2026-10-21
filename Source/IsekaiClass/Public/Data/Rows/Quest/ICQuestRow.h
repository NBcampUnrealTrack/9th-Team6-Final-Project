#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICQuestRow.generated.h"

UENUM(BlueprintType)
enum class EICQuestType : uint8
{
    General ,
    Chain ,
    Main
};


UENUM(BlueprintType)
enum class EICQuestObjectiveType : uint8
{
    Talk ,
    DeliverItem ,
    Defeat ,
    Collect ,
    Craft ,
    Affinity ,
    Stat ,
    Visit ,
    PossessItem
};

UENUM(BlueprintType)
enum class EICQuestCompletionMode : uint8
{
    NPCInteraction ,

    SystemEvent
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICQuestObjectiveDefinition
{
    GENERATED_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective")
    FName ObjectiveID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective")
    FString DescriptionKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective")
    EICQuestObjectiveType Type = EICQuestObjectiveType::Talk;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective")
    FICDefinitionRef Target;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Activity")
    FName OutputItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Activity")
    bool bFilterLifeActivity = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Activity", meta=(EditCondition="bFilterLifeActivity"))
    EICLifeActivity LifeActivity = EICLifeActivity::Gathering;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ClampMin="1"))
    int32 RequiredCount = 1;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState")
    double RequiredValue = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState")
    EICComparison Compare = EICComparison::GreaterOrEqual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState")
    EICStat RequiredStat = EICStat::None;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState")
    bool bUseEffectiveStats = false;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CurrentState")
    bool bAllowCurrentState = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FName DialogueID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance")
    FName GuidanceAnchorID;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICQuestObjectiveGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives")
    EICConditionPolicy Policy = EICConditionPolicy::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives")
    TArray<FICQuestObjectiveDefinition> Objectives;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICQuestRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    EICQuestType QuestType = EICQuestType::General;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    bool bTutorial = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance")
    bool IsAutoAccept = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FName RegionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FName ChainID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance")
    TArray<FName> PrerequisiteQuestIDs;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance")
    TArray<FICConditionGroup> AcceptanceConditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance")
    FName OfferNPCID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance")
    FName OfferAnchorID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acceptance")
    TArray<FICItemAmount> AcceptanceQuestItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives")
    EICConditionPolicy ObjectiveGroupPolicy = EICConditionPolicy::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objectives")
    TArray<FICQuestObjectiveGroup> ObjectiveGroups;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion")
    EICQuestCompletionMode CompletionMode = EICQuestCompletionMode::NPCInteraction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion")
    FName TurnInNPCID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion")
    FName TurnInAnchorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion")
    FName RewardID;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion")
    TArray<FName> SuccessorQuestIDs;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Completion")
    bool bEndingQuest = false;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat")
    bool bAllowAbandon = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Repeat")
    bool bRepeatable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FName OfferDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FName ActiveDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FName ReadyDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FName CompletedDialogueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UTexture2D> Icon;
};
