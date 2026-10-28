#pragma once
#include "CoreMinimal.h"
#include "Life/World/ICLifeActor.h"
#include "ICPlayerHomeActors.generated.h"
UCLASS(Blueprintable)
class ISEKAICLASS_API AICFarmPlot : public AICLifeActor
{
    GENERATED_BODY()
public:
    AICFarmPlot();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Soil;
};
UCLASS(Blueprintable)
class ISEKAICLASS_API AICPlayerBed : public AICInteractionActor
{
    GENERATED_BODY()
public:
    AICPlayerBed();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> BedVisual;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Sleep") FTransform SleepPoseOffset;
};
UCLASS(Blueprintable)
class ISEKAICLASS_API AICPlayerStorage : public AICInteractionActor
{
    GENERATED_BODY()
public:
    AICPlayerStorage();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UICChestPresentationComponent> ChestPresentation;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ClampMax="2")) int32 RequiredExtraStorage=0;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool TryInteract_Implementation(AICPlayerController* Controller) override;
    UFUNCTION(BlueprintCallable) void RefreshAvailability();
private: FTimerHandle RefreshTimer;
};
