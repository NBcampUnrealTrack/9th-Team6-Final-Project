#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Core/ICStateParticipant.h"
#include "PlayerPrototypeController.generated.h"

class APlayerPrototypeObject;
class UPlayerInventory;
class UPlayerPrototypeWidget;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICSaveSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerCombatRequest,FName,Action,int32,Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerEffectRequest,FGuid,RequestID,FName,ItemID,FName,TargetID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerInitialized,FName,JobID,FName,FirstSkillID);

UCLASS()
class ISEKAICLASS_API APlayerPrototypeController : public APlayerController, public IICStateParticipant
{
    GENERATED_BODY()
public:
    APlayerPrototypeController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    UPROPERTY(BlueprintAssignable) FPlayerCombatRequest OnCombatRequest;
    UPROPERTY(BlueprintAssignable) FPlayerEffectRequest OnEffectRequest;
    UPROPERTY(BlueprintAssignable) FPlayerInitialized OnPlayerInitialized;
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
    FName PendingItem;
    int32 PendingQuick=INDEX_NONE;
    int32 SelectedBag=INDEX_NONE;
    int32 SelectedStorage=INDEX_NONE;
    int32 MoveFrom=INDEX_NONE;
    bool bDirty=true;
    UPROPERTY() TObjectPtr<UPlayerPrototypeWidget> Screen;
    UPlayerInventory* GetInventory() const;
    UICWorldStateSubsystem* GetWorldState() const;
    UICTimeSubsystem* GetTime() const;
    UICSaveSubsystem* GetSaves() const;
    UFUNCTION(BlueprintPure) FICCharacterState GetPlayerSnapshot() const;
    bool CanEditCreation() const { return !bCreated && !bInitialSavePending; }
    UFUNCTION(BlueprintCallable) void SetCombatState(bool Active);
    UFUNCTION(BlueprintCallable) void SetFinalMoveSpeed(float Speed);
    UFUNCTION(BlueprintCallable) void ResolveEffect(FGuid RequestID,bool Success);
    UFUNCTION(BlueprintCallable) void PlayerDied();
    UFUNCTION(BlueprintCallable) void OpenContainer(FName ContainerID,bool bPlayerHome,bool bAccessGranted);
    void HandleKey(FKey Key);
    void ExecuteUI(const FString& Command,int32 Index);
    void SetMenu(const FString& NewMenu);
    void Show(const FString& Text);
    bool SavePlayer();
    bool LoadPlayer();
    void BeginItem(FName ID,int32 Quick=INDEX_NONE);
    void ConfirmTarget(FName TargetID);
    void SyncSaveSafety();
    void RefreshSaveSlots();
    UFUNCTION() void InventoryChanged();
    UFUNCTION() void InventoryMessage(const FText& Message);
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
    void CreatePlayer();
    void CancelTarget();
    void Interact();
    void StartSmokeTest();
    void UpdateGamePhase();
};
