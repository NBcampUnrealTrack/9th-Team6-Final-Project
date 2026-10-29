#pragma once

#include "Data/Rows/Quest/ICQuestRow.h"

class UWorld;
struct FICInventoryState;
struct FICQuestState;
struct FICWorldState;

/** Read-only quest rules. Effective stat objectives read the current player's ability component. */
class FICQuestObjectiveEvaluator
{
public:
    explicit FICQuestObjectiveEvaluator(const UWorld* InWorld) : ContextWorld(InWorld) {}

    bool IsComplete(const FICQuestObjectiveDefinition& Objective, const FICQuestState& Quest,
        const FICWorldState& World) const;
    bool GetProgress(const FICQuestObjectiveDefinition& Objective, const FICQuestState& Quest,
        const FICWorldState& World, int32& CurrentCount) const;
    bool IsGroupComplete(const FICQuestObjectiveGroup& Group, const FICQuestState& Quest,
        const FICWorldState& World) const;
    bool AreObjectivesComplete(const FICQuestRow& Definition, const FICQuestState& Quest,
        const FICWorldState& World) const;

    static int32 CountOwnedItem(const FICInventoryState& Inventory, FName ItemID);

private:
    static bool CompareValues(double Value, double RequiredValue, EICComparison Comparison);
    const UWorld* ContextWorld;
};
