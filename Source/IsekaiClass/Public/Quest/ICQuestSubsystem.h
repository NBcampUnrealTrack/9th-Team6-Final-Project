#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Quest/ICQuestRow.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/State/Types/ICInventoryStateTypes.h"
#include "ICQuestSubsystem.generated.h"

class UICDefinitionSubsystem;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
struct FICWorldState;
struct FICQuestState;
struct FICInventoryState;
struct FICRewardRow;

UCLASS()
class ISEKAICLASS_API UICQuestSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void ConfigureServices(UICDefinitionSubsystem* InDefinitions, UICWorldStateSubsystem* InState, UICTimeSubsystem* InTime);

    UFUNCTION(BlueprintCallable, Category="IC|Quest")
    bool AcceptQuest(FName QuestID);

    UFUNCTION(BlueprintCallable, Category="IC|Quest") bool StartQuest(FName QuestID);
    UFUNCTION(BlueprintCallable, Category="IC|Quest") bool AbandonQuest(FName QuestID);

    UFUNCTION(BlueprintCallable, Category="IC|Quest")
    bool TurnInQuest(FName QuestID, FName NPCID);

    UFUNCTION(BlueprintPure, Category="IC|Quest")
    bool CanOfferQuest(FName QuestID) const;

    UFUNCTION(BlueprintPure, Category="IC|Quest")
    bool AreQuestObjectivesReady(FName QuestID) const;

    /** 0: none, 1: available quest, 2: current quest objective/turn-in. */
    UFUNCTION(BlueprintPure, Category="IC|Quest") int32 GetNPCQuestMarker(FName NPCID) const;

    /** Dedicated dialogue for an unfinished Talk objective in a started quest, or None. */
    UFUNCTION(BlueprintPure, Category="IC|Quest")
    FName GetNPCObjectiveDialogue(FName NPCID) const;

    /** Shared objective predicate for gameplay and quest guidance. */
    bool IsObjectiveSatisfied(const FICQuestObjectiveDefinition& Objective, const FICQuestState& Quest,
        const FICWorldState& World) const { return IsObjectiveComplete(Objective, Quest, World); }

    bool GetObjectiveProgress(const FICQuestObjectiveDefinition& Objective, const FICQuestState& Quest,
        const FICWorldState& World, int32& CurrentCount) const;

    UFUNCTION(BlueprintCallable, Category="IC|Quest")
    bool ReportObjectiveEvent(EICQuestObjectiveType Type, FName TargetID, int32 Count, FGuid EventID,
        EICLifeActivity Activity = EICLifeActivity::Gathering, FName OutputItemID = NAME_None, FName DialogueID = NAME_None);

    UFUNCTION(BlueprintCallable, Category="IC|Quest")
    bool DeliverItem(FName QuestID, FName ObjectiveID);

    int32 ForceCompleteActiveQuests();

    void RefreshAvailableQuests();

private:
    UPROPERTY(Transient)
    TObjectPtr<UICDefinitionSubsystem> Definitions;

    UPROPERTY(Transient)
    TObjectPtr<UICWorldStateSubsystem> State;

    UPROPERTY(Transient)
    TObjectPtr<UICTimeSubsystem> Time;

    FICInventoryState InventoryBaseline;
    bool IsBaselineReady = false;
    bool IsProcessing = false;

    UFUNCTION()
    void HandleInventoryChanged();

    UFUNCTION()
    void HandleWorldRestored();

    UFUNCTION()
    void HandleCharacterChanged(FName CharacterID);

    void EvaluateQuests();
    bool AcceptQuestInternal(FName QuestID);
    /** Commit a quest mutation without treating its state notifications as new gameplay events. */
    bool CommitQuestState(const FICWorldState& Candidate, FGuid TransactionID);
    bool CanAccept(const FICQuestRow& Definition, const FICWorldState& World) const;
    bool IsObjectiveComplete(const FICQuestObjectiveDefinition& Objective, const FICQuestState& Quest,
        const FICWorldState& World) const;
    bool IsObjectiveGroupComplete(const FICQuestObjectiveGroup& Group, const FICQuestState& Quest,
        const FICWorldState& World) const;
    bool AreObjectivesComplete(const FICQuestRow& Definition, const FICQuestState& Quest,
        const FICWorldState& World) const;
    bool TryCompleteDeliveryGroup(const FICQuestRow& Definition, const FICQuestObjectiveGroup& Group,
        FICWorldState& Candidate) const;
    bool PrepareDeliveryObjectives(const FICQuestRow& Definition, FICWorldState& World) const;
    bool CompleteQuest(FName QuestID, const FICQuestRow& Definition, bool IsForced = false,
        const FICWorldState* PreparedWorld = nullptr);
    bool ApplyReward(FICWorldState& Candidate, const FICRewardRow& Reward);
};
