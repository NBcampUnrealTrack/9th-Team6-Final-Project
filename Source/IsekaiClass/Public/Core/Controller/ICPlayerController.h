#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "Life/Survival/ICPlayerSurvival.h"
#include "World/State/ICStateParticipant.h"
#include "Input/ICInputTypes.h"
#include "ICPlayerController.generated.h"

class AICInteractionActor;
class AICNPCBase;
class AICDroppedItem;
class UICCraftingComponent;
class UICInventoryComponent;
class UICGameplayWidget;
class UICDeathWidget;
class UICQuestTrackerWidget;
class UICNPCDialogueWidget;
class UICMainMenuWidget;
class UICCharacterCreateWidget;
class UICSaveLoadWidget;
class UICSettingsWidget;
class UICLoadingWidget;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICSaveSubsystem;
class UInputMappingContext;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerCombatRequest,FName,Action,int32,Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerInitialized,FName,JobID,FName,FirstSkillID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerStatBookApplied,FName,TargetID,EPlayerMainStat,Stat,int32,Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerExhaustionChanged,bool,bExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerSleepRequested,int32,GameMinutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerEquipmentStatsChanged,FPlayerEquipmentStats,Stats);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICMenuChanged,const FString&,Menu);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICNoticeChanged,const FString&,Notice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICSaveSessionChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICFishingInputRequested);

UCLASS(Blueprintable)
class ISEKAICLASS_API AICPlayerController : public APlayerController, public IICStateParticipant
{
    GENERATED_BODY()
public:
    AICPlayerController();
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|UI") TSubclassOf<UICSaveLoadWidget> SaveLoadWidgetClass;
    UPROPERTY(BlueprintAssignable,Category="IC|Session") FICSaveSessionChanged OnSaveSessionChanged;
    UFUNCTION(BlueprintCallable,Category="IC|Session") void SelectSessionSlot(int32 Slot);
    UFUNCTION(BlueprintCallable,Category="IC|Session") void RequestLoadPlayer();
    UFUNCTION(BlueprintPure,Category="IC|Session") FString GetSessionMenu() const;
    UFUNCTION(BlueprintPure,Category="IC|Session") int32 GetSelectedSessionSlot() const;
    int32 GetPendingSessionSlot() const { IC_TRACE_FUNCTION();return ModalSlot; }
    void SetSessionGameplayMap(TSoftObjectPtr<UWorld> Map) { IC_TRACE_FUNCTION();SessionGameplayMap=Map; }
    bool HandleSessionKey(const FKey& Key,bool bRepeat=false);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Main Menu")
    TSubclassOf<UICMainMenuWidget> MainMenuWidgetClass;
    UFUNCTION(BlueprintPure, Category="IC|Main Menu")
    bool IsMainMenuLevel() const;
    UFUNCTION(BlueprintCallable, Category="IC|Main Menu")
    bool ContinueLatestSave();
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|New Game") FName NewGameDefinitionID=TEXT("CFG_NEWGAME_001");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    TSubclassOf<UICCharacterCreateWidget> CharacterCreateWidgetClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|UI") TSubclassOf<UICGameplayWidget> GameplayWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="IC|UI") TSoftClassPtr<UICQuestTrackerWidget> QuestTrackerWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="IC|UI") TSubclassOf<UICNPCDialogueWidget> DialogueWidgetClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TObjectPtr<UInputMappingContext> DefaultMappingContext;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TSoftObjectPtr<UInputMappingContext> GamepadMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Input/Contexts/IMC_Gamepad.IMC_Gamepad")));
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") int32 MappingPriority=0;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TObjectPtr<UInputAction> LookAction;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Input") TArray<FICInputCommandBinding> CommandActions;
    UPROPERTY(BlueprintAssignable, Category="IC|Fishing") FICFishingInputRequested OnFishingRequested;
    UFUNCTION(BlueprintCallable, Category="IC|Fishing") void RequestFishing();
    UFUNCTION(BlueprintPure, Category="IC|Input") bool HasTargetSelection() const { return !PendingItem.IsNone() || !PendingSkill.IsNone(); }
    UFUNCTION(BlueprintCallable, Category="IC|Input") bool SelectTarget(FName CharacterID);
    UFUNCTION(BlueprintCallable, Category="IC|Input") void ConfirmSelectedTarget();
    UFUNCTION(BlueprintPure, Category="IC|Weapons") FName GetCurrentWeaponSkill() const;
    UFUNCTION(BlueprintPure, Category="IC|Weapons") double GetCurrentWeaponSkillCooldown() const;
    UFUNCTION(BlueprintCallable, Category="IC|Weapons") bool EquipWeaponSkill(EICWeaponFamily Family, FName SkillID);
    UPROPERTY(BlueprintAssignable,Category="IC|UI") FICMenuChanged OnMenuChanged;
    UPROPERTY(BlueprintAssignable,Category="IC|UI") FICNoticeChanged OnNoticeChanged;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void FlushPressedKeys() override;
    void RefreshUserInputMapping();
    virtual void PlayerTick(float DeltaSeconds) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void ReceivedPlayer() override;
    UPROPERTY(BlueprintAssignable) FPlayerCombatRequest OnCombatRequest;
    UPROPERTY(BlueprintAssignable) FPlayerInitialized OnPlayerInitialized;
    UPROPERTY(BlueprintAssignable) FPlayerStatBookApplied OnStatBookApplied;
    UPROPERTY(BlueprintAssignable) FPlayerExhaustionChanged OnExhaustionChanged;
    UPROPERTY(BlueprintAssignable) FPlayerSleepRequested OnSleepRequested;
    UPROPERTY(BlueprintAssignable) FPlayerEquipmentStatsChanged OnEquipmentStatsChanged;

    UPROPERTY(BlueprintReadOnly) FString PlayerName;
    UPROPERTY(BlueprintReadOnly) int32 Preset=0;
    UPROPERTY(BlueprintReadOnly) int32 Job=0;
    UPROPERTY(BlueprintReadOnly) FName FirstSkill;
    UPROPERTY(BlueprintReadOnly) bool bCreated=false;
    UPROPERTY(BlueprintReadOnly) bool bInCombat=false;
    UPROPERTY(BlueprintReadWrite) bool bInDialogue=false;
    UPROPERTY(BlueprintReadWrite) bool bInEvent=false;
    UPROPERTY(BlueprintReadOnly) FPlayerSurvivalState Survival;
    UFUNCTION(BlueprintPure, Category="IC|Survival") int32 GetDisplayedSP() const { IC_TRACE_FUNCTION(); return FMath::FloorToInt(FMath::Max(0.f,Survival.SP)); }
    UFUNCTION(BlueprintPure, Category="IC|Survival") int32 GetDisplayedMaxSP() const { IC_TRACE_FUNCTION(); return FMath::FloorToInt(FMath::Max(0.f,Survival.MaxSP)); }
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Survival") float SleepSPRecoveryPerGameMinute=1.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Survival") float ExhaustedMoveSpeedMultiplier=0.65f;
    UPROPERTY(BlueprintReadOnly) FPlayerMainStats MainStats;
    UPROPERTY(BlueprintReadOnly) int32 Gold=200;
    UPROPERTY(BlueprintReadOnly) int32 SelectedSaveSlot=1;
    UPROPERTY(BlueprintReadOnly) int32 SelectedLoadSlot=0;
    UPROPERTY(BlueprintReadOnly) TArray<FICSaveSlotInfo> SaveSlots;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString ModalText;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI") FText ModalDisplayText;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString InteractionHint;
    UFUNCTION(BlueprintPure,Category="IC|Input") FString MenuCancelLabel() const;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Items") TSubclassOf<AICDroppedItem> DroppedItemClass;
    bool IsQuitAfterSave() const { IC_TRACE_FUNCTION(); return bExitAfterSave; }
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
    AICNPCBase* FindNPCInteractionTarget() const;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString Menu=TEXT("Create");
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FString Notice;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI") FText NoticeDisplayText;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName OpenStorage;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName ActiveStation;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName ActiveWorkAnchor;
    UFUNCTION() void ProductionChanged(FGuid JobID,const FString& Message);
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName PendingItem;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName PendingSkill;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") FName PendingTargetID;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 PendingQuick=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 SelectedBag=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 SelectedStorage=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 MoveFrom=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") int32 PendingPartyKey=INDEX_NONE;
    bool bDirty=true;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI") TObjectPtr<UICGameplayWidget> Screen;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI") TObjectPtr<UICDeathWidget> DeathScreen;
    UPROPERTY(Transient) TObjectPtr<UICQuestTrackerWidget> QuestTrackerScreen;
    UPROPERTY(Transient) TObjectPtr<UICNPCDialogueWidget> DialogueScreen;
    UPROPERTY(Transient) TObjectPtr<UICSettingsWidget> SettingsScreen;
    UPROPERTY(Transient,BlueprintReadOnly,Category="IC|UI") TObjectPtr<UICCharacterCreateWidget> CharacterCreateScreen;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICInventoryComponent* GetInventory() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICCraftingComponent* GetCrafting() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICWorldStateSubsystem* GetWorldState() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICTimeSubsystem* GetTime() const;
    UFUNCTION(BlueprintPure,Category="IC|Systems") UICSaveSubsystem* GetSaves() const;
    UFUNCTION(BlueprintPure) FICCharacterState GetPlayerSnapshot() const;
    UFUNCTION(BlueprintPure,Category="IC|Creation") bool CanEditCreation() const { IC_TRACE_FUNCTION(); return !bCreated && !bInitialSavePending; }
    UFUNCTION(BlueprintPure,Category="IC|Creation") bool CanSubmitCreation() const;

    bool DebugCreatePlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Creation") void RetryNewGame();
    bool ValidateCreationRequest(FString& OutReason) const;
    void ReportCreationFailure(const FString& Reason);
    UFUNCTION(BlueprintCallable) void SetCombatState(bool Active);
    UFUNCTION(BlueprintCallable) void SetFinalMoveSpeed(float Speed);
    UFUNCTION(BlueprintCallable,Category="IC|Player") bool SetGold(int32 Amount);
    UFUNCTION(BlueprintPure,Category="IC|Player") bool IsDead() const { IC_TRACE_FUNCTION(); return bDead; }
    UFUNCTION(BlueprintCallable) void PlayerDied();
    UFUNCTION(BlueprintCallable) float ConsumeProductionSP(float Amount);
    UFUNCTION() void HandlePlaySecondsAdvanced(double Seconds);
    UFUNCTION(BlueprintCallable) bool Sleep();
    UFUNCTION(BlueprintCallable) bool CraftRecipe(FName RecipeID);
    UFUNCTION(BlueprintCallable) void OpenContainer(FName ContainerID,bool bPlayerHome,bool bAccessGranted);
    UFUNCTION(BlueprintCallable,Category="IC|Input") void HandleInputCommand(EICInputCommand Command,int32 Slot=0);
    UFUNCTION(BlueprintCallable,Category="IC|Input") void ReleaseInputCommand(EICInputCommand Command,int32 Slot=0);
    UFUNCTION(BlueprintCallable,Category="IC|UI") void ExecuteUI(const FString& Command,int32 Index=0);
    UFUNCTION(BlueprintCallable,Category="IC|UI") void SetMenu(const FString& NewMenu);
    UFUNCTION(BlueprintCallable,Category="IC|UI") void Show(const FString& Text);
    void ShowLocalized(const FText& Text);
    UFUNCTION(BlueprintCallable,Category="IC|Session") bool SavePlayer();
    bool AutoSavePlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Session") bool LoadPlayer();
    UFUNCTION(BlueprintCallable,Category="IC|Items") void BeginItem(FName ID,int32 Quick=-1);
    UFUNCTION(BlueprintCallable,Category="IC|Items") void ConfirmTarget(FName TargetID);
    UFUNCTION(BlueprintCallable,Category="IC|Items") void ConfirmPartyTarget(int32 Slot);
    void SyncSaveSafety();
    void RefreshSaveSlots();
    UFUNCTION() void InventoryChanged();
    UFUNCTION() void InventoryMessage(const FText& Message);
    UFUNCTION() void InventoryLootResult(FName ItemID,int32 Accepted,int32 Overflow);
    UFUNCTION() void SynchronizeDrops();
    UFUNCTION() void CharacterChanged(FName ID);
    UFUNCTION() void ClockChanged();
    UFUNCTION() void SaveStarted(int32 Slot);
    UFUNCTION() void SaveFinished(int32 Slot,bool bSuccess,const FString& Message);
    virtual bool ReadLiveState_Implementation(FName CharacterId,FICCharacterState& Snapshot) const override;
    virtual bool FlushToWorldState_Implementation(UICWorldStateSubsystem* WorldState) override;
    virtual bool CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
private:
    friend class UICGatheringSubsystem;
    TSoftObjectPtr<UWorld> SessionGameplayMap;
    UPROPERTY(Transient) TObjectPtr<UICSaveLoadWidget> SessionSaveLoadScreen;
    bool LoadSelectedSessionSlot();
    void ExecuteCombatInput(FName Action,int32 Slot);
    void SelectCombatTarget(bool Next);
    void BeginSkillTarget(class AICCharacterBase* Caster, FName SkillID);
    TWeakObjectPtr<class AICCharacterBase> PendingSkillCaster;
    FString LastEquipmentKey;
    TWeakObjectPtr<class AICCharacterBase> CombatTarget;
    double TargetOccludedAt=-1;
    UFUNCTION() void CombatCacheChanged();
    void InitializeMainMenu();
    UPROPERTY(Transient)
    TObjectPtr<UICMainMenuWidget> MainMenuWidget;
    UPROPERTY(Transient) TObjectPtr<UICLoadingWidget> LoadingScreen;
    FString SaveReturnMenu;
    FString SettingsReturnMenu;
    FString LoadReturnMenu=TEXT("Create");
    FString QuitReturnMenu;
    FString ModalReturnMenu;
    FString ModalAction;
    int32 ModalSlot=INDEX_NONE;
    bool bExitAfterSave=false;
    bool bQuitAfterWrite=false;
    bool bSessionClosing=false;
    void ShowModal(const FString& Action,const FString& Text,int32 Slot=INDEX_NONE);
    void ShowModal(const FString& Action,const FText& Text,int32 Slot=INDEX_NONE);
    void CompleteQuit();
    bool bItemTransactionInProgress=false;
    bool bDead=false;
    bool bInitialSavePending=false;
    bool bLastExhausted=false;
    void CreatePlayer();
    void FlushWorldState();
    void CancelTarget();
    void Interact();
    void OpenNPCDialogue(AICNPCBase* NPC);

    UFUNCTION()
    void HandleNPCInteractionStarted(AICNPCBase* NPC, AICPlayerController* Requester, FName ServiceID);
    TMap<FGuid,TWeakObjectPtr<AICDroppedItem>> DropActors;
    void UpdateGamePhase();
    void TryInitializeGameplay();
    void BindPawnSystems();
    void UnbindPawnSystems();
    void UpdateScreens();
    void ApplyInputMapping();
    void RemoveInputMapping();
    void Move(const FInputActionValue& Value);
    void StopMove(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void ZoomCamera(const FInputActionValue& Value);
    UPROPERTY(Transient) TObjectPtr<UInputAction> CameraZoomAction;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> CameraMappingContext;
    bool bCameraLockOn=false;
    void CancelInputCommand(EICInputCommand Command,int32 Slot);
    void ResetHeldInput();
    bool CanHandleGameplayInput() const;
    bool bGameplayInitialized=false;
    UPROPERTY(Transient) TObjectPtr<UICInventoryComponent> BoundInventory;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> AppliedMappingContext;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> UserMappingContext;
};
