#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICWorldStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICCharacterChanged, FName, CharacterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICStateChanged);

UCLASS()
class ISEKAICLASS_API UICWorldStateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICCharacterChanged OnCharacterChanged;
    UPROPERTY(BlueprintAssignable) FICStateChanged OnInventoryChanged;
    UPROPERTY(BlueprintAssignable) FICStateChanged OnWorldRestored;
    UPROPERTY(BlueprintAssignable) FICStateChanged OnPlayerGoldChanged;
    UPROPERTY(BlueprintAssignable) FICStateChanged OnDropsChanged;

    static FName PlayerId() { IC_TRACE_FUNCTION(); return TEXT("CHR_PLAYER"); }
    static bool IsCharacterIdValid(FName ID);
    UFUNCTION(BlueprintPure, Category="IC|State") FICCharacterState GetSnapshot(FName CharacterId) const;
    UFUNCTION(BlueprintPure, Category="IC|State") FICInventoryState GetInventorySnapshot() const { IC_TRACE_FUNCTION(); return State.Inventory; }
    UFUNCTION(BlueprintPure, Category="IC|State") FICWorldState ExportState() const { IC_TRACE_FUNCTION(); return State; }
    const FICCharacterState* GetStoredCharacter(FName ID) const { IC_TRACE_FUNCTION(); return State.Characters.Find(ID); }
    UFUNCTION(BlueprintCallable, Category="IC|State") bool EnsureCharacter(FName ID, EICCharacterKind Kind);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool SetCharacterState(const FICCharacterState& Character);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool SetAffinity(FName ID, int32 Value);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool SetMainStat(FName ID, EICMainStat Stat, int32 Value);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool SetVitals(FName ID, float HP, float MP, float SP);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool SetCharacterMode(FName ID, EICCharacterMode Mode);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool SetPartySlot(FName ID, int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool CommitInventoryState(const FICInventoryState& Inventory);
    bool CommitCharacterAndInventory(const FICCharacterState& Character, const FICInventoryState& Inventory);
    bool CommitGameplayTransaction(const FICWorldState& Candidate, FGuid TransactionID);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool SetPlayerTransform(FVector Position, FRotator Rotation);
    UFUNCTION(BlueprintCallable, Category="IC|State") bool RegisterStateParticipant(FName ID, UObject* Participant);
    UFUNCTION(BlueprintCallable, Category="IC|State") void UnregisterStateParticipant(FName ID, UObject* Participant);
    bool SetPlayerGold(int32 Value);
    bool FlushParticipants();
    bool CanRestoreState(const FICWorldState& Candidate) const;
    bool RestoreState(const FICWorldState& Candidate);
    UFUNCTION(BlueprintPure, Category="IC|State") static bool ValidateStateWithReason(const FICWorldState& Candidate, FString& FailureReason);
    static bool ValidateState(const FICWorldState& Candidate);
    static bool ValidateAdditionalState(const FICWorldState& Candidate, FString& FailureReason);
    static bool ValidateStateClock(const FICWorldState& Candidate, const FICClockState& Clock);
    UFUNCTION(BlueprintPure, Category="IC|State") double GetProductionRemainingSeconds(FGuid JobID, double PlaySecond) const;
    static bool ValidateInventoryShape(const FICInventoryState& Inventory, TSet<FGuid>* ExistingIDs = nullptr);
    bool ApplyWorldProgress(const FICWorldState& Candidate);
private:
    bool AdvanceOfflineCharacter(FName ID);
    UPROPERTY() FICWorldState State;
    TMap<FName, TWeakObjectPtr<UObject>> Participants;
};
