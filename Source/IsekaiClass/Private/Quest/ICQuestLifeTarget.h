#pragma once

#include "Data/Rows/Quest/ICQuestRow.h"

namespace ICQuestLifeTarget
{
    // Match both the requested source/action and its output; an activity filter is additive.
    inline bool Matches(const FICQuestObjectiveDefinition& Objective, EICDefinitionType SourceType,
        FName SourceID, FName ProductionID, EICLifeActivity Activity, const TArray<FICItemAmount>& Yields)
    {
        if (Objective.bFilterLifeActivity && Objective.LifeActivity != Activity) return false;
        auto HasItem = [&Yields](FName ID)
        {
            return Yields.ContainsByPredicate([ID](const FICItemAmount& Item)
            {
                return !ID.IsNone() && Item.ItemID == ID && Item.Count > 0;
            });
        };
        if (!Objective.OutputItemID.IsNone() && !HasItem(Objective.OutputItemID)) return false;
        if (Objective.Target.ID.IsNone()) return Objective.Type == EICQuestObjectiveType::Craft;
        if (Objective.Target.Type == EICDefinitionType::Item) return HasItem(Objective.Target.ID);
        if (Objective.Target.Type == EICDefinitionType::Production) return Objective.Target.ID == ProductionID;
        return Objective.Target.Type == SourceType && Objective.Target.ID == SourceID;
    }
}
