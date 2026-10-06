#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveSubsystem.generated.h"

class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICSaveGame;
class UICLegacyPlayerSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICSaveStarted, int32, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FICSaveFinished, int32, Slot, bool, bSuccess, const FString&, Message);

UCLASS()
class ISEKAICLASS_API UICSaveSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override;

    UPROPERTY(BlueprintAssignable) FICSaveStarted OnSaveStarted;
    UPROPERTY(BlueprintAssignable) FICSaveFinished OnSaveFinished;
    UPROPERTY(BlueprintReadOnly) bool bSaving = false;
    UPROPERTY(BlueprintReadOnly) bool bAutoSavePending = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Save", meta=(ClampMin="1")) double AutoSaveInterval = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Save", meta=(ClampMin="0", ClampMax="1")) float AutoSaveHealthThreshold = 0.25f;
    UFUNCTION(BlueprintCallable, Category="IC|Save") void SetSafetyState(const FICSaveSafety& NewSafety) { Safety = NewSafety; }
    UFUNCTION(BlueprintPure, Category="IC|Save") bool CanSave(bool bAutoSave) const;
    UFUNCTION(BlueprintCallable, Category="IC|Save") bool SaveSlot(int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|Save") bool LoadSlot(int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|Save") void RequestAutoSave(FName Reason);
    UFUNCTION(BlueprintPure, Category="IC|Save") TArray<FICSaveSlotInfo> GetSlotList() const;
    UFUNCTION(BlueprintCallable, Category="IC|Save") bool ImportLegacySave();
    UFUNCTION(BlueprintCallable, Category="IC|Save") bool DeleteSlot(int32 Slot);
    UFUNCTION(BlueprintCallable, Category="IC|Save") bool DeleteLegacySave();
    UFUNCTION(BlueprintPure, Category="IC|Save") FString GetSaveBlockReason(bool bAutoSave) const;
    FString GetLegacySavePath() const;
    bool HasLegacySave() const;
    void SetAutoSaveSuspended(bool bSuspended) { bAutoSaveSuspended = bSuspended; }
    bool ConvertLegacySave(const UICLegacyPlayerSaveGame* Legacy, FICWorldState& OutState) const;
    void ResetSession();
    void PollAutoSave();
    // Explicit dependency injection and isolated slot prefixes for automated checks.
    bool ConfigureServices(UICWorldStateSubsystem* InState, UICTimeSubsystem* InTime, const FString& InPrefix);
    bool SaveSlotSynchronously(int32 Slot);
    FString GetSlotPath(int32 Slot) const;
    // Region/quest code updates this whenever the player's displayed location changes.
    UFUNCTION(BlueprintCallable, Category="IC|Save") void SetPreviewLocation(const FString& Location) { PreviewLocation = Location.Left(128); }
    UFUNCTION(BlueprintPure, Category="IC|Save") FString GetPreviewLocation() const { return PreviewLocation; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Save") bool bCaptureSaveThumbnail = true;
    FString GetLastMessage() const { return LastMessage; }
private:
    FString PreviewLocation;
    bool CaptureThumbnail(TArray<uint8>& OutPNG);
    UPROPERTY() TObjectPtr<UICWorldStateSubsystem> WorldState;
    UPROPERTY() TObjectPtr<UICTimeSubsystem> Time;
    FICSaveSafety Safety;
    FString Prefix = TEXT("IC_Team");
    FString LastMessage;
    double LastSavePlaySecond = 0;
    double RetryAfterPlaySecond = 0;
    FName PendingReason;
    uint64 AutoRequestGeneration = 0;
    bool bAutoSaveSuspended = false;
    bool PrepareSave(int32 Slot, TArray<uint8>& Bytes, double& CapturedPlaySecond, bool& bRotateBackup);
    UICSaveGame* ReadSlot(int32 Slot, bool bAllowBackup, bool& bUsedBackup) const;
    bool ValidateSave(const UICSaveGame* Save, int32 Slot) const;
    void FinishSave(int32 Slot, bool bSuccess, double CapturedPlaySecond, uint64 CapturedGeneration, const FString& Message);
};
