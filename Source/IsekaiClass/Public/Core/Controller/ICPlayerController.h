#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "Life/Survival/ICPlayerSurvival.h"
#include "World/State/ICStateParticipant.h"
#include "Input/ICInputTypes.h"
#include "ICPlayerController.generated.h"

class AICInteractionActor;
class AICDroppedItem;
class UICCraftingComponent;
class UICInventoryComponent;
class UICGameplayWidget;
class UICCharacterCreateWidget;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICSaveSubsystem;
class UInputMappingContext;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerCombatRequest,FName,Action,int32,Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerEffectRequest,FGuid,RequestID,FName,ItemID,FName,TargetID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerInitialized,FName,JobID,FName,FirstSkillID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerStatBookApplied,FName,TargetID,EPlayerMainStat,Stat,int32,Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerExhaustionChanged,bool,bExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerSleepRequested,int32,GameMinutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerEquipmentStatsChanged,FPlayerEquipmentStats,Stats);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICMenuChanged,const FString&,Menu);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICNoticeChanged,const FString&,Notice);

UCLASS(Blueprintable)
class ISEKAICLASS_API AICPlayerController : public APlayerController, public IICStateParticipant
{
    GENERATED_BODY()
public:
    AICPlayerController();
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    TSubclassOf<UICCharacterCreateWidget> CharacterCreateWidgetClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|UI") TSubclassOf<UICGameplayWidget> GameplayWidgetClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TObjectPtr<UInputMappingContext> DefaultMappingContext;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") int32 MappingPriority=0;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TObjectPtr<UInputAction> LookAction;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TArray<FICInputCommandBinding> CommandActions;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input",meta=(ClampMin="0.05")) float SprintHoldSeconds=0.2f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input",meta=(ClampMin="0.05")) float TargetHoldSeconds=0.35f;
    UPROPERTY(BlueprintReadOnly,Category="IC|Input") bool bSprinting=false;
    UPROPERTY(BlueprintAssignable,Category="IC|UI") FICMenuChanged OnMenuChanged;
    UPROPERTY(BlueprintAssignable,Category="IC|UI") FICNoticeChanged OnNoticeChanged;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|New Game") TArray<FName> StartingWeapons;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|New Game") TArray<FName> StartingSkills;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|New Game") TMap<FName,int32> StartingItems;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|New Game") TArray<FName> StartingQuickSlots;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|New Game",meta=(ClampMin="0")) int32 StartingGold=1000;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void FlushPressedKeys() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void ReceivedPlayer() override;
    UPROPERTY(BlueprintAssignable) FPlayerCombatRequest OnCombatRequest;
    UPROPERTY(BlueprintAssignable) FPlayerEffectRequest OnEffectRequest;
    UPROPERTY(BlueprintAssignable) FPlayerInitialized OnPlayerInitialized;
    UPROPERTY(BlueprintAssignable) FPlayerStatBookApplied OnStatBookApplied;
    UPROPERTY(BlueprintAssignable) FPlayerExhaustionChanged OnExhaustionChanged;
    UPROPERTY(BlueprintAssignable) FPlayerSleepRequested OnSleepRequested;
    UPROPERTY(BlueprintAssignable) FPlayerEquipmentStatsChanged OnEquipmentStatsChanged;
    // Native item effects are optional. Disable when connecting GAS through OnEffectRequest.
    UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category="IC|Items") bool bUseNativeItemEffects=true;
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
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString ModalText;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString InteractionHint;
    UFUNCTION(BlueprintPure,Category="IC|Input") FString MenuCancelLabel() const;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Items") TSubclassOf<AICDroppedItem> DroppedItemClass;
    bool IsQuitAfterSave() const { return bExitAfterSave; }
    UFUNCTION(BlueprintCallable,Category="IC|Session") void OpenSaveMenu(bool bQuitAfter=false);
    UFUNCTION(BlueprintCallable,Category="IC|Session") void OpenLoadMenu();
    UFUNCTION(BlueprintCallable,Category="IC|Session") void CloseSessionMenu();
    UFUNCTION(BlueprintCallable,Category="IC|Session") void RequestManualSave();
    UFUNCTION(BlueprintCallable,Category="IC|Session") void RequestCreatePlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Session") void RequestQuit();
    UFUNCTION(BlueprintCallable,Category="IC|Session") void RequestDelete(bool bLegacy=false);
    UFUNCTION(BlueprintCallable,Category="IC|Session") void ConfirmModal(bool bAccept);
    UFUNCTION(BlueprintCallable,Category="IC|Session") void QuitWithoutSave();
    void FinishSessionSave(int32 Slot,bool bSuccess);
    AICInteractionActor* FindInteractionTarget() const;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString Menu=TEXT("Create");
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString Notice;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName OpenStorage;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName ActiveStation;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName PendingItem;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 PendingQuick=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 SelectedBag=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 SelectedStorage=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 MoveFrom=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 PendingPartyKey=INDEX_NONE;
    bool bDirty=true;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI") TObjectPtr<UICGameplayWidget> Screen;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI") TObjectPtr<UICCharacterCreateWidget> CharacterCreateScreen;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICInventoryComponent* GetInventory() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICCraftingComponent* GetCrafting() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICWorldStateSubsystem* GetWorldState() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICTimeSubsystem* GetTime() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICSaveSubsystem* GetSaves() const;
    UFUNCTION(BlueprintPure) FICCharacterState GetPlayerSnapshot() const;
    UFUNCTION(BlueprintPure,Category="IC|Creation") bool CanEditCreation() const { return !bCreated && !bInitialSavePending; }
    UFUNCTION(BlueprintPure,Category="IC|Creation") bool CanSubmitCreation() const;
    bool ValidateCreationRequest(FString& OutReason) const;
    void ReportCreationFailure(const FString& Reason);
    UFUNCTION(BlueprintCallable) void SetCombatState(bool Active);
    UFUNCTION(BlueprintCallable) void SetFinalMoveSpeed(float Speed);
    UFUNCTION(BlueprintCallable,Category="IC|Player") bool SetGold(int32 Amount);
    UFUNCTION(BlueprintPure,Category="IC|Player") bool IsDead() const { return bDead; }
    UFUNCTION(BlueprintCallable) void ResolveEffect(FGuid RequestID,bool Success);
    UFUNCTION(BlueprintCallable) void PlayerDied();
    UFUNCTION(BlueprintCallable) float ConsumeProductionSP(float Amount);
    UFUNCTION(BlueprintCallable) void ApplyGameMinutes(int32 Minutes);
    UFUNCTION() void HandleGameMinutesAdvanced(int32 Minutes);
    UFUNCTION(BlueprintCallable) bool Sleep();
    UFUNCTION(BlueprintCallable) bool CraftRecipe(FName RecipeID);
    UFUNCTION(BlueprintCallable) void OpenContainer(FName ContainerID,bool bPlayerHome,bool bAccessGranted);
    UFUNCTION(BlueprintCallable,Category="IC|Input") void HandleInputCommand(EICInputCommand Command,int32 Slot=0);
    UFUNCTION(BlueprintCallable,Category="IC|Input") void ReleaseInputCommand(EICInputCommand Command,int32 Slot=0);
    UFUNCTION(BlueprintCallable,Category="IC|UI") void ExecuteUI(const FString& Command,int32 Index=0);
    UFUNCTION(BlueprintCallable,Category="IC|UI") void SetMenu(const FString& NewMenu);
    UFUNCTION(BlueprintCallable,Category="IC|UI") void Show(const FString& Text);
    UFUNCTION(BlueprintCallable,Category="IC|Session") bool SavePlayer();
    bool AutoSavePlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Session") bool LoadPlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Items") void BeginItem(FName ID,int32 Quick=-1);
    UFUNCTION(BlueprintCallable,Category="IC|Items") void ConfirmTarget(FName TargetID);
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
    void UpdateGamePhase();
    void TryInitializeGameplay();
    void BindPawnSystems();
    void UnbindPawnSystems();
    void UpdateScreens();
    void ApplyInputMapping();
    void RemoveInputMapping();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void CancelInputCommand(EICInputCommand Command,int32 Slot);
    void ResetHeldInput();
    bool bSprintKeyHeld=false;
    bool bTargetKeyHeld=false;
    bool bTargetHoldHandled=false;
    double SprintPressedAt=0;
    double TargetPressedAt=0;
    bool CanHandleGameplayInput() const;
    bool bGameplayInitialized=false;
    UPROPERTY(Transient) TObjectPtr<UICInventoryComponent> BoundInventory;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> AppliedMappingContext;
};
