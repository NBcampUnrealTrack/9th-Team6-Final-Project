#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PlayerGameState.generated.h"
UENUM(BlueprintType)
enum class EPlayerGamePhase : uint8 { CharacterCreation, Playing, Menu, Dead };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerTimeJumped,int32,FromMinute,int32,ToMinute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerGameMinutesAdvanced,int32,Minutes);
UCLASS()
class ISEKAICLASS_API APlayerPrototypeGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    APlayerPrototypeGameState();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly) EPlayerGamePhase Phase=EPlayerGamePhase::CharacterCreation;
    UPROPERTY(BlueprintReadOnly) FName SharedHomeStorageID=TEXT("PlayerHome");
    UPROPERTY(BlueprintReadOnly) int32 GameMinute=480;
    UPROPERTY(BlueprintAssignable) FPlayerTimeJumped OnTimeJumped;
    UPROPERTY(BlueprintAssignable) FPlayerGameMinutesAdvanced OnGameMinutesAdvanced;
    UFUNCTION(BlueprintCallable) void SetPhase(EPlayerGamePhase NewPhase);
    UFUNCTION(BlueprintCallable) void AdvanceGameMinutes(int32 Minutes);
private:
    int32 LastBroadcastMinute=480;
};
