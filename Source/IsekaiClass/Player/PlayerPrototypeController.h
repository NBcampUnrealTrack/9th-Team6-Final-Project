#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerPrototypeController.generated.h"
class UPlayerInventory;
class UPlayerPrototypeWidget;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerCombatRequest,FName,Action,int32,Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerEffectRequest,FGuid,RequestID,FName,ItemID,FName,TargetID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerInitialized,FName,JobID,FName,FirstSkillID);
UCLASS()
class ISEKAICLASS_API APlayerPrototypeController : public APlayerController
{
    GENERATED_BODY()
public:
    APlayerPrototypeController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    UPROPERTY(BlueprintAssignable) FPlayerCombatRequest OnCombatRequest;
    UPROPERTY(BlueprintAssignable) FPlayerEffectRequest OnEffectRequest;
    UPROPERTY(BlueprintAssignable) FPlayerInitialized OnPlayerInitialized;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prototype") bool bUseTestEffects=true;
    UPROPERTY(BlueprintReadOnly) FString PlayerName;
    UPROPERTY(BlueprintReadOnly) int32 Preset=0;
    UPROPERTY(BlueprintReadOnly) int32 Job=0;
    UPROPERTY(BlueprintReadOnly) FName FirstSkill;
    UPROPERTY(BlueprintReadOnly) bool bCreated=false;
    UPROPERTY(BlueprintReadOnly) bool bInCombat=false;
    UPROPERTY(BlueprintReadWrite) bool bInDialogue=false;
    UPROPERTY(BlueprintReadWrite) bool bInEvent=false;
    UPROPERTY(BlueprintReadOnly) bool bBlocking=false;
    UPROPERTY(BlueprintReadOnly) float TestHP=60;
    UPROPERTY(BlueprintReadOnly) float TestSP=70;
    UPROPERTY(BlueprintReadOnly) int32 TestStrength=1;
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
    UFUNCTION(BlueprintCallable) void SetCombatState(bool Active) { bInCombat=Active; }
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
    UFUNCTION() void InventoryChanged();
    UFUNCTION() void InventoryMessage(const FText& Message);
private:
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
