#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICTimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FICTimeJumped, double, FromGameMinute, double, ToGameMinute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICClockChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICNaturalMinutesAdvanced,int32,Minutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICPlaySecondsAdvanced,double,Seconds);

UCLASS()
class ISEKAICLASS_API UICTimeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICTimeJumped OnTimeJumped;
    UPROPERTY(BlueprintAssignable) FICClockChanged OnClockChanged;
    UPROPERTY(BlueprintAssignable) FICNaturalMinutesAdvanced OnGameMinutesAdvanced;
    UPROPERTY(BlueprintAssignable) FICPlaySecondsAdvanced OnPlaySecondsAdvanced;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { IC_TRACE_FUNCTION(); return true; }
    virtual UWorld* GetTickableGameObjectWorld() const override { IC_TRACE_FUNCTION(); return GetWorld(); }
    virtual TStatId GetStatId() const override;
    UFUNCTION(BlueprintPure, Category="IC|Time") FICClockState GetClock() const { IC_TRACE_FUNCTION(); return Clock; }
    UFUNCTION(BlueprintCallable, Category="IC|Time") void SetRunning(bool Running);
    UFUNCTION(BlueprintCallable, Category="IC|Time") void SetMenuPaused(bool Paused);
    UFUNCTION(BlueprintPure, Category="IC|Time") bool IsRunning() const { IC_TRACE_FUNCTION(); return bRunning && !bMenuPaused && PauseRequests.IsEmpty(); }

    UFUNCTION(BlueprintCallable, Category="IC|Time") void SetPauseRequest(UObject* Owner, FName Reason, bool Paused);
    UFUNCTION(BlueprintCallable, Category="IC|Time") void ReleasePauseRequests(UObject* Owner);
    UFUNCTION(BlueprintCallable, Category="IC|Time") void PrepareForTravel();
    UFUNCTION(BlueprintCallable, Category="IC|Time") bool JumpGameMinutes(double Minutes);
    bool RestoreClock(const FICClockState& Saved);
    void ResetClock();
    void AdvanceClock(double DeltaSeconds, bool bWorldPaused);
private:
    struct FPauseRequest { TWeakObjectPtr<UObject> Owner; FName Reason; };
    TArray<FPauseRequest> PauseRequests;
    void UpdateWorldPause();
    TWeakObjectPtr<UWorld> PausedWorld;
    bool bOwnsWorldPause = false;
    UPROPERTY() FICClockState Clock;
    bool bRunning = false;
    bool bMenuPaused = true;
};
