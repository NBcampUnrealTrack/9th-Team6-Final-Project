#include "ICQuestObjectiveEvaluator.h"

#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Engine/World.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "World/State/ICWorldStateSubsystem.h"

bool FICQuestObjectiveEvaluator::CompareValues(double Value, double RequiredValue, EICComparison Comparison)
{
    switch (Comparison)
    {
    case EICComparison::Equal: return Value == RequiredValue;
    case EICComparison::NotEqual: return Value != RequiredValue;
    case EICComparison::Greater: return Value > RequiredValue;
    case EICComparison::GreaterOrEqual: return Value >= RequiredValue;
    case EICComparison::Less: return Value < RequiredValue;
    case EICComparison::LessOrEqual: return Value <= RequiredValue;
    default: return false;
    }
}

int32 FICQuestObjectiveEvaluator::CountOwnedItem(const FICInventoryState& Inventory, FName ItemID)
{
    int64 Count = 0;
    for (const FPlayerItemInstance& Item : Inventory.Bag.Slots)
        if (Item.ItemID == ItemID) Count += Item.Count;
    for (const FPlayerItemInstance& Item : Inventory.QuestItems)
        if (Item.ItemID == ItemID) Count += Item.Count;
    for (const FPlayerItemInstance& Item : Inventory.Equipment)
        if (Item.ItemID == ItemID) Count += Item.Count;
    for (const FPlayerItemInstance& Item : Inventory.WeaponSlots)
        if (Item.ItemID == ItemID) Count += Item.Count;
    return static_cast<int32>(FMath::Min<int64>(Count, MAX_int32));
}

bool FICQuestObjectiveEvaluator::IsComplete(const FICQuestObjectiveDefinition& Objective,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    switch (Objective.Type)
    {
    case EICQuestObjectiveType::PossessItem:
        return CountOwnedItem(World.Inventory, Objective.Target.ID) >= Objective.RequiredCount;
    case EICQuestObjectiveType::Affinity:
        if (const FICCharacterState* Character = World.Characters.Find(Objective.Target.ID))
            return CompareValues(Character->Affinity, Objective.RequiredValue, Objective.Compare);
        return false;
    case EICQuestObjectiveType::Stat:
    {
        const FICCharacterState* Player = World.Characters.Find(UICWorldStateSubsystem::PlayerId());
        if (!Player) return false;
        double Value = 0;
        if (Objective.bUseEffectiveStats)
        {
            const AICPlayerController* Controller = ContextWorld
                ? Cast<AICPlayerController>(ContextWorld->GetFirstPlayerController()) : nullptr;
            const AICPlayerCharacter* Character = Controller
                ? Cast<AICPlayerCharacter>(Controller->GetPawn()) : nullptr;
            const UICAbilitySystemComponent* Ability = Character ? Character->GetICAbilitySystem() : nullptr;
            if (!Ability) return false;
            Value = Ability->Stat(Objective.RequiredStat);
        }
        else
        {
            switch (Objective.RequiredStat)
            {
            case EICStat::STR: Value = Player->MainStats.STR; break;
            case EICStat::DEX: Value = Player->MainStats.DEX; break;
            case EICStat::INT: Value = Player->MainStats.INT; break;
            case EICStat::CON: Value = Player->MainStats.CON; break;
            case EICStat::CHA: return false; // Serialized legacy stat; no gameplay conditions.
            case EICStat::LUK: Value = Player->MainStats.LUK; break;
            default: return false;
            }
        }
        return CompareValues(Value, Objective.RequiredValue, Objective.Compare);
    }
    case EICQuestObjectiveType::Visit:
        if (Objective.bAllowCurrentState && Objective.RequiredCount == 1)
        {
            if (Objective.Target.Type == EICDefinitionType::Anchor
                && World.DiscoveredAnchorIDs.Contains(Objective.Target.ID)) return true;
            if (Objective.Target.Type == EICDefinitionType::Region
                && World.DiscoveredRegionIDs.Contains(Objective.Target.ID)) return true;
        }
        return Quest.Objectives.FindRef(Objective.ObjectiveID) >= Objective.RequiredCount;
    default:
        return Quest.Objectives.FindRef(Objective.ObjectiveID) >= Objective.RequiredCount;
    }
}

bool FICQuestObjectiveEvaluator::GetProgress(const FICQuestObjectiveDefinition& Objective,
    const FICQuestState& Quest, const FICWorldState& World, int32& CurrentCount) const
{
    if (Objective.Type == EICQuestObjectiveType::Affinity || Objective.Type == EICQuestObjectiveType::Stat)
        return false;

    int64 Progress = Quest.Objectives.FindRef(Objective.ObjectiveID);
    if (Objective.Type == EICQuestObjectiveType::PossessItem)
        Progress = CountOwnedItem(World.Inventory, Objective.Target.ID);
    else if (Objective.Type == EICQuestObjectiveType::DeliverItem)
        Progress += CountOwnedItem(World.Inventory, Objective.Target.ID);
    else if (Objective.Type == EICQuestObjectiveType::Visit && IsComplete(Objective, Quest, World))
        Progress = Objective.RequiredCount;

    CurrentCount = static_cast<int32>(FMath::Clamp<int64>(Progress, 0, Objective.RequiredCount));
    return true;
}

bool FICQuestObjectiveEvaluator::IsGroupComplete(const FICQuestObjectiveGroup& Group,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    if (Group.Objectives.IsEmpty()) return false;
    bool bComplete = Group.Policy == EICConditionPolicy::All;
    for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
    {
        const bool bObjectiveComplete = IsComplete(Objective, Quest, World);
        bComplete = Group.Policy == EICConditionPolicy::All
            ? bComplete && bObjectiveComplete : bComplete || bObjectiveComplete;
    }
    return bComplete;
}

bool FICQuestObjectiveEvaluator::AreObjectivesComplete(const FICQuestRow& Definition,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    if (Definition.ObjectiveGroups.IsEmpty()) return false;
    bool bComplete = Definition.ObjectiveGroupPolicy == EICConditionPolicy::All;
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        if (Group.Objectives.IsEmpty()) return false;
        const bool bGroupComplete = IsGroupComplete(Group, Quest, World);
        bComplete = Definition.ObjectiveGroupPolicy == EICConditionPolicy::All
            ? bComplete && bGroupComplete : bComplete || bGroupComplete;
    }
    return bComplete;
}
