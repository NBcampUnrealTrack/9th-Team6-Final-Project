#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICPlayerState.generated.h"

class UICWorldStateSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICPlayerSnapshotChanged);


UCLASS(Blueprintable)
class ISEKAICLASS_API AICPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure, Category="IC|Player") FICCharacterState GetPlayerSnapshot() const;
    UFUNCTION(BlueprintPure, Category="IC|Player") FICInventoryState GetInventorySnapshot() const;
    UFUNCTION(BlueprintPure, Category="IC|Player") int32 GetGold() const;
    UFUNCTION(BlueprintPure, Category="IC|Player") bool IsPlayerCreated() const;
    UPROPERTY(BlueprintAssignable, Category="IC|Player") FICPlayerSnapshotChanged OnSnapshotChanged;
private:
    UICWorldStateSubsystem* GetWorldState() const;
    UFUNCTION() void HandleCharacterChanged(FName CharacterId);
    UFUNCTION() void HandleSnapshotChanged();
};
