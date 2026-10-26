#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ICGatheringSubsystem.generated.h"

class AICLifeActor;
class AICPlayerController;
class APawn;
class UICGatherProgressWidget;

/** A cancellable gathering action. Only its final reward changes saved world state. */
UCLASS()
class ISEKAICLASS_API UICGatheringSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override;
    UFUNCTION(BlueprintCallable, Category="IC|Life") bool StartGather(AICLifeActor* Resource, AICPlayerController* Player);
    UFUNCTION(BlueprintCallable, Category="IC|Life") void CancelGather();
    UFUNCTION(BlueprintPure, Category="IC|Life") bool IsGathering() const { return ActiveResource.IsValid(); }
    UFUNCTION(BlueprintPure, Category="IC|Life") float GetProgress() const;
    UFUNCTION(BlueprintPure, Category="IC|Life") UICGatherProgressWidget* GetProgressWidget() const { return ProgressWidget; }
private:
    UFUNCTION() void OnDamaged(AActor* Victim, float Amount);
    TWeakObjectPtr<AICLifeActor> ActiveResource;
    TWeakObjectPtr<AICPlayerController> Controller;
    TWeakObjectPtr<APawn> GatheringPawn;
    UPROPERTY(Transient) TObjectPtr<UICGatherProgressWidget> ProgressWidget;
    UPROPERTY(Transient) TSubclassOf<UICGatherProgressWidget> ProgressWidgetClass;
    FVector StartLocation = FVector::ZeroVector;
    FName GatheringDefinition;
    FName GatheringPoint;
    double StartedAt = 0;
    double Duration = 5;
};
