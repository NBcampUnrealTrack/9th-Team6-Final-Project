#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerItemTypes.h"
#include "PlayerSurvival.h"
#include "../Core/ICStateParticipant.h"
#include "PlayerPrototypeController.generated.h"

class APlayerPrototypeObject;
class UPlayerCrafting;
class UPlayerInventory;
class UPlayerPrototypeWidget;
class UICCharacterCreateWidget;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICSaveSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerCombatRequest,FName,Action,int32,Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerEffectRequest,FGuid,RequestID,FName,ItemID,FName,TargetID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerInitialized,FName,JobID,FName,FirstSkillID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerStatBookApplied,FName,TargetID,EPlayerMainStat,Stat,int32,Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerExhaustionChanged,bool,bExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerSleepRequested,int32,GameMinutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerEquipmentStatsChanged,FPlayerEquipmentStats,Stats);

UCLASS()
class ISEKAICLASS_API APlayerPrototypeController : public APlayerController, public IICStateParticipant
{
    GENERATED_BODY()
public:
    APlayerPrototypeController();
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    TSubclassOf<UICCharacterCreateWidget> CharacterCreateWidgetClass;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    UPROPERTY(BlueprintAssignable) FPlayerCombatRequest OnCombatRequest;
    UPROPERTY(BlueprintAssignable) FPlayerEffectRequest OnEffectRequest;
    UPROPERTY(BlueprintAssignable) FPlayerInitialized OnPlayerInitialized;
    UPROPERTY(BlueprintAssignable) FPlayerStatBookApplied OnStatBookApplied;
    UPROPERTY(BlueprintAssignable) FPlayerExhaustionChanged OnExhaustionChanged;
    UPROPERTY(BlueprintAssignable) FPlayerSleepRequested OnSleepRequested;
    UPROPERTY(BlueprintAssignable) FPlayerEquipmentStatsChanged OnEquipmentStatsChanged;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prototype") bool bUseTestEffects=true;
    // Creation form fields; WorldState owns saved values after creation.
    UPROPERTY(BlueprintReadOnly) FString PlayerName;
    UPROPERTY(BlueprintReadOnly) int32 Preset=0;
    UPROPERTY(BlueprintReadOnly) int32 Job=0;
    UPROPERTY(BlueprintReadOnly) FName FirstSkill;
    UPROPERTY(BlueprintReadOnly) bool bCreated=false;
    UPROPERTY(BlueprintReadOnly) bool bInCombat=false;
    UPROPERTY(BlueprintReadWrite) bool bInDialogue=false;
    UPROPERTY(BlueprintReadWrite) bool bInEvent=false;
    UPROPERTY(BlueprintReadOnly) bool bBlocking=false;
    UPROPERTY(BlueprintReadOnly) FPlayerSurvivalState Survival;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Survival") float SleepSPRecoveryPerGameMinute=1.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Survival") float SPDrainPerGameMinute=0.05f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Survival") float ExhaustedMoveSpeedMultiplier=0.65f;
    UPROPERTY(BlueprintReadOnly) FPlayerMainStats MainStats;
    UPROPERTY(BlueprintReadOnly) int32 Gold=1000;
    UPROPERTY(BlueprintReadOnly) int32 SelectedSaveSlot=1;
    UPROPERTY(BlueprintReadOnly) int32 SelectedLoadSlot=0;
    UPROPERTY(BlueprintReadOnly) TArray<FICSaveSlotInfo> SaveSlots;
    FString ModalText;
    FString InteractionHint;
    static FKey MenuCancelKey();
    static FString MenuCancelLabel();
    bool IsQuitAfterSave() const { return bExitAfterSave; }
    void OpenSaveMenu(bool bQuitAfter=false);
    void OpenLoadMenu();
    void CloseSessionMenu();
    void RequestManualSave();
    void RequestCreatePlayer();
    void RequestQuit();
    void RequestDelete(bool bLegacy=false);
    void ConfirmModal(bool bAccept);
    void QuitWithoutSave();
    void FinishSessionSave(int32 Slot,bool bSuccess);
    APlayerPrototypeObject* FindInteractionTarget() const;
    FString Menu=TEXT("Create");
    FString Notice;
    FName OpenStorage;
    FName ActiveStation;
    FName PendingItem;
    int32 PendingQuick=INDEX_NONE;
    int32 SelectedBag=INDEX_NONE;
    int32 SelectedStorage=INDEX_NONE;
    int32 MoveFrom=INDEX_NONE;
    int32 PendingPartyKey=INDEX_NONE;
    bool bDirty=true;
    UPROPERTY() TObjectPtr<UPlayerPrototypeWidget> Screen;
    UPlayerInventory* GetInventory() const;
    UPlayerCrafting* GetCrafting() const;
    UICWorldStateSubsystem* GetWorldState() const;
    UICTimeSubsystem* GetTime() const;
    UICSaveSubsystem* GetSaves() const;
    UFUNCTION(BlueprintPure) FICCharacterState GetPlayerSnapshot() const;
    bool CanEditCreation() const { return !bCreated && !bInitialSavePending; }
    bool CanSubmitCreation() const;
    bool ValidateCreationRequest(FString& OutReason) const;
    void ReportCreationFailure(const FString& Reason);
    UFUNCTION(BlueprintCallable) void SetCombatState(bool Active);
    UFUNCTION(BlueprintCallable) void SetFinalMoveSpeed(float Speed);
    UFUNCTION(BlueprintCallable) void ResolveEffect(FGuid RequestID,bool Success);
    UFUNCTION(BlueprintCallable) void PlayerDied();
    UFUNCTION(BlueprintCallable) float ConsumeProductionSP(float Amount);
    UFUNCTION(BlueprintCallable) void ApplyGameMinutes(int32 Minutes);
    UFUNCTION() void HandleGameMinutesAdvanced(int32 Minutes);
    UFUNCTION(BlueprintCallable) bool Sleep();
    UFUNCTION(BlueprintCallable) bool CraftRecipe(FName RecipeID);
    UFUNCTION(BlueprintCallable) void OpenContainer(FName ContainerID,bool bPlayerHome,bool bAccessGranted);
    void HandleKey(FKey Key);
    void ExecuteUI(const FString& Command,int32 Index);
    void SetMenu(const FString& NewMenu);
    void Show(const FString& Text);
    bool SavePlayer();
    bool AutoSavePlayer();
    bool LoadPlayer();
    void BeginItem(FName ID,int32 Quick=INDEX_NONE);
    void ConfirmTarget(FName TargetID);
    void SyncSaveSafety();
    void RefreshSaveSlots();
    UFUNCTION() void InventoryChanged();
    UFUNCTION() void InventoryMessage(const FText& Message);
    UFUNCTION() void InventoryLootResult(FName ItemID,int32 Accepted,int32 Overflow);
    UFUNCTION() void InventoryDropRequested(const FPlayerItemInstance& Item);
    UFUNCTION() void CharacterChanged(FName ID);
    UFUNCTION() void ClockChanged();
    UFUNCTION() void SaveStarted(int32 Slot);
    UFUNCTION() void SaveFinished(int32 Slot,bool bSuccess,const FString& Message);
    virtual bool ReadLiveState_Implementation(FName CharacterId,FICCharacterState& Snapshot) const override;
    virtual bool FlushToWorldState_Implementation(UICWorldStateSubsystem* WorldState) override;
    virtual bool CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
private:
    FString SaveReturnMenu;
    FString LoadReturnMenu=TEXT("Create");
    FString QuitReturnMenu;
    FString ModalReturnMenu;
    FString ModalAction;
    int32 ModalSlot=INDEX_NONE;
    bool bExitAfterSave=false;
    bool bQuitAfterWrite=false;
    bool bSessionClosing=false;
    void ShowModal(const FString& Action,const FString& Text,int32 Slot=INDEX_NONE);
    void CompleteQuit();
    FGuid PendingRequest;
    bool bEffectWaiting=false;
    bool bDead=false;
    bool bInitialSavePending=false;
    bool bLastExhausted=false;
    void CreatePlayer();
    bool UseStatBook(FName ItemID,FName TargetID,const FPlayerItemDefinition& Definition);
    void FlushWorldState();
    void CancelTarget();
    void Interact();
    void SpawnDroppedItem(const FPlayerItemInstance& Item);
    void StartSmokeTest();
    void UpdateGamePhase();
};
