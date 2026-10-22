#include "Data/Validation/ICDefinitionLibrary.h"
#include "Debug/ICLog.h"
#include "Data/Rows/Life/ICCropRow.h"
#include "Data/Rows/Life/ICFishRow.h"
#include "Data/Rows/Life/ICFishingPointRow.h"
#include "Data/Rows/Life/ICProductionRow.h"
#include "Data/Rows/Life/ICRecipeRow.h"
#include "Data/Rows/Life/ICResourceRow.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "Data/Rows/Quest/ICEndingRow.h"
#include "Data/Rows/Quest/ICProgressionRow.h"
#include "Data/Rows/Quest/ICQuestRow.h"

namespace
{
struct FICRowSite
{
    const UDataTable* Table = nullptr;
    FName Row;
    bool bComplete = false;
};

void AddIssue(TArray<FICDefinitionIssue>& Issues, const FICRowSite& Site,
    const FString& Field, const FString& Reason, EICDefinitionSeverity Severity = EICDefinitionSeverity::Error)
{
    IC_TRACE_FUNCTION();
    FICDefinitionIssue Issue;
    Issue.Severity = Severity;
    Issue.Table = Site.Table ? Site.Table->GetPathName() : FString();
    Issue.Row = Site.Row;
    Issue.Field = Field;
    Issue.Reason = Reason;
    Issues.Add(MoveTemp(Issue));
}

void AddMissing(TArray<FICDefinitionIssue>& Issues, const FICRowSite& Site,
    const FString& Field, const FString& Reason)
{
    IC_TRACE_FUNCTION();
    AddIssue(Issues, Site, Field, Reason,
        Site.bComplete ? EICDefinitionSeverity::Error : EICDefinitionSeverity::Warning);
}

template <typename TRow, typename TCallback>
void ForEachActiveRow(const TArray<FICDefinitionTable>& Tables, bool bComplete, TCallback Callback)
{
    IC_TRACE_FUNCTION();
    for (const FICDefinitionTable& Entry : Tables)
    {
        const UDataTable* Table = Entry.Table.Get();
        if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(TRow::StaticStruct()))
        {
            continue;
        }
        for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
        {
            if (!Pair.Value)
            {
                continue;
            }
            const TRow& Row = *reinterpret_cast<const TRow*>(Pair.Value);
            if (!Row.bDeprecated && !Row.bTestOnly)
            {
                Callback(Row, FICRowSite{Table, Pair.Key, bComplete});
            }
        }
    }
}

void RequireName(TArray<FICDefinitionIssue>& Issues, const FICRowSite& Site,
    const FString& Field, FName Value)
{
    IC_TRACE_FUNCTION();
    if (Value.IsNone())
    {
        AddMissing(Issues, Site, Field, TEXT("An authored definition requires a nonempty reference."));
    }
}

void CheckUniqueNames(TArray<FICDefinitionIssue>& Issues, const FICRowSite& Site,
    const FString& Field, const TArray<FName>& Values)
{
    IC_TRACE_FUNCTION();
    TSet<FName> Seen;
    for (int32 Index = 0; Index < Values.Num(); ++Index)
    {
        const FString IndexedField = FString::Printf(TEXT("%s[%d]"), *Field, Index);
        RequireName(Issues, Site, IndexedField, Values[Index]);
        if (!Values[Index].IsNone() && Seen.Contains(Values[Index]))
        {
            AddIssue(Issues, Site, IndexedField, TEXT("The same reference is listed more than once."));
        }
        Seen.Add(Values[Index]);
    }
}

void CheckItems(TArray<FICDefinitionIssue>& Issues, const FICRowSite& Site,
    const FString& Field, const TArray<FICItemAmount>& Items, bool bRequired)
{
    IC_TRACE_FUNCTION();
    if (bRequired && Items.IsEmpty())
    {
        AddMissing(Issues, Site, Field, TEXT("At least one result item is required."));
    }
    TSet<FName> Seen;
    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        const FString ItemField = FString::Printf(TEXT("%s[%d].ItemID"), *Field, Index);
        RequireName(Issues, Site, ItemField, Items[Index].ItemID);
        if (!Items[Index].ItemID.IsNone() && Seen.Contains(Items[Index].ItemID))
        {
            AddIssue(Issues, Site, ItemField, TEXT("Combine quantities of the same item into one entry."));
        }
        Seen.Add(Items[Index].ItemID);
    }
}

bool IsPrimaryStat(EICStat Stat)
{
    IC_TRACE_FUNCTION();
    return Stat == EICStat::STR || Stat == EICStat::DEX || Stat == EICStat::INT
        || Stat == EICStat::CON || Stat == EICStat::LUK;
}

bool IsGatheringActivity(EICLifeActivity Activity)
{
    IC_TRACE_FUNCTION();
    return Activity == EICLifeActivity::Woodcutting || Activity == EICLifeActivity::Mining
        || Activity == EICLifeActivity::Gathering;
}

void CheckInstallation(TArray<FICDefinitionIssue>& Issues, const FICRowSite& Site,
    const TArray<FName>& Anchors, const TSoftClassPtr<AActor>& ActorClass, int32 Capacity,
    const TArray<FICItemAmount>& BagResults)
{
    IC_TRACE_FUNCTION();
    if (Anchors.IsEmpty())
    {
        AddMissing(Issues, Site, TEXT("InstallationAnchorIDs"), TEXT("Storage installation needs a fixed home anchor."));
    }
    CheckUniqueNames(Issues, Site, TEXT("InstallationAnchorIDs"), Anchors);
    if (ActorClass.IsNull())
    {
        AddMissing(Issues, Site, TEXT("InstalledActorClass"), TEXT("Storage installation needs its actor class."));
    }
    if (Capacity != 40)
    {
        AddIssue(Issues, Site, TEXT("StorageCapacityAdded"), TEXT("Each storage installation adds exactly 40 shared slots."));
    }
    if (!BagResults.IsEmpty())
    {
        AddIssue(Issues, Site, TEXT("Results"), TEXT("Storage installs at its anchor and must not also grant a bag item."));
    }
}

struct FICGraphEdge
{
    FName Target;
    FICRowSite Owner;
    FString Field;
};

using FICDefinitionGraph = TMap<FName, TArray<FICGraphEdge>>;

void AddGraphEdge(FICDefinitionGraph& Graph, FName From, FName To,
    const FICRowSite& Owner, const FString& Field)
{
    IC_TRACE_FUNCTION();
    if (!From.IsNone() && !To.IsNone())
    {
        TArray<FICGraphEdge>& Edges = Graph.FindOrAdd(From);
        if (!Edges.ContainsByPredicate([To](const FICGraphEdge& Edge) { IC_TRACE_FUNCTION(); return Edge.Target == To; }))
        {
            Edges.Add(FICGraphEdge{To, Owner, Field});
        }
    }
}

void CheckCycles(const FICDefinitionGraph& Graph, const TSet<FName>& Nodes,
    TArray<FICDefinitionIssue>& Issues, const FString& Kind)
{
    IC_TRACE_FUNCTION();

    TMap<FName, uint8> Colors;
    TFunction<void(FName)> Visit;
    Visit = [&](FName Node)
    {
        IC_TRACE_FUNCTION();
        Colors.Add(Node, 1);
        if (const TArray<FICGraphEdge>* Edges = Graph.Find(Node))
        {
            for (const FICGraphEdge& Edge : *Edges)
            {
                if (!Nodes.Contains(Edge.Target))
                {
                    continue;
                }
                const uint8 Color = Colors.FindRef(Edge.Target);
                if (Color == 1)
                {
                    AddIssue(Issues, Edge.Owner, Edge.Field,
                        FString::Printf(TEXT("%s dependency cycle: %s -> %s."), *Kind,
                            *Node.ToString(), *Edge.Target.ToString()));
                }
                else if (Color == 0)
                {
                    Visit(Edge.Target);
                }
            }
        }
        Colors.Add(Node, 2);
    };
    for (FName Node : Nodes)
    {
        if (Colors.FindRef(Node) == 0)
        {
            Visit(Node);
        }
    }
}

void CheckQuestObjective(const FICQuestObjectiveDefinition& Objective, const FString& Prefix,
    const FICRowSite& Site, TSet<FName>& ObjectiveIDs, TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    RequireName(Issues, Site, Prefix + TEXT(".ObjectiveID"), Objective.ObjectiveID);
    if (!Objective.ObjectiveID.IsNone() && ObjectiveIDs.Contains(Objective.ObjectiveID))
    {
        AddIssue(Issues, Site, Prefix + TEXT(".ObjectiveID"), TEXT("Objective keys must be unique across all groups in this quest."));
    }
    ObjectiveIDs.Add(Objective.ObjectiveID);
    if (Objective.bAllowCurrentState && Objective.Type != EICQuestObjectiveType::Talk
        && Objective.Type != EICQuestObjectiveType::Visit)
    {
        AddIssue(Issues, Site, Prefix + TEXT(".bAllowCurrentState"), TEXT("Only Talk and Visit can opt into state already true at acceptance."));
    }
    if (Objective.bUseEffectiveStats && Objective.Type != EICQuestObjectiveType::Stat)
    {
        AddIssue(Issues, Site, Prefix + TEXT(".bUseEffectiveStats"), TEXT("Effective-stat exceptions apply only to stat objectives."));
    }
    if (Objective.Type == EICQuestObjectiveType::Stat)
    {
        if (Objective.RequiredStat == EICStat::None)
        {
            AddMissing(Issues, Site, Prefix + TEXT(".RequiredStat"), TEXT("Choose the primary statistic required by this objective."));
        }
        else if (!IsPrimaryStat(Objective.RequiredStat))
        {
            AddIssue(Issues, Site, Prefix + TEXT(".RequiredStat"), TEXT("Stat objectives require one of the six primary stats."));
        }
    }
    else
    {
        const bool bOutputOnlyCraft = Objective.Type == EICQuestObjectiveType::Craft && !Objective.OutputItemID.IsNone();
        if (!bOutputOnlyCraft)
        {
            RequireName(Issues, Site, Prefix + TEXT(".Target.ID"), Objective.Target.ID);
        }
    }
    if (Objective.Type == EICQuestObjectiveType::Affinity
        && (Objective.RequiredValue < 0.0 || Objective.RequiredValue > 100.0))
    {
        AddIssue(Issues, Site, Prefix + TEXT(".RequiredValue"), TEXT("Current affinity thresholds must be between 0 and 100."));
    }
    if (Objective.bFilterLifeActivity && Objective.Type != EICQuestObjectiveType::Collect
        && Objective.Type != EICQuestObjectiveType::Craft)
    {
        AddIssue(Issues, Site, Prefix + TEXT(".bFilterLifeActivity"), TEXT("Life-activity filters apply to collection/crafting success events."));
    }
    if (Objective.Type == EICQuestObjectiveType::Craft && Objective.bFilterLifeActivity
        && Objective.LifeActivity != EICLifeActivity::Crafting && Objective.LifeActivity != EICLifeActivity::Cooking)
    {
        AddIssue(Issues, Site, Prefix + TEXT(".LifeActivity"), TEXT("Craft objectives must filter Crafting or Cooking."));
    }
    if (!Objective.Target.ID.IsNone())
    {
        bool bCorrectType = true;
        switch (Objective.Type)
        {
        case EICQuestObjectiveType::Talk:
        case EICQuestObjectiveType::Affinity:
            bCorrectType = Objective.Target.Type == EICDefinitionType::NPC;
            break;
        case EICQuestObjectiveType::DeliverItem:
        case EICQuestObjectiveType::Collect:
        case EICQuestObjectiveType::PossessItem:
            bCorrectType = Objective.Target.Type == EICDefinitionType::Item;
            break;
        case EICQuestObjectiveType::Defeat:
            bCorrectType = Objective.Target.Type == EICDefinitionType::Monster;
            break;
        case EICQuestObjectiveType::Craft:
            bCorrectType = Objective.Target.Type == EICDefinitionType::Recipe || Objective.Target.Type == EICDefinitionType::Item;
            break;
        case EICQuestObjectiveType::Visit:
            bCorrectType = Objective.Target.Type == EICDefinitionType::Anchor || Objective.Target.Type == EICDefinitionType::Region;
            break;
        default:
            break;
        }
        if (!bCorrectType)
        {
            AddIssue(Issues, Site, Prefix + TEXT(".Target.Type"), TEXT("The definition type does not match this objective's target."));
        }
    }
}
}

namespace ICDefinitionValidation
{
void ValidateLifeProgression(const TArray<FICDefinitionTable>& Tables, bool bComplete,
    TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();

    ForEachActiveRow<FICProductionRow>(Tables, bComplete, [&](const FICProductionRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        TSet<FName> ToolIDs;
        for (int32 Index = 0; Index < Row.Tools.Num(); ++Index)
        {
            const FString Field = FString::Printf(TEXT("Tools[%d].ItemID"), Index);
            RequireName(Issues, Site, Field, Row.Tools[Index].ItemID);
            if (!Row.Tools[Index].ItemID.IsNone() && ToolIDs.Contains(Row.Tools[Index].ItemID))
            {
                AddIssue(Issues, Site, Field, TEXT("A nonconsumed tool must be listed only once."));
            }
            ToolIDs.Add(Row.Tools[Index].ItemID);
        }
        TSet<FName> MaterialIDs;
        int32 PreconsumedMaterials = 0;
        for (int32 Index = 0; Index < Row.Materials.Num(); ++Index)
        {
            const FICProductionMaterialRequirement& Material = Row.Materials[Index];
            const FString Prefix = FString::Printf(TEXT("Materials[%d]"), Index);
            RequireName(Issues, Site, Prefix + TEXT(".Item.ItemID"), Material.Item.ItemID);
            if (!Material.Item.ItemID.IsNone() && MaterialIDs.Contains(Material.Item.ItemID))
            {
                AddIssue(Issues, Site, Prefix + TEXT(".Item.ItemID"), TEXT("Combine quantities of the same material into one entry."));
            }
            MaterialIDs.Add(Material.Item.ItemID);
            if (Material.ConsumptionTiming == EICProductionMaterialTiming::ConsumeAtStart)
            {
                ++PreconsumedMaterials;
            }
            if (Material.ConsumptionTiming == EICProductionMaterialTiming::ConsumeAtStart
                && (Row.Activity != EICLifeActivity::Fishing || Material.Item.Count != 1))
            {
                AddIssue(Issues, Site, Prefix + TEXT(".ConsumptionTiming"), TEXT("Only a single fishing bait may be consumed at start; ordinary materials are reserved until success."));
            }
        }
        if (PreconsumedMaterials > 1)
        {
            AddIssue(Issues, Site, TEXT("Materials"), TEXT("Fishing consumes one selected bait, never multiple preconsumed material entries."));
        }
        CheckItems(Issues, Site, TEXT("Results"), Row.Results, false);
        CheckUniqueNames(Issues, Site, TEXT("WorkAnchorIDs"), Row.WorkAnchorIDs);
        bool bActivityMatchesResult = false;
        switch (Row.ResultKind)
        {
        case EICProductionResultKind::BagItems:
        case EICProductionResultKind::InstallStorage:
            bActivityMatchesResult = Row.Activity == EICLifeActivity::Crafting || Row.Activity == EICLifeActivity::Cooking;
            break;
        case EICProductionResultKind::PlantCrop:
            bActivityMatchesResult = Row.Activity == EICLifeActivity::Planting;
            break;
        case EICProductionResultKind::HarvestCrop:
            bActivityMatchesResult = Row.Activity == EICLifeActivity::Harvesting;
            break;
        case EICProductionResultKind::FishingCatch:
            bActivityMatchesResult = Row.Activity == EICLifeActivity::Fishing;
            break;
        case EICProductionResultKind::GatherResource:
            bActivityMatchesResult = IsGatheringActivity(Row.Activity);
            break;
        }
        if (!bActivityMatchesResult)
        {
            AddIssue(Issues, Site, TEXT("ResultKind"), TEXT("Production result kind does not match the life activity."));
        }
        if (Row.ResultKind == EICProductionResultKind::InstallStorage)
        {
            CheckInstallation(Issues, Site, Row.InstallationAnchorIDs, Row.InstalledActorClass, Row.StorageCapacityAdded, Row.Results);
            if (Row.Activity != EICLifeActivity::Crafting)
            {
                AddIssue(Issues, Site, TEXT("Activity"), TEXT("Storage is crafted, not cooked."));
            }
        }
        if (!Row.ResultDefinition.ID.IsNone())
        {
            EICDefinitionType RequiredType = EICDefinitionType::Item;
            bool bTypedResult = true;
            switch (Row.ResultKind)
            {
            case EICProductionResultKind::PlantCrop:
            case EICProductionResultKind::HarvestCrop: RequiredType = EICDefinitionType::Crop; break;
            case EICProductionResultKind::FishingCatch: RequiredType = EICDefinitionType::FishingPoint; break;
            case EICProductionResultKind::GatherResource: RequiredType = EICDefinitionType::Resource; break;
            default: bTypedResult = false; break;
            }
            if (bTypedResult && Row.ResultDefinition.Type != RequiredType)
            {
                AddIssue(Issues, Site, TEXT("ResultDefinition.Type"), TEXT("Result definition type does not match the production result kind."));
            }
        }
    });

    ForEachActiveRow<FICRecipeRow>(Tables, bComplete, [&](const FICRecipeRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        if (Row.ResultKind == EICProductionResultKind::InstallStorage)
        {
            CheckInstallation(Issues, Site, Row.InstallationAnchorIDs, Row.InstalledActorClass, Row.StorageCapacityAdded, Row.Results);
            if (Row.Type != EPlayerRecipeType::Craft)
            {
                AddIssue(Issues, Site, TEXT("Type"), TEXT("Storage installation requires a crafting recipe."));
            }

        }
        else if (Row.ResultKind != EICProductionResultKind::BagItems)
        {
            AddIssue(Issues, Site, TEXT("ResultKind"), TEXT("Recipes produce bag items or fixed storage; farming/fishing/resource actions use production definitions."));
        }
    });

    ForEachActiveRow<FICResourceRow>(Tables, bComplete, [&](const FICResourceRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        if (!IsGatheringActivity(Row.Activity))
        {
            AddIssue(Issues, Site, TEXT("Activity"), TEXT("Resources support only Woodcutting, Mining and Gathering."));
        }
        RequireName(Issues, Site, TEXT("ProductionID"), Row.ProductionID);
        CheckItems(Issues, Site, TEXT("Yields"), Row.Yields, true);
        CheckUniqueNames(Issues, Site, TEXT("PlacementAnchorIDs"), Row.PlacementAnchorIDs);
    });

    ForEachActiveRow<FICCropRow>(Tables, bComplete, [&](const FICCropRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        RequireName(Issues, Site, TEXT("SeedItemID"), Row.SeedItemID);
        RequireName(Issues, Site, TEXT("PlantingProductionID"), Row.PlantingProductionID);
        RequireName(Issues, Site, TEXT("HarvestProductionID"), Row.HarvestProductionID);
        CheckItems(Issues, Site, TEXT("HarvestYields"), Row.HarvestYields, true);
        CheckUniqueNames(Issues, Site, TEXT("AllowedPlotAnchorIDs"), Row.AllowedPlotAnchorIDs);
        if (Row.GrowthStages.Num() < 2)
        {
            AddMissing(Issues, Site, TEXT("GrowthStages"), TEXT("A crop needs a timed growth stage followed by a permanent harvestable stage."));
        }
        TSet<FName> StageIDs;
        for (int32 Index = 0; Index < Row.GrowthStages.Num(); ++Index)
        {
            const FICCropStageDefinition& Stage = Row.GrowthStages[Index];
            const FString Prefix = FString::Printf(TEXT("GrowthStages[%d]"), Index);
            RequireName(Issues, Site, Prefix + TEXT(".StageID"), Stage.StageID);
            if (!Stage.StageID.IsNone() && StageIDs.Contains(Stage.StageID))
            {
                AddIssue(Issues, Site, Prefix + TEXT(".StageID"), TEXT("Growth stage keys must be unique within this crop."));
            }
            StageIDs.Add(Stage.StageID);
            const bool bFinal = Index == Row.GrowthStages.Num() - 1;
            if ((bFinal && Stage.DurationGameMinutes != 0.0) || (!bFinal && Stage.DurationGameMinutes <= 0.0))
            {
                AddIssue(Issues, Site, Prefix + TEXT(".DurationGameMinutes"), TEXT("Growth stages need positive game minutes; only the final permanent stage has duration zero."));
            }
            if (Stage.bHarvestable != bFinal)
            {
                AddIssue(Issues, Site, Prefix + TEXT(".bHarvestable"), TEXT("Only the final stage is harvestable and it remains until a successful harvest."));
            }
        }
    });

    TMap<FName, const FICFishRow*> FishDefinitions;
    ForEachActiveRow<FICFishRow>(Tables, bComplete, [&](const FICFishRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        FishDefinitions.Add(Row.ID, &Row);
        RequireName(Issues, Site, TEXT("ItemID"), Row.ItemID);
        RequireName(Issues, Site, TEXT("RegionID"), Row.RegionID);
        CheckUniqueNames(Issues, Site, TEXT("AllowedFishingPointIDs"), Row.AllowedFishingPointIDs);
        CheckUniqueNames(Issues, Site, TEXT("AcceptedBaitItemIDs"), Row.AcceptedBaitItemIDs);
    });

    ForEachActiveRow<FICFishingPointRow>(Tables, bComplete, [&](const FICFishingPointRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        RequireName(Issues, Site, TEXT("RegionID"), Row.RegionID);
        RequireName(Issues, Site, TEXT("AnchorID"), Row.AnchorID);
        RequireName(Issues, Site, TEXT("ProductionID"), Row.ProductionID);
        RequireName(Issues, Site, TEXT("FishingRodItemID"), Row.FishingRodItemID);
        if (Row.AcceptedBaitItemIDs.IsEmpty())
        {
            AddMissing(Issues, Site, TEXT("AcceptedBaitItemIDs"), TEXT("A fishing point must accept at least one bait item."));
        }
        CheckUniqueNames(Issues, Site, TEXT("AcceptedBaitItemIDs"), Row.AcceptedBaitItemIDs);
        if (Row.FishSelections.IsEmpty())
        {
            AddMissing(Issues, Site, TEXT("FishSelections"), TEXT("A fishing point needs at least one catchable species."));
        }
        bool bHasPositiveWeight = false;
        for (int32 Index = 0; Index < Row.FishSelections.Num(); ++Index)
        {
            const FICFishingSelectionEntry& Selection = Row.FishSelections[Index];
            const FString Prefix = FString::Printf(TEXT("FishSelections[%d]"), Index);
            RequireName(Issues, Site, Prefix + TEXT(".FishID"), Selection.FishID);
            bHasPositiveWeight |= Selection.Weight > 0.0;
            if (const FICFishRow* const* FoundFish = FishDefinitions.Find(Selection.FishID))
            {
                if (!(*FoundFish)->RegionID.IsNone() && !Row.RegionID.IsNone() && (*FoundFish)->RegionID != Row.RegionID)
                {
                    AddIssue(Issues, Site, Prefix + TEXT(".FishID"), TEXT("The species and fishing point must belong to the same region."));
                }
                if (!(*FoundFish)->AllowedFishingPointIDs.IsEmpty() && !(*FoundFish)->AllowedFishingPointIDs.Contains(Row.ID))
                {
                    AddIssue(Issues, Site, Prefix + TEXT(".FishID"), TEXT("The species explicitly excludes this fishing point."));
                }
            }
            CheckUniqueNames(Issues, Site, Prefix + TEXT(".AcceptedBaitItemIDs"), Selection.AcceptedBaitItemIDs);
            for (int32 BaitIndex = 0; BaitIndex < Selection.AcceptedBaitItemIDs.Num(); ++BaitIndex)
            {
                if (!Row.AcceptedBaitItemIDs.Contains(Selection.AcceptedBaitItemIDs[BaitIndex]))
                {
                    AddIssue(Issues, Site, FString::Printf(TEXT("%s.AcceptedBaitItemIDs[%d]"), *Prefix, BaitIndex),
                        TEXT("Entry bait restrictions must be among this fishing point's accepted bait."));
                }
            }
        }
        if (!Row.FishSelections.IsEmpty() && !bHasPositiveWeight)
        {
            AddIssue(Issues, Site, TEXT("FishSelections"), TEXT("The species-selection table needs positive total weight."));
        }
        if (!Row.FishSelections.IsEmpty())
        {
            for (int32 BaitIndex = 0; BaitIndex < Row.AcceptedBaitItemIDs.Num(); ++BaitIndex)
            {
                const FName BaitID = Row.AcceptedBaitItemIDs[BaitIndex];
                if (BaitID.IsNone()) continue;
                bool bHasEligibleFish = false;
                bool bHasUnknownCandidate = false;
                for (const FICFishingSelectionEntry& Selection : Row.FishSelections)
                {
                    if (Selection.Weight <= 0.0 || Selection.FishID.IsNone()
                        || (!Selection.AcceptedBaitItemIDs.IsEmpty() && !Selection.AcceptedBaitItemIDs.Contains(BaitID)))
                        continue;
                    const FICFishRow* const* FoundFish = FishDefinitions.Find(Selection.FishID);
                    if (!FoundFish)
                    {
                        bHasUnknownCandidate = true;
                        continue;
                    }
                    const FICFishRow& Fish = **FoundFish;
                    if ((!Fish.AcceptedBaitItemIDs.IsEmpty() && !Fish.AcceptedBaitItemIDs.Contains(BaitID))
                        || (!Fish.AllowedFishingPointIDs.IsEmpty() && !Fish.AllowedFishingPointIDs.Contains(Row.ID))
                        || (!Fish.RegionID.IsNone() && !Row.RegionID.IsNone() && Fish.RegionID != Row.RegionID))
                        continue;
                    bHasEligibleFish = true;
                    break;
                }
                if (!bHasEligibleFish && !bHasUnknownCandidate)
                {
                    AddIssue(Issues, Site, FString::Printf(TEXT("AcceptedBaitItemIDs[%d]"), BaitIndex),
                        TEXT("This accepted bait has no eligible species with positive weight after point/entry/species filtering."));
                }
            }
        }
    });

    FICDefinitionGraph QuestGraph;
    TSet<FName> QuestIDs;
    TMap<FName, const FICQuestRow*> Quests;
    ForEachActiveRow<FICQuestRow>(Tables, bComplete, [&](const FICQuestRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        if (!Row.ID.IsNone())
        {
            QuestIDs.Add(Row.ID);
            Quests.Add(Row.ID, &Row);
        }
        if (Row.QuestType != EICQuestType::General && (Row.bAllowAbandon || Row.bRepeatable))
        {
            AddIssue(Issues, Site, Row.bAllowAbandon ? TEXT("bAllowAbandon") : TEXT("bRepeatable"),
                TEXT("Main and chain quests are one-time quests and cannot be abandoned or repeated."));
        }
        if (Row.CompletionMode == EICQuestCompletionMode::NPCInteraction)
        {
            RequireName(Issues, Site, TEXT("TurnInNPCID"), Row.TurnInNPCID);
        }
        if (Row.ObjectiveGroups.IsEmpty())
        {
            AddMissing(Issues, Site, TEXT("ObjectiveGroups"), TEXT("An authored quest needs at least one objective group."));
        }
        TSet<FName> ObjectiveIDs;
        for (int32 GroupIndex = 0; GroupIndex < Row.ObjectiveGroups.Num(); ++GroupIndex)
        {
            const FICQuestObjectiveGroup& Group = Row.ObjectiveGroups[GroupIndex];
            const FString GroupField = FString::Printf(TEXT("ObjectiveGroups[%d]"), GroupIndex);
            if (Group.Objectives.IsEmpty())
            {
                AddMissing(Issues, Site, GroupField + TEXT(".Objectives"), TEXT("AND/OR groups must contain at least one objective."));
            }
            for (int32 Index = 0; Index < Group.Objectives.Num(); ++Index)
            {
                CheckQuestObjective(Group.Objectives[Index], FString::Printf(TEXT("%s.Objectives[%d]"), *GroupField, Index),
                    Site, ObjectiveIDs, Issues);
            }
        }
        CheckItems(Issues, Site, TEXT("AcceptanceQuestItems"), Row.AcceptanceQuestItems, false);
        CheckUniqueNames(Issues, Site, TEXT("PrerequisiteQuestIDs"), Row.PrerequisiteQuestIDs);
        CheckUniqueNames(Issues, Site, TEXT("SuccessorQuestIDs"), Row.SuccessorQuestIDs);
        for (int32 Index = 0; Index < Row.PrerequisiteQuestIDs.Num(); ++Index)
        {

            AddGraphEdge(QuestGraph, Row.PrerequisiteQuestIDs[Index], Row.ID, Site,
                FString::Printf(TEXT("PrerequisiteQuestIDs[%d]"), Index));
        }
        for (int32 Index = 0; Index < Row.SuccessorQuestIDs.Num(); ++Index)
        {
            AddGraphEdge(QuestGraph, Row.ID, Row.SuccessorQuestIDs[Index], Site,
                FString::Printf(TEXT("SuccessorQuestIDs[%d]"), Index));
        }
    });
    CheckCycles(QuestGraph, QuestIDs, Issues, TEXT("Quest"));

    TMap<FName, const FICProgressionRow*> Progressions;
    ForEachActiveRow<FICProgressionRow>(Tables, bComplete, [&](const FICProgressionRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        Progressions.Add(Row.ID, &Row);
        RequireName(Issues, Site, TEXT("InitialStageID"), Row.InitialStageID);
        RequireName(Issues, Site, TEXT("FinalBossID"), Row.FinalBossID);
        RequireName(Issues, Site, TEXT("EndingQuestID"), Row.EndingQuestID);
        if (Row.Stages.IsEmpty())
        {
            AddMissing(Issues, Site, TEXT("Stages"), TEXT("An authored progression definition needs its main-progress stages."));
        }
        TSet<FName> StageIDs;
        FICDefinitionGraph StageGraph;
        for (int32 Index = 0; Index < Row.Stages.Num(); ++Index)
        {
            const FICProgressionStageDefinition& Stage = Row.Stages[Index];
            const FString Prefix = FString::Printf(TEXT("Stages[%d]"), Index);
            RequireName(Issues, Site, Prefix + TEXT(".StageID"), Stage.StageID);
            if (!Stage.StageID.IsNone() && StageIDs.Contains(Stage.StageID))
            {
                AddIssue(Issues, Site, Prefix + TEXT(".StageID"), TEXT("Progression stage keys must be unique within this definition."));
            }
            StageIDs.Add(Stage.StageID);
            CheckUniqueNames(Issues, Site, Prefix + TEXT(".QuestIDs"), Stage.QuestIDs);
            CheckUniqueNames(Issues, Site, Prefix + TEXT(".NextStageIDs"), Stage.NextStageIDs);
            for (int32 NextIndex = 0; NextIndex < Stage.NextStageIDs.Num(); ++NextIndex)
            {
                AddGraphEdge(StageGraph, Stage.StageID, Stage.NextStageIDs[NextIndex], Site,
                    FString::Printf(TEXT("%s.NextStageIDs[%d]"), *Prefix, NextIndex));
            }
        }
        if (!Row.InitialStageID.IsNone() && !StageIDs.Contains(Row.InitialStageID))
        {
            AddIssue(Issues, Site, TEXT("InitialStageID"), TEXT("The initial stage must be defined inside this progression row."));
        }
        for (const TPair<FName, TArray<FICGraphEdge>>& Pair : StageGraph)
        {
            for (const FICGraphEdge& Edge : Pair.Value)
            {
                if (!StageIDs.Contains(Edge.Target))
                {
                    AddIssue(Issues, Site, Edge.Field, TEXT("The next stage is not defined inside this progression row."));
                }
            }
        }
        CheckCycles(StageGraph, StageIDs, Issues, TEXT("Progression stage"));
    });

    TMap<FName, const FICNPCRow*> NPCs;
    TMap<FName, FICRowSite> NPCSites;
    ForEachActiveRow<FICNPCRow>(Tables, bComplete, [&](const FICNPCRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        NPCs.Add(Row.ID, &Row);
        NPCSites.Add(Row.ID, Site);
    });
    TSet<FName> AllEndingTargets;
    int32 EndingCount = 0;
    ForEachActiveRow<FICEndingRow>(Tables, bComplete, [&](const FICEndingRow& Row, const FICRowSite& Site)
    {
        IC_TRACE_FUNCTION();
        ++EndingCount;
        RequireName(Issues, Site, TEXT("FinalBossID"), Row.FinalBossID);
        RequireName(Issues, Site, TEXT("EndingQuestID"), Row.EndingQuestID);
        RequireName(Issues, Site, TEXT("ProgressionID"), Row.ProgressionID);
        RequireName(Issues, Site, TEXT("WorldReturnAnchorID"), Row.WorldReturnAnchorID);
        if (Row.HiddenAffinityNPCIDs.Num() != 10)
        {
            if (Row.HiddenAffinityNPCIDs.Num() < 10)
                AddMissing(Issues, Site, TEXT("HiddenAffinityNPCIDs"), TEXT("Hidden-ending criteria must name exactly ten distinct affinity-enabled NPCs."));
            else
                AddIssue(Issues, Site, TEXT("HiddenAffinityNPCIDs"), TEXT("Hidden-ending criteria must not contain more than ten NPCs."));
        }
        CheckUniqueNames(Issues, Site, TEXT("HiddenAffinityNPCIDs"), Row.HiddenAffinityNPCIDs);
        for (int32 Index = 0; Index < Row.HiddenAffinityNPCIDs.Num(); ++Index)
        {
            const FName NPCID = Row.HiddenAffinityNPCIDs[Index];
            AllEndingTargets.Add(NPCID);
            if (const FICNPCRow* const* FoundNPC = NPCs.Find(NPCID))
            {
                const FString Field = FString::Printf(TEXT("HiddenAffinityNPCIDs[%d]"), Index);
                if ((*FoundNPC)->Type == EICNPCType::Background)
                {
                    AddIssue(Issues, Site, Field, TEXT("Background NPCs have no affinity and cannot be hidden-ending targets."));
                }
                if (!(*FoundNPC)->bEndingTarget)
                {
                    AddIssue(Issues, Site, Field, TEXT("This NPC must also declare bEndingTarget in its NPC definition."));
                }
            }
        }
        if (const FICQuestRow* const* FoundQuest = Quests.Find(Row.EndingQuestID))
        {
            if (!(*FoundQuest)->bEndingQuest)
            {
                AddIssue(Issues, Site, TEXT("EndingQuestID"), TEXT("The referenced quest must declare the ending reward policy."));
            }
        }
        if (const FICProgressionRow* const* FoundProgression = Progressions.Find(Row.ProgressionID))
        {
            if ((!(*FoundProgression)->FinalBossID.IsNone() && !Row.FinalBossID.IsNone() && (*FoundProgression)->FinalBossID != Row.FinalBossID)
                || (!(*FoundProgression)->EndingQuestID.IsNone() && !Row.EndingQuestID.IsNone() && (*FoundProgression)->EndingQuestID != Row.EndingQuestID))
            {
                AddIssue(Issues, Site, TEXT("ProgressionID"), TEXT("Progression and ending definitions must agree on the final boss and ending quest."));
            }
        }
    });
    if (EndingCount > 0)
    {
        for (const TPair<FName, const FICNPCRow*>& Pair : NPCs)
        {
            if (Pair.Value->bEndingTarget && !AllEndingTargets.Contains(Pair.Key))
            {
                AddMissing(Issues, NPCSites.FindChecked(Pair.Key), TEXT("bEndingTarget"),
                    TEXT("This NPC is marked as an ending target but is absent from the active ending definitions."));
            }
        }
    }
}
}
