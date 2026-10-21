#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ICProductionSubsystem.generated.h"
class UICDefinitionSubsystem;
class UICWorldStateSubsystem;
class UICTimeSubsystem;
class UICInventoryComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FICProductionChanged,FGuid,JobID,const FString&,Message);

UCLASS()
class ISEKAICLASS_API UICProductionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void ConfigureServices(UICDefinitionSubsystem* Definitions,UICWorldStateSubsystem* State,UICTimeSubsystem* Clock);
    UPROPERTY(BlueprintAssignable) FICProductionChanged OnProductionChanged;
    UFUNCTION(BlueprintCallable,Category="IC|Production") bool StartRecipe(FName RecipeID,FName WorkAnchorID,UICInventoryComponent* Inventory,FGuid& JobID,FString& Reason);
    UFUNCTION(BlueprintCallable,Category="IC|Production") bool CompleteJob(FGuid JobID,UICInventoryComponent* Inventory,FString& Reason);
    UFUNCTION(BlueprintCallable,Category="IC|Production") bool CancelJob(FGuid JobID);
    UFUNCTION() void PlaySecondsAdvanced(double Seconds);
private:
    UPROPERTY() TObjectPtr<UICDefinitionSubsystem> Definitions;
    UPROPERTY() TObjectPtr<UICWorldStateSubsystem> State;
    UPROPERTY() TObjectPtr<UICTimeSubsystem> Clock;
    TWeakObjectPtr<UICInventoryComponent> RuntimeInventory;
};
