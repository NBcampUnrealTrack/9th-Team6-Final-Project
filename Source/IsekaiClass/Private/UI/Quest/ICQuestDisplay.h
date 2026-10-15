#pragma once

#include "CoreMinimal.h"

class UICQuestSubsystem;
struct FICQuestObjectiveDefinition;
struct FICQuestState;
struct FICWorldState;

class FICQuestDisplay
{
public:
    static void GetActiveQuestIDs(const FICWorldState& World, TArray<FName>& QuestIDs);
    static FText FormatObjective(const FText& Description, const FICQuestObjectiveDefinition& Objective,
        const FICQuestState& Quest, const FICWorldState& World, const UICQuestSubsystem* Quests);
};
