#include "Quest/ICQuestSubsystem.h"

#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/Rewards/ICRewardRows.h"
#include "Debug/ICLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Subsystems/SubsystemCollection.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

void UICQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ConfigureServices(Collection.InitializeDependency<UICDefinitionSubsystem>(),
        Collection.InitializeDependency<UICWorldStateSubsystem>(),
        Collection.InitializeDependency<UICTimeSubsystem>());
}

void UICQuestSubsystem::Deinitialize()
{
    ConfigureServices(nullptr, nullptr, nullptr);
    Super::Deinitialize();
}

void UICQuestSubsystem::ConfigureServices(UICDefinitionSubsystem* InDefinitions,
    UICWorldStateSubsystem* InState, UICTimeSubsystem* InTime)
{
    if (State)
    {
        State->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleInventoryChanged);
        State->OnWorldRestored.RemoveDynamic(this, &ThisClass::HandleWorldRestored);
        State->OnCharacterChanged.RemoveDynamic(this, &ThisClass::HandleCharacterChanged);
    }
    Definitions = InDefinitions;
    State = InState;
    Time = InTime;
    IsBaselineReady = false;
    if (State)
    {
        State->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::HandleInventoryChanged);
        State->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::HandleWorldRestored);
        State->OnCharacterChanged.AddUniqueDynamic(this, &ThisClass::HandleCharacterChanged);
        InventoryBaseline = State->GetInventorySnapshot();
        IsBaselineReady = true;
    }
}

bool UICQuestSubsystem::CompareValues(double Value, double RequiredValue, EICComparison Comparison)
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

int32 UICQuestSubsystem::CountOwnedItem(const FICInventoryState& Inventory, FName ItemID) const
{
    int64 Count = 0;
    for (const FPlayerItemInstance& Item : Inventory.Bag.Slots)
    {
        if (Item.ItemID == ItemID) Count += Item.Count;
    }
    for (const FPlayerItemInstance& Item : Inventory.QuestItems)
    {
        if (Item.ItemID == ItemID) Count += Item.Count;
    }
    for (const FPlayerItemInstance& Item : Inventory.Equipment)
    {
        if (Item.ItemID == ItemID) Count += Item.Count;
    }
    return static_cast<int32>(FMath::Min<int64>(Count, MAX_int32));
}

bool UICQuestSubsystem::CanAccept(const FICQuestRow& Definition, const FICWorldState& World) const
{
    if (!World.bPlayerCreated)
    {
        return false;
    }
    if (const FICQuestState* Existing = World.Quests.Find(Definition.ID))
    {
        if ((Existing->bAccepted && !Existing->bCompleted)
            || (Existing->bCompleted && !Definition.bRepeatable))
        {
            return false;
        }
    }
    for (FName PrerequisiteID : Definition.PrerequisiteQuestIDs)
    {
        const FICQuestState* Prerequisite = World.Quests.Find(PrerequisiteID);
        if (!Prerequisite || !Prerequisite->bCompleted)
        {
            return false;
        }
    }
    if (Definition.AcceptanceConditions.IsEmpty())
    {
        return true;
    }
    const AICPlayerController* Controller = GetWorld()
        ? Cast<AICPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
    const AICPlayerCharacter* Player = Controller
        ? Cast<AICPlayerCharacter>(Controller->GetPawn()) : nullptr;
    const UICAbilitySystemComponent* Ability = Player ? Player->GetICAbilitySystem() : nullptr;
    if (!Ability)
    {
        return false;
    }
    for (const FICConditionGroup& Group : Definition.AcceptanceConditions)
    {
        if (!Ability->Conditions(Group))
        {
            return false;
        }
    }
    return true;
}

bool UICQuestSubsystem::AcceptQuest(FName QuestID)
{
    if (!Definitions || !State || !Time)
    {
        return false;
    }
    FICQuestRow Definition;
    if (!Definitions->GetQuest(QuestID, Definition).bSuccess)
    {
        return false;
    }
    FICWorldState Candidate = State->ExportState();
    if (!CanAccept(Definition, Candidate))
    {
        return false;
    }
    if (!Definition.AcceptanceQuestItems.IsEmpty())
    {
        UICInventoryComponent* Staging = NewObject<UICInventoryComponent>(this);
        Staging->BeginStaging(Definitions, Candidate.Job);
        if (!Staging->ApplySnapshot(Candidate.Inventory))
        {
            return false;
        }
        for (const FICItemAmount& Item : Definition.AcceptanceQuestItems)
        {
            FICItemRow ItemDefinition;
            if (!Definitions->GetItem(Item.ItemID, ItemDefinition).bSuccess
                || ItemDefinition.Kind != EPlayerItemKind::Quest
                || !Staging->AddItem(Item.ItemID, Item.Count, false))
            {
                return false;
            }
        }
        Candidate.Inventory = Staging->GetSnapshot();
    }
    FICQuestState& Quest = Candidate.Quests.FindOrAdd(QuestID);
    Quest.bAccepted = true;
    Quest.bCompleted = false;
    Quest.bRewardClaimed = false;
    Quest.RewardTransactionID.Invalidate();
    Quest.Objectives.Reset();
    Quest.AcceptedAtGameMinute = Time->GetClock().GameMinute;
    if (!State->CommitGameplayTransaction(Candidate, FGuid::NewGuid()))
    {
        return false;
    }
    UE_LOG(LogICWorldState, Log, TEXT("[Quest.Accepted] Quest=%s"), *QuestID.ToString());
    if (!IsProcessing)
    {
        EvaluateQuests();
    }
    return true;
}

bool UICQuestSubsystem::IsObjectiveComplete(const FICQuestObjectiveDefinition& Objective,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    switch (Objective.Type)
    {
    case EICQuestObjectiveType::PossessItem:
        return CountOwnedItem(World.Inventory, Objective.Target.ID) >= Objective.RequiredCount;
    case EICQuestObjectiveType::Affinity:
        if (const FICCharacterState* Character = World.Characters.Find(Objective.Target.ID))
        {
            return CompareValues(Character->Affinity, Objective.RequiredValue, Objective.Compare);
        }
        return false;
    case EICQuestObjectiveType::Stat:
    {
        const FICCharacterState* Player = World.Characters.Find(UICWorldStateSubsystem::PlayerId());
        if (!Player)
        {
            return false;
        }
        double Value = 0;
        if (Objective.bUseEffectiveStats)
        {
            const AICPlayerController* Controller = GetWorld()
                ? Cast<AICPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
            const AICPlayerCharacter* Character = Controller
                ? Cast<AICPlayerCharacter>(Controller->GetPawn()) : nullptr;
            const UICAbilitySystemComponent* Ability = Character ? Character->GetICAbilitySystem() : nullptr;
            if (!Ability)
            {
                return false;
            }
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
                && World.DiscoveredAnchorIDs.Contains(Objective.Target.ID))
            {
                return true;
            }
            if (Objective.Target.Type == EICDefinitionType::Region
                && World.DiscoveredRegionIDs.Contains(Objective.Target.ID))
            {
                return true;
            }
        }
        return Quest.Objectives.FindRef(Objective.ObjectiveID) >= Objective.RequiredCount;
    default:
        return Quest.Objectives.FindRef(Objective.ObjectiveID) >= Objective.RequiredCount;
    }
}

bool UICQuestSubsystem::GetObjectiveProgress(const FICQuestObjectiveDefinition& Objective,
    const FICQuestState& Quest, const FICWorldState& World, int32& CurrentCount) const
{
    if (Objective.Type == EICQuestObjectiveType::Affinity || Objective.Type == EICQuestObjectiveType::Stat)
    {
        return false;
    }

    int64 Progress = Quest.Objectives.FindRef(Objective.ObjectiveID);
    if (Objective.Type == EICQuestObjectiveType::PossessItem)
    {
        Progress = CountOwnedItem(World.Inventory, Objective.Target.ID);
    }
    else if (Objective.Type == EICQuestObjectiveType::DeliverItem)
    {
        Progress += CountOwnedItem(World.Inventory, Objective.Target.ID);
    }
    else if (Objective.Type == EICQuestObjectiveType::Visit && IsObjectiveComplete(Objective, Quest, World))
    {
        Progress = Objective.RequiredCount;
    }

    CurrentCount = static_cast<int32>(FMath::Clamp<int64>(Progress, 0, Objective.RequiredCount));
    return true;
}

bool UICQuestSubsystem::AreObjectivesComplete(const FICQuestRow& Definition,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    if (Definition.ObjectiveGroups.IsEmpty())
    {
        return false;
    }
    bool IsComplete = Definition.ObjectiveGroupPolicy == EICConditionPolicy::All;
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        if (Group.Objectives.IsEmpty())
        {
            return false;
        }
        bool IsGroupComplete = Group.Policy == EICConditionPolicy::All;
        for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
        {
            const bool IsObjectiveReady = IsObjectiveComplete(Objective, Quest, World);
            if (Group.Policy == EICConditionPolicy::All)
            {
                IsGroupComplete &= IsObjectiveReady;
            }
            else
            {
                IsGroupComplete |= IsObjectiveReady;
            }
        }
        if (Definition.ObjectiveGroupPolicy == EICConditionPolicy::All)
        {
            IsComplete &= IsGroupComplete;
        }
        else
        {
            IsComplete |= IsGroupComplete;
        }
    }
    return IsComplete;
}

bool UICQuestSubsystem::PrepareDeliveryObjectives(const FICQuestRow& Definition,
    FICWorldState& World) const
{
    FICQuestState* Quest = World.Quests.Find(Definition.ID);
    if (!Quest || !Quest->bAccepted || Quest->bCompleted)
    {
        return false;
    }
    if (AreObjectivesComplete(Definition, *Quest, World))
    {
        return true;
    }

    UICInventoryComponent* Staging = NewObject<UICInventoryComponent>();
    Staging->BeginStaging(Definitions, World.Job);
    if (!Staging->ApplySnapshot(World.Inventory))
    {
        return false;
    }
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
        {
            if (Objective.Type != EICQuestObjectiveType::DeliverItem)
            {
                continue;
            }
            const int32 Remaining = Objective.RequiredCount - Quest->Objectives.FindRef(Objective.ObjectiveID);
            if (Remaining <= 0)
            {
                continue;
            }
            FICInventoryState UpdatedInventory;
            if (!Staging->BuildQuestItemConsumption(Objective.Target.ID, Remaining, UpdatedInventory)
                || !Staging->ApplySnapshot(UpdatedInventory))
            {
                return false;
            }
            Quest->Objectives.Add(Objective.ObjectiveID, Objective.RequiredCount);
        }
    }
    World.Inventory = Staging->GetSnapshot();
    return AreObjectivesComplete(Definition, *Quest, World);
}

bool UICQuestSubsystem::ApplyReward(FICWorldState& Candidate, const FICRewardRow& Reward)
{
    if (!Definitions || Reward.Overflow != EICRewardOverflow::RejectAll || Reward.Gold < 0
        || static_cast<int64>(Candidate.Gold) + Reward.Gold > MAX_int32)
    {
        return false;
    }
    UICInventoryComponent* Staging = NewObject<UICInventoryComponent>(this);
    Staging->BeginStaging(Definitions, Candidate.Job);
    if (!Staging->ApplySnapshot(Candidate.Inventory))
    {
        return false;
    }
    for (const FICItemAmount& Item : Reward.Items)
    {
        if (!Staging->AddItem(Item.ItemID, Item.Count, false))
        {
            return false;
        }
    }
    Candidate.Inventory = Staging->GetSnapshot();
    Candidate.Gold += Reward.Gold;
    for (const FICAffinityReward& Affinity : Reward.Affinity)
    {
        FICCharacterState* Character = Candidate.Characters.Find(Affinity.NPCID);
        if (!Character || Character->Kind == EICCharacterKind::Background)
        {
            return false;
        }
        Character->Affinity = static_cast<int32>(FMath::Clamp<int64>(
            static_cast<int64>(Character->Affinity) + Affinity.Amount, 0, 500));
    }
    for (FName RecipeID : Reward.UnlockRecipeIDs)
    {
        FICRecipeRow Recipe;
        if (!Definitions->GetRecipe(RecipeID, Recipe).bSuccess)
        {
            return false;
        }
        Candidate.UnlockedRecipeIDs.Add(RecipeID);
    }
    return true;
}

bool UICQuestSubsystem::CompleteQuest(FName QuestID, const FICQuestRow& Definition,
    bool IsForced, const FICWorldState* PreparedWorld)
{
    if (!Definitions || !State || !Time)
    {
        return false;
    }
    FICWorldState Candidate = PreparedWorld ? *PreparedWorld : State->ExportState();
    FICQuestState* Quest = Candidate.Quests.Find(QuestID);
    if (!Quest || !Quest->bAccepted || Quest->bCompleted
        || (!IsForced && !AreObjectivesComplete(Definition, *Quest, Candidate)))
    {
        return false;
    }
    bool IsRewardApplied = true;
    if (!Definition.RewardID.IsNone())
    {
        FICRewardRow Reward;
        if (!Definitions->GetReward(Definition.RewardID, Reward).bSuccess
            || !ApplyReward(Candidate, Reward))
        {
            UE_LOG(LogICWorldState, Warning, TEXT("[Quest.RewardRejected] Quest=%s Reward=%s"),
                *QuestID.ToString(), *Definition.RewardID.ToString());
            if (!IsForced)
            {
                return false;
            }
            Candidate = State->ExportState();
            IsRewardApplied = false;
        }
    }
    Quest = Candidate.Quests.Find(QuestID);
    const FGuid TransactionID = FGuid::NewGuid();
    Quest->bCompleted = true;
    Quest->bRewardClaimed = IsRewardApplied;
    Quest->RewardTransactionID = IsRewardApplied ? TransactionID : FGuid();
    Quest->CompletionCount++;
    Quest->CompletedAtGameMinute = Time->GetClock().GameMinute;
    if (!State->CommitGameplayTransaction(Candidate, TransactionID))
    {
        return false;
    }
    UE_LOG(LogICWorldState, Log, TEXT("[Quest.Completed] Quest=%s Reward=%s"),
        *QuestID.ToString(), *Definition.RewardID.ToString());
    return true;
}

int32 UICQuestSubsystem::ForceCompleteActiveQuests()
{
    if (!Definitions || !State || !Time)
    {
        return 0;
    }

    const FICWorldState World = State->ExportState();
    int32 CompletedCount = 0;
    for (const auto& Entry : World.Quests)
    {
        if (!Entry.Value.bAccepted || Entry.Value.bCompleted)
        {
            continue;
        }

        FICQuestRow Definition;
        if (Definitions->GetQuest(Entry.Key, Definition).bSuccess
            && CompleteQuest(Entry.Key, Definition, true))
        {
            ++CompletedCount;
        }
    }
    return CompletedCount;
}

bool UICQuestSubsystem::TurnInQuest(FName QuestID, FName NPCID)
{
    if (!Definitions || !State)
    {
        return false;
    }
    FICQuestRow Definition;
    if (NPCID.IsNone() || !Definitions->GetQuest(QuestID, Definition).bSuccess
        || Definition.CompletionMode != EICQuestCompletionMode::NPCInteraction
        || Definition.TurnInNPCID != NPCID)
    {
        return false;
    }
    FICWorldState Candidate = State->ExportState();
    return PrepareDeliveryObjectives(Definition, Candidate)
        && CompleteQuest(QuestID, Definition, false, &Candidate);
}

bool UICQuestSubsystem::CanOfferQuest(FName QuestID) const
{
    if (!Definitions || !State) return false;
    FICQuestRow Definition;
    return Definitions->GetQuest(QuestID, Definition).bSuccess
        && !Definition.IsAutoAccept && !Definition.OfferNPCID.IsNone()
        && CanAccept(Definition, State->ExportState());
}

bool UICQuestSubsystem::AreQuestObjectivesReady(FName QuestID) const
{
    if (!Definitions || !State) return false;
    FICQuestRow Definition;
    if (!Definitions->GetQuest(QuestID, Definition).bSuccess) return false;
    FICWorldState World = State->ExportState();
    return PrepareDeliveryObjectives(Definition, World);
}

bool UICQuestSubsystem::DeliverItem(FName QuestID, FName ObjectiveID)
{
    if (!Definitions || !State || !Time)
    {
        return false;
    }
    FICQuestRow Definition;
    if (!Definitions->GetQuest(QuestID, Definition).bSuccess)
    {
        return false;
    }
    const FICQuestObjectiveDefinition* Objective = nullptr;
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        for (const FICQuestObjectiveDefinition& Entry : Group.Objectives)
        {
            if (Entry.ObjectiveID == ObjectiveID && Entry.Type == EICQuestObjectiveType::DeliverItem)
            {
                Objective = &Entry;
                break;
            }
        }
    }
    if (!Objective)
    {
        return false;
    }
    FICWorldState Candidate = State->ExportState();
    FICQuestState* Quest = Candidate.Quests.Find(QuestID);
    if (!Quest || !Quest->bAccepted || Quest->bCompleted)
    {
        return false;
    }
    const int32 Remaining = Objective->RequiredCount - Quest->Objectives.FindRef(ObjectiveID);
    if (Remaining <= 0)
    {
        return false;
    }
    UICInventoryComponent* Staging = NewObject<UICInventoryComponent>(this);
    Staging->BeginStaging(Definitions, Candidate.Job);
    if (!Staging->ApplySnapshot(Candidate.Inventory)
        || !Staging->BuildQuestItemConsumption(Objective->Target.ID, Remaining, Candidate.Inventory))
    {
        return false;
    }
    Quest->Objectives.Add(ObjectiveID, Objective->RequiredCount);
    if (!State->CommitGameplayTransaction(Candidate, FGuid::NewGuid()))
    {
        return false;
    }
    UE_LOG(LogICWorldState, Log, TEXT("[Quest.ItemDelivered] Quest=%s Objective=%s Item=%s Count=%d"),
        *QuestID.ToString(), *ObjectiveID.ToString(), *Objective->Target.ID.ToString(), Remaining);
    if (!IsProcessing)
    {
        EvaluateQuests();
    }
    return true;
}

bool UICQuestSubsystem::ReportObjectiveEvent(EICQuestObjectiveType Type, FName TargetID,
    int32 Count, FGuid EventID, EICLifeActivity Activity, FName OutputItemID, FName DialogueID)
{
    if (!Definitions || !State || !EventID.IsValid() || TargetID.IsNone() || Count <= 0
        || Type == EICQuestObjectiveType::DeliverItem || Type == EICQuestObjectiveType::PossessItem
        || Type == EICQuestObjectiveType::Affinity || Type == EICQuestObjectiveType::Stat)
    {
        return false;
    }
    EvaluateQuests();
    FICWorldState Candidate = State->ExportState();
    if (!Candidate.bPlayerCreated || Candidate.ProcessedEventIDs.Contains(EventID))
    {
        return false;
    }
    TArray<FName> QuestIDs;
    if (!Definitions->GetDefinitionIDs(EICDefinitionType::Quest, QuestIDs).bSuccess)
    {
        return false;
    }
    bool IsChanged = false;
    for (FName QuestID : QuestIDs)
    {
        FICQuestState* Quest = Candidate.Quests.Find(QuestID);
        if (!Quest || !Quest->bAccepted || Quest->bCompleted)
        {
            continue;
        }
        FICQuestRow Definition;
        if (!Definitions->GetQuest(QuestID, Definition).bSuccess)
        {
            continue;
        }
        for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
        {
            for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
            {
                const bool IsTargetMatch = Objective.Target.ID == TargetID
                    || (Type == EICQuestObjectiveType::Craft
                        && (Objective.Target.ID.IsNone()
                            || (Objective.Target.Type == EICDefinitionType::Item
                                && Objective.Target.ID == OutputItemID)));
                if (Objective.Type != Type || !IsTargetMatch
                    || (Objective.bFilterLifeActivity && Objective.LifeActivity != Activity)
                    || (!Objective.OutputItemID.IsNone() && Objective.OutputItemID != OutputItemID)
                    || (!Objective.DialogueID.IsNone() && Objective.DialogueID != DialogueID))
                {
                    continue;
                }
                const int32 Progress = Quest->Objectives.FindRef(Objective.ObjectiveID);
                Quest->Objectives.Add(Objective.ObjectiveID, static_cast<int32>(
                    FMath::Min<int64>(Objective.RequiredCount, static_cast<int64>(Progress) + Count)));
                IsChanged = true;
            }
        }
    }
    if (!IsChanged)
    {
        return false;
    }
    Candidate.ProcessedEventIDs.Add(EventID);
    IsProcessing = true;
    const bool IsCommitted = State->CommitGameplayTransaction(Candidate, FGuid::NewGuid());
    IsProcessing = false;
    if (!IsCommitted)
    {
        return false;
    }
    EvaluateQuests();
    return true;
}

void UICQuestSubsystem::HandleInventoryChanged()
{
    if (!State || IsProcessing)
    {
        return;
    }
    const FICInventoryState CurrentInventory = State->GetInventorySnapshot();
    if (!IsBaselineReady)
    {
        InventoryBaseline = CurrentInventory;
        IsBaselineReady = true;
        EvaluateQuests();
        return;
    }
    const FICInventoryState PreviousInventory = InventoryBaseline;
    InventoryBaseline = CurrentInventory;
    EvaluateQuests();

    TSet<FName> ItemIDs;
    const auto AddIDs = [&ItemIDs](const FICInventoryState& Inventory)
    {
        for (const FPlayerItemInstance& Item : Inventory.Bag.Slots) ItemIDs.Add(Item.ItemID);
        for (const FPlayerItemInstance& Item : Inventory.QuestItems) ItemIDs.Add(Item.ItemID);
        for (const FPlayerItemInstance& Item : Inventory.Equipment) ItemIDs.Add(Item.ItemID);
        for (const auto& Storage : Inventory.Storage)
        {
            for (const FPlayerItemInstance& Item : Storage.Value.Slots) ItemIDs.Add(Item.ItemID);
        }
    };
    AddIDs(CurrentInventory);
    AddIDs(PreviousInventory);
    ItemIDs.Remove(NAME_None);

    if (!Definitions || ItemIDs.IsEmpty())
    {
        return;
    }
    TArray<FName> QuestIDs;
    if (!Definitions->GetDefinitionIDs(EICDefinitionType::Quest, QuestIDs).bSuccess)
    {
        return;
    }
    FICWorldState Candidate = State->ExportState();
    bool IsChanged = false;
    for (FName QuestID : QuestIDs)
    {
        FICQuestState* Quest = Candidate.Quests.Find(QuestID);
        if (!Quest || !Quest->bAccepted || Quest->bCompleted)
        {
            continue;
        }
        FICQuestRow Definition;
        if (!Definitions->GetQuest(QuestID, Definition).bSuccess)
        {
            continue;
        }
        for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
        {
            for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
            {
                if (Objective.Type != EICQuestObjectiveType::Collect
                    || Objective.bFilterLifeActivity || !Objective.OutputItemID.IsNone()
                    || !ItemIDs.Contains(Objective.Target.ID))
                {
                    continue;
                }
                const int32 Added = CountOwnedItem(CurrentInventory, Objective.Target.ID)
                    - CountOwnedItem(PreviousInventory, Objective.Target.ID);
                if (Added <= 0)
                {
                    continue;
                }
                const int32 Progress = Quest->Objectives.FindRef(Objective.ObjectiveID);
                Quest->Objectives.Add(Objective.ObjectiveID, static_cast<int32>(
                    FMath::Min<int64>(Objective.RequiredCount, static_cast<int64>(Progress) + Added)));
                IsChanged = true;
            }
        }
    }
    if (IsChanged)
    {
        IsProcessing = true;
        const bool IsCommitted = State->CommitGameplayTransaction(Candidate, FGuid::NewGuid());
        IsProcessing = false;
        if (IsCommitted)
        {
            EvaluateQuests();
        }
    }
    InventoryBaseline = State->GetInventorySnapshot();
}

void UICQuestSubsystem::HandleWorldRestored()
{
    if (!State || IsProcessing)
    {
        return;
    }
    InventoryBaseline = State->GetInventorySnapshot();
    IsBaselineReady = true;
    EvaluateQuests();
}

void UICQuestSubsystem::HandleCharacterChanged(FName CharacterID)
{
    if (!CharacterID.IsNone())
    {
        EvaluateQuests();
    }
}

void UICQuestSubsystem::RefreshAvailableQuests()
{
    EvaluateQuests();
}

void UICQuestSubsystem::EvaluateQuests()
{
    if (!Definitions || !State || !Time || IsProcessing || !State->ExportState().bPlayerCreated)
    {
        return;
    }
    TArray<FName> QuestIDs;
    const FICDefinitionResult Lookup = Definitions->GetDefinitionIDs(EICDefinitionType::Quest, QuestIDs);
    if (!Lookup.bSuccess)
    {
        UE_LOG(LogICWorldState, Warning, TEXT("[Quest.DefinitionListRejected] Row=%s Field=%s Reason=%s"),
            *Lookup.Row.ToString(), *Lookup.Field, *Lookup.Reason);
        return;
    }
    IsProcessing = true;
    for (int32 Pass = 0; Pass < QuestIDs.Num() * 2; ++Pass)
    {
        bool IsChanged = false;
        for (FName QuestID : QuestIDs)
        {
            FICQuestRow Definition;
            if (!Definitions->GetQuest(QuestID, Definition).bSuccess)
            {
                continue;
            }
            FICWorldState World = State->ExportState();
            const FICQuestState* Quest = World.Quests.Find(QuestID);
            if ((!Quest || !Quest->bAccepted) && Definition.IsAutoAccept && CanAccept(Definition, World))
            {
                if (AcceptQuest(QuestID))
                {
                    IsChanged = true;
                    World = State->ExportState();
                    Quest = World.Quests.Find(QuestID);
                }
            }
            if (Quest && Quest->bAccepted && !Quest->bCompleted
                && Definition.CompletionMode == EICQuestCompletionMode::SystemEvent
                && AreObjectivesComplete(Definition, *Quest, World)
                && CompleteQuest(QuestID, Definition))
            {
                IsChanged = true;
            }
        }
        if (!IsChanged)
        {
            break;
        }
    }
    IsProcessing = false;
    InventoryBaseline = State->GetInventorySnapshot();
    IsBaselineReady = true;
}
