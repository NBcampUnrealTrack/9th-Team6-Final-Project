#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerTimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerLegacyTimeJumped,int32,FromMinute,int32,ToMinute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICGameMinutesAdvanced,int32,Minutes);

UCLASS()
class ISEKAICLASS_API UPlayerTimeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerTimeSubsystem,STATGROUP_Tickables); }
    virtual void Tick(float DeltaSeconds) override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override { return WorldType==EWorldType::Game||WorldType==EWorldType::PIE; }
    UPROPERTY(BlueprintReadOnly) int32 GameMinute=480;
    UPROPERTY(BlueprintReadOnly) float PlaySeconds=0.f;
    UPROPERTY(BlueprintAssignable) FPlayerLegacyTimeJumped OnTimeJumped;
    UPROPERTY(BlueprintAssignable) FICGameMinutesAdvanced OnGameMinutesAdvanced;
    UFUNCTION(BlueprintCallable) void AdvanceGameMinutes(int32 Minutes);
    UFUNCTION(BlueprintCallable) void Restore(int32 InGameMinute,float InPlaySeconds);
private:
    float GameMinuteAccumulator=0.f;
};
