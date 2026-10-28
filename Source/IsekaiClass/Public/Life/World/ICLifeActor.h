#pragma once
#include "CoreMinimal.h"
#include "Interaction/ICInteractionActor.h"
#include "Data/Rows/Life/ICCropRow.h"
#include "ICLifeActor.generated.h"

/** Placeable presentation for a life DT entry; crops use the saved game clock. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICLifeActor : public AICInteractionActor
{
    GENERATED_BODY()
public:
    AICLifeActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool TryInteract_Implementation(AICPlayerController* Controller) override;
    virtual FText GetInteractionText() const override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life") EICDefinitionType LifeType=EICDefinitionType::Resource;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life") FName DefinitionID;
    /** Give each placed plot/resource a stable unique ID. Falls back to its level actor name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life") FName PointID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life") bool bPlantingPlot=false;
    /** Show the first stage before a player has created the saved world state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life|Preview") bool bPreviewUnplantedCrop=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life|Presentation") bool bShowLifeLabel=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UWidgetComponent> LifeLabel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UStaticMeshComponent> VisualMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USkeletalMeshComponent> AnimalMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UGeometryCollectionComponent> OreMesh;
    UFUNCTION(BlueprintCallable, Category="IC|Life") bool Plant(AICPlayerController* Controller);
    UFUNCTION(BlueprintCallable, Category="IC|Life") virtual void RefreshLife();
    bool CompletePlayerAction(AICPlayerController* Player,FName CropToPlant,FText& Failure);
    UFUNCTION(BlueprintPure, Category="IC|Life") int32 GetGrowthStage() const;
    UFUNCTION(BlueprintPure, Category="IC|Life") bool IsHarvestable() const;
    UFUNCTION(BlueprintPure, Category="IC|Life") bool IsResourceAvailable() const;
    /** Uses the placed water mesh bounds, so editing the box/plane also edits the catch area. */
    UFUNCTION(BlueprintPure, Category="IC|Fishing") bool ContainsFishingLanding(FVector Point) const;
    void GetFishingSurface(FVector& Center,FVector& Normal) const;
    static int32 ResolveGrowthStage(const FICCropRow& Crop,double ElapsedMinutes);
private:
    friend class UICGatheringSubsystem;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FICLifeCalendarCheck;
#endif
    bool CompleteResourceGather(AICPlayerController* Controller, FText& Failure);
    FName PointKey() const;
    FTimerHandle RefreshTimer;
    int32 PresentedStage=INDEX_NONE;
    UFUNCTION() void GameTimeJumped(double From,double To);
    void EnsureNaturalCrop();
};
