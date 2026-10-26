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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life") EICDefinitionType LifeType=EICDefinitionType::Resource;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life") FName DefinitionID;
    /** Give each placed plot/resource a stable unique ID. Falls back to its level actor name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life") FName PointID;
    /** Test-map display only. Planting still begins with the invisible seed stage. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life|Preview") bool bPreviewUnplantedCrop=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Life|Presentation") bool bShowLifeLabel=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UWidgetComponent> LifeLabel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UStaticMeshComponent> VisualMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USkeletalMeshComponent> AnimalMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UGeometryCollectionComponent> OreMesh;
    UFUNCTION(BlueprintCallable, Category="IC|Life") bool Plant(AICPlayerController* Controller);
    UFUNCTION(BlueprintCallable, Category="IC|Life") void RefreshLife();
    UFUNCTION(BlueprintPure, Category="IC|Life") int32 GetGrowthStage() const;
    UFUNCTION(BlueprintPure, Category="IC|Life") bool IsHarvestable() const;
    static int32 ResolveGrowthStage(const FICCropRow& Crop,double ElapsedMinutes);
private:
    friend class UICGatheringSubsystem;
    bool CompleteResourceGather(AICPlayerController* Controller, FText& Failure);
    FName PointKey() const;
    FTimerHandle RefreshTimer;
    int32 PresentedStage=INDEX_NONE;
};
