#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICTimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FICTimeJumped, double, FromGameMinute, double, ToGameMinute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICClockChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICNaturalMinutesAdvanced,int32,Minutes);

UCLASS()
class ISEKAICLASS_API UICTimeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICTimeJumped OnTimeJumped;
    UPROPERTY(BlueprintAssignable) FICClockChanged OnClockChanged;
    UPROPERTY(BlueprintAssignable) FICNaturalMinutesAdvanced OnGameMinutesAdvanced;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override;
    UFUNCTION(BlueprintPure, Category="IC|Time") FICClockState GetClock() const { return Clock; }
    UFUNCTION(BlueprintCallable, Category="IC|Time") void SetRunning(bool Running) { bRunning = Running; }
    UFUNCTION(BlueprintCallable, Category="IC|Time") void SetMenuPaused(bool Paused) { bMenuPaused = Paused; }
    UFUNCTION(BlueprintPure, Category="IC|Time") bool IsRunning() const { return bRunning && !bMenuPaused; }
    UFUNCTION(BlueprintCallable, Category="IC|Time") bool JumpGameMinutes(double Minutes);
    bool RestoreClock(const FICClockState& Saved);
    void ResetClock();
    void AdvanceClock(double DeltaSeconds, bool bWorldPaused);
private:
    UPROPERTY() FICClockState Clock;
    bool bRunning = false;
    bool bMenuPaused = true;
};
