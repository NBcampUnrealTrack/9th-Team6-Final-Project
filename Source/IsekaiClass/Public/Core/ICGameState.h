#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ICGameState.generated.h"
UENUM(BlueprintType)
enum class EICGamePhase : uint8 { CharacterCreation , Playing , Menu , Dead  };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerTimeJumped,int32,FromMinute,int32,ToMinute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerGameMinutesAdvanced,int32,Minutes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICGamePhaseChanged,EICGamePhase,Phase);
class UICTimeSubsystem;
UCLASS(Blueprintable)
class ISEKAICLASS_API AICGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    AICGameState();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(BlueprintReadOnly) EICGamePhase Phase=EICGamePhase::CharacterCreation;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Storage") FName SharedHomeStorageID=TEXT("PlayerHome");
    UPROPERTY(BlueprintReadOnly) int32 GameMinute=480;
    UPROPERTY(BlueprintAssignable) FICGamePhaseChanged OnPhaseChanged;
    UPROPERTY(BlueprintAssignable) FPlayerTimeJumped OnTimeJumped;
    UPROPERTY(BlueprintAssignable) FPlayerGameMinutesAdvanced OnGameMinutesAdvanced;
    UFUNCTION(BlueprintCallable) void SetPhase(EICGamePhase NewPhase);
    UFUNCTION(BlueprintCallable) void AdvanceGameMinutes(int32 Minutes);
private:
    UICTimeSubsystem* GetTime() const;
    UFUNCTION() void RefreshClock();
    UFUNCTION() void HandleMinutesAdvanced(int32 Minutes);
    UFUNCTION() void HandleTimeJumped(double From,double To);
};
