#include "Quest/ICQuestSubsystem.h"
#include "ICQuestObjectiveEvaluator.h"

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

namespace
{
bool IsRunningQuest(const FICQuestState& Quest)
{
    return Quest.bAccepted && Quest.bStarted && !Quest.bCompleted && !Quest.bAbandoned;
}

FICQuestObjectiveEvaluator MakeObjectiveEvaluator(const UICQuestSubsystem& Subsystem)
{
    return FICQuestObjectiveEvaluator(Subsystem.GetWorld());
}
}

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

bool UICQuestSubsystem::CommitQuestState(const FICWorldState& Candidate, FGuid TransactionID)
{
    if (!State || !TransactionID.IsValid()) return false;
    TGuardValue<bool> Guard(IsProcessing, true);
    if (!State->CommitGameplayTransaction(Candidate, TransactionID)) return false;
    InventoryBaseline = State->GetInventorySnapshot();
    IsBaselineReady = true;
    return true;
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
    if (IsProcessing) return false;
    return AcceptQuestInternal(QuestID);
}

bool UICQuestSubsystem::AcceptQuestInternal(FName QuestID)
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
        // Keep quest items on abandon; only replenish missing grants on reacceptance.
        const FICQuestState* Previous = Candidate.Quests.Find(QuestID);
        TMap<FName, int32> GrantCounts;
        for (const FICItemAmount& Item : Definition.AcceptanceQuestItems)
        {
            if (Item.Count <= 0 || GrantCounts.FindRef(Item.ItemID) > MAX_int32 - Item.Count)
            {
                return false;
            }
            GrantCounts.FindOrAdd(Item.ItemID) += Item.Count;
        }
        for (const auto& Grant : GrantCounts)
        {
            FICItemRow ItemDefinition;
            if (!Definitions->GetItem(Grant.Key, ItemDefinition).bSuccess
                || ItemDefinition.Kind != EPlayerItemKind::Quest)
            {
                return false;
            }
            const int32 Count = Previous && Previous->bAbandoned
                ? FMath::Max(0, Grant.Value - FICQuestObjectiveEvaluator::CountOwnedItem(Candidate.Inventory, Grant.Key)) : Grant.Value;
            if (Count > 0 && !Staging->AddItem(Grant.Key, Count, false))
            {
                return false;
            }
        }
        Candidate.Inventory = Staging->GetSnapshot();
    }
    bool HasRunningQuest = false;
    for (const auto& Pair : Candidate.Quests)
        if (Pair.Value.bAccepted && Pair.Value.bStarted && !Pair.Value.bCompleted && !Pair.Value.bAbandoned)
        { HasRunningQuest = true; break; }
    FICQuestState& Quest = Candidate.Quests.FindOrAdd(QuestID);
    Quest.bAccepted = true;
    Quest.bStarted = !HasRunningQuest;
    Quest.bAbandoned = false;
    Quest.bCompleted = false;
    Quest.bRewardClaimed = false;
    Quest.RewardTransactionID.Invalidate();
    Quest.Objectives.Reset();
    Quest.AcceptedAtGameMinute = Time->GetClock().GameMinute;
    // Acceptance and automatic tracking are one transaction. Existing progress stays intact.
    if (!CommitQuestState(Candidate, FGuid::NewGuid())) return false;
    UE_LOG(LogICWorldState, Log, TEXT("[Quest.Accepted] Quest=%s"), *QuestID.ToString());
    if (!IsProcessing)
    {
        EvaluateQuests();
    }
    return true;
}

bool UICQuestSubsystem::StartQuest(FName QuestID)
{
    if (!Definitions || !State || !Time || IsProcessing) return false;
    FICQuestRow Definition;
    FICWorldState Candidate = State->ExportState();
    FICQuestState* Selected = Candidate.Quests.Find(QuestID);
    if (!Selected || !Selected->bAccepted || Selected->bCompleted || Selected->bAbandoned
        || !Definitions->GetQuest(QuestID, Definition).bSuccess) return false;
    if (Selected->bStarted) return true;

    // Switching the tracked quest restarts its event objectives by design.
    for (auto& Pair : Candidate.Quests)
    {
        if (Pair.Value.bStarted && !Pair.Value.bCompleted)
        {
            Pair.Value.bStarted = false;
            Pair.Value.Objectives.Reset();
            Pair.Value.AcceptedAtGameMinute = 0;
        }
    }
    Selected->bStarted = true;
    Selected->Objectives.Reset();
    Selected->AcceptedAtGameMinute = Time->GetClock().GameMinute;
    if (!CommitQuestState(Candidate, FGuid::NewGuid())) return false;
    EvaluateQuests();
    return true;
}

bool UICQuestSubsystem::AbandonQuest(FName QuestID)
{
    if (!Definitions || !State || IsProcessing) return false;
    FICQuestRow Definition;
    FICWorldState Candidate = State->ExportState();
    FICQuestState* Quest = Candidate.Quests.Find(QuestID);
    if (!Quest || !Quest->bAccepted || Quest->bCompleted
        || !Definitions->GetQuest(QuestID, Definition).bSuccess || !Definition.bAllowAbandon) return false;
    Quest->bAccepted = false;
    Quest->bStarted = false;
    Quest->bAbandoned = true;
    Quest->Objectives.Reset();
    Quest->AcceptedAtGameMinute = 0;
    return CommitQuestState(Candidate, FGuid::NewGuid());
}

bool UICQuestSubsystem::IsObjectiveComplete(const FICQuestObjectiveDefinition& Objective,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    return MakeObjectiveEvaluator(*this).IsComplete(Objective, Quest, World);
}

bool UICQuestSubsystem::GetObjectiveProgress(const FICQuestObjectiveDefinition& Objective,
    const FICQuestState& Quest, const FICWorldState& World, int32& CurrentCount) const
{
    return MakeObjectiveEvaluator(*this).GetProgress(Objective, Quest, World, CurrentCount);
}

bool UICQuestSubsystem::IsObjectiveGroupComplete(const FICQuestObjectiveGroup& Group,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    return MakeObjectiveEvaluator(*this).IsGroupComplete(Group, Quest, World);
}

bool UICQuestSubsystem::AreObjectivesComplete(const FICQuestRow& Definition,
    const FICQuestState& Quest, const FICWorldState& World) const
{
    return MakeObjectiveEvaluator(*this).AreObjectivesComplete(Definition, Quest, World);
}

bool UICQuestSubsystem::TryCompleteDeliveryGroup(const FICQuestRow& Definition,
    const FICQuestObjectiveGroup& Group, FICWorldState& Candidate) const
{
    FICQuestState& Quest = Candidate.Quests.FindChecked(Definition.ID);
    if (IsObjectiveGroupComplete(Group, Quest, Candidate)) return true;

    UICInventoryComponent* Staging = NewObject<UICInventoryComponent>();
    Staging->BeginStaging(Definitions, Candidate.Job);
    for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
    {
        if (Objective.Type != EICQuestObjectiveType::DeliverItem
            || IsObjectiveComplete(Objective, Quest, Candidate)) continue;

        const int32 Remaining = Objective.RequiredCount - Quest.Objectives.FindRef(Objective.ObjectiveID);
        FICInventoryState UpdatedInventory;
        if (Remaining <= 0 || !Staging->ApplySnapshot(Candidate.Inventory)
            || !Staging->BuildQuestItemConsumption(Objective.Target.ID, Remaining, UpdatedInventory))
        {
            if (Group.Policy == EICConditionPolicy::All) return false;
            continue;
        }
        Candidate.Inventory = MoveTemp(UpdatedInventory);
        Quest.Objectives.Add(Objective.ObjectiveID, Objective.RequiredCount);
        if (Group.Policy == EICConditionPolicy::Any) break;
    }
    return IsObjectiveGroupComplete(Group, Quest, Candidate);
}

bool UICQuestSubsystem::PrepareDeliveryObjectives(const FICQuestRow& Definition,
    FICWorldState& World) const
{
    const FICQuestState* Quest = World.Quests.Find(Definition.ID);
    if (!Quest || !IsRunningQuest(*Quest)) return false;
    if (AreObjectivesComplete(Definition, *Quest, World)) return true;

    FICWorldState Working = World;
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        FICWorldState Trial = Working;
        if (!TryCompleteDeliveryGroup(Definition, Group, Trial))
        {
            if (Definition.ObjectiveGroupPolicy == EICConditionPolicy::All) return false;
            continue;
        }
        Working = MoveTemp(Trial);
        if (Definition.ObjectiveGroupPolicy == EICConditionPolicy::Any) break;
    }
    if (!AreObjectivesComplete(Definition, Working.Quests.FindChecked(Definition.ID), Working)) return false;
    World = MoveTemp(Working);
    return true;
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
    if (!Quest || !Quest->bAccepted || !Quest->bStarted || Quest->bCompleted
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
    Quest->bStarted = false;
    Quest->bRewardClaimed = IsRewardApplied;
    Quest->RewardTransactionID = IsRewardApplied ? TransactionID : FGuid();
    Quest->CompletionCount++;
    Quest->CompletedAtGameMinute = Time->GetClock().GameMinute;
    if (!CommitQuestState(Candidate, TransactionID))
    {
        return false;
    }
    UE_LOG(LogICWorldState, Log, TEXT("[Quest.Completed] Quest=%s Reward=%s"),
        *QuestID.ToString(), *Definition.RewardID.ToString());
    return true;
}

int32 UICQuestSubsystem::ForceCompleteActiveQuests()
{
    if (!Definitions || !State || !Time || IsProcessing)
    {
        return 0;
    }

    const FICWorldState World = State->ExportState();
    int32 CompletedCount = 0;
    for (const auto& Entry : World.Quests)
    {
        if (!Entry.Value.bAccepted || !Entry.Value.bStarted || Entry.Value.bCompleted)
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
    if (!Definitions || !State || IsProcessing)
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
    if (!Definitions || !State || !Time || IsProcessing)
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
    if (!Quest || !Quest->bAccepted || !Quest->bStarted || Quest->bCompleted)
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
    if (!CommitQuestState(Candidate, FGuid::NewGuid()))
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
    if (!Definitions || !State || IsProcessing || !EventID.IsValid() || TargetID.IsNone() || Count <= 0
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
    bool IsChanged = false;
    for (auto& Entry : Candidate.Quests)
    {
        FICQuestState& Quest = Entry.Value;
        if (!IsRunningQuest(Quest))
        {
            continue;
        }
        FICQuestRow Definition;
        if (!Definitions->GetQuest(Entry.Key, Definition).bSuccess)
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
                const int32 Progress = Quest.Objectives.FindRef(Objective.ObjectiveID);
                const int32 Updated = static_cast<int32>(FMath::Min<int64>(
                    Objective.RequiredCount, static_cast<int64>(Progress) + Count));
                if (Updated > Progress)
                {
                    Quest.Objectives.Add(Objective.ObjectiveID, Updated);
                    IsChanged = true;
                }
            }
        }
    }
    if (!IsChanged)
    {
        return false;
    }
    Candidate.ProcessedEventIDs.Add(EventID);
    if (!CommitQuestState(Candidate, FGuid::NewGuid()))
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

    TSet<FName> ItemIDs;
    const auto AddIDs = [&ItemIDs](const FICInventoryState& Inventory)
    {
        for (const FPlayerItemInstance& Item : Inventory.Bag.Slots) ItemIDs.Add(Item.ItemID);
        for (const FPlayerItemInstance& Item : Inventory.QuestItems) ItemIDs.Add(Item.ItemID);
        for (const FPlayerItemInstance& Item : Inventory.Equipment) ItemIDs.Add(Item.ItemID);
        for (const FPlayerItemInstance& Item : Inventory.WeaponSlots) ItemIDs.Add(Item.ItemID);
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
        EvaluateQuests();
        return;
    }
    FICWorldState Candidate = State->ExportState();
    bool IsChanged = false;
    for (auto& Entry : Candidate.Quests)
    {
        FICQuestState& Quest = Entry.Value;
        if (!IsRunningQuest(Quest))
        {
            continue;
        }
        FICQuestRow Definition;
        if (!Definitions->GetQuest(Entry.Key, Definition).bSuccess)
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
                const int64 Added = static_cast<int64>(FICQuestObjectiveEvaluator::CountOwnedItem(CurrentInventory, Objective.Target.ID))
                    - FICQuestObjectiveEvaluator::CountOwnedItem(PreviousInventory, Objective.Target.ID);
                if (Added <= 0)
                {
                    continue;
                }
                const int32 Progress = Quest.Objectives.FindRef(Objective.ObjectiveID);
                const int32 Updated = static_cast<int32>(FMath::Min<int64>(
                    Objective.RequiredCount, static_cast<int64>(Progress) + Added));
                if (Updated > Progress)
                {
                    Quest.Objectives.Add(Objective.ObjectiveID, Updated);
                    IsChanged = true;
                }
            }
        }
    }
    if (IsChanged)
    {
        CommitQuestState(Candidate, FGuid::NewGuid());
    }
    EvaluateQuests();
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
    TGuardValue<bool> Guard(IsProcessing, true);
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
            if ((!Quest || (!Quest->bAccepted&&!Quest->bAbandoned)) && Definition.IsAutoAccept && CanAccept(Definition, World))
            {
                if (AcceptQuestInternal(QuestID))
                {
                    IsChanged = true;
                    World = State->ExportState();
                    Quest = World.Quests.Find(QuestID);
                }
            }
            if (Quest && Quest->bAccepted && Quest->bStarted && !Quest->bCompleted
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
    InventoryBaseline = State->GetInventorySnapshot();
    IsBaselineReady = true;
}

int32 UICQuestSubsystem::GetNPCQuestMarker(FName NPCID) const
{
    if (!Definitions || !State || NPCID.IsNone()) return 0;
    const auto World = State->ExportState();
    if(const auto* NPC=World.Characters.Find(NPCID);NPC&&NPC->IsDowned())return 0;
    FICDefinitionResult NPCResult;const auto* NPCDefinition=Definitions->Find<FICNPCRow>(NPCID,NPCResult);
    TArray<FName> IDs; Definitions->GetDefinitionIDs(EICDefinitionType::Quest, IDs);
    bool Available = false;
    for (FName ID : IDs)
    {
        FICDefinitionResult Result; const auto* Quest = Definitions->Find<FICQuestRow>(ID, Result);
        if (!Quest) continue;
        const auto* Progress = World.Quests.Find(ID);
        if (Progress && Progress->bAccepted && Progress->bStarted && !Progress->bCompleted && !Progress->bAbandoned)
        {
            const bool Ready=AreQuestObjectivesReady(ID);
            if (Quest->TurnInNPCID == NPCID && Ready) return 2;
            if(Ready)continue;
            for (const auto& Group : Quest->ObjectiveGroups)
            {
                if (IsObjectiveGroupComplete(Group, *Progress, World)) continue;
                for (const auto& Objective : Group.Objectives)
                    if ((Objective.Target.Type == EICDefinitionType::NPC && Objective.Target.ID == NPCID)
                        ||(NPCDefinition&&!NPCDefinition->InitialAnchorID.IsNone()
                            &&(Objective.GuidanceAnchorID==NPCDefinition->InitialAnchorID
                                ||(Objective.Target.Type==EICDefinitionType::Anchor&&Objective.Target.ID==NPCDefinition->InitialAnchorID))))
                    {
                        if (!IsObjectiveComplete(Objective, *Progress, World)) return 2;
                    }
            }
        }
        if (Quest->OfferNPCID == NPCID && CanOfferQuest(ID)) Available = true;
    }
    return Available ? 1 : 0;
}

FName UICQuestSubsystem::GetNPCObjectiveDialogue(FName NPCID) const
{
    if (!Definitions || !State || NPCID.IsNone()) return NAME_None;
    const FICWorldState World = State->ExportState();
    TArray<FName> Candidates;
    for (const auto& Entry : World.Quests)
    {
        const FICQuestState& Quest = Entry.Value;
        if (Quest.bAccepted && Quest.bStarted && !Quest.bCompleted && !Quest.bAbandoned)
            Candidates.Add(Entry.Key);
    }
    // Normally only one quest is started. Keep selection stable for restored data.
    Candidates.Sort([&World](FName A, FName B)
    {
        const double TimeA = World.Quests.FindChecked(A).AcceptedAtGameMinute;
        const double TimeB = World.Quests.FindChecked(B).AcceptedAtGameMinute;
        return TimeA == TimeB ? A.LexicalLess(B) : TimeA > TimeB;
    });
    for (FName QuestID : Candidates)
    {
        FICQuestRow Definition;
        const FICQuestState& Quest = World.Quests.FindChecked(QuestID);
        if (!Definitions->GetQuest(QuestID, Definition).bSuccess
            || AreObjectivesComplete(Definition, Quest, World)) continue;
        for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
        {
            if (IsObjectiveGroupComplete(Group, Quest, World)) continue;
            for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
            {
                if (Objective.Type != EICQuestObjectiveType::Talk
                    || Objective.Target.Type != EICDefinitionType::NPC || Objective.Target.ID != NPCID
                    || Objective.DialogueID.IsNone() || IsObjectiveComplete(Objective, Quest, World)) continue;
                FICDialogueRow Dialogue;
                if (Definitions->GetDialogue(Objective.DialogueID, Dialogue).bSuccess
                    && Dialogue.NPCID == NPCID && !Dialogue.EntryNodeID.IsNone()
                    && Dialogue.Nodes.ContainsByPredicate([&](const FICDialogueNode& Node)
                        { return Node.NodeID == Dialogue.EntryNodeID; }))
                    return Objective.DialogueID;
                UE_LOG(LogICWorldState, Warning,
                    TEXT("[Quest.Dialogue.Invalid] Quest=%s NPC=%s Dialogue=%s"),
                    *QuestID.ToString(), *NPCID.ToString(), *Objective.DialogueID.ToString());
            }
        }
    }
    return NAME_None;
}
