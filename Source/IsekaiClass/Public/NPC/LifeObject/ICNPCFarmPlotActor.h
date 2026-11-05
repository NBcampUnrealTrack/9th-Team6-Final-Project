#pragma once

#include "CoreMinimal.h"
#include "NPC/LifeObject/ICNPCWorkRewardActor.h"
#include "Data/Rows/Life/ICCropRow.h"
#include "ICNPCFarmPlotActor.generated.h"

class UStaticMeshComponent;

/**
 * NPC-only field. NPCs tending a non-mature field schedule one midnight growth;
 * an NPC using a mature field harvests into that NPC's personal container.
 */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICNPCFarmPlotActor : public AICNPCWorkRewardActor
{
    GENERATED_BODY()
public:
    AICNPCFarmPlotActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnNPCUseStarted(AICNPCBase* NPC) override;

    /** Stable per-field ID. Falls back to the level actor name. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="IC|NPC Farm") FName PlotID;
    /** A zero-stage field chooses one of these crops on its first NPC interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC Farm") TArray<FName> PlantableCropIDs;
    /** Mature crops reset after harvest regardless of this roll; rewards are granted only on success. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC Farm", meta=(ClampMin="0.0", ClampMax="1.0")) float HarvestRewardChance = 0.6f;
    /** Used only when the selected crop has no player crop definition; leave empty to use primitive/default presentation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC Farm|Fallback") TArray<TSoftObjectPtr<UStaticMesh>> FallbackStageMeshes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|NPC Farm|Fallback") TArray<FICNPCWorkReward> FallbackHarvestRewards;
    /** Stage mesh only; keeps the inherited interaction/collision mesh intact. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|NPC Farm") TObjectPtr<UStaticMeshComponent> CropVisual;

    UFUNCTION(BlueprintPure, Category="IC|NPC Farm") int32 GetNPCGrowthStage() const;
    UFUNCTION(BlueprintPure, Category="IC|NPC Farm") bool IsMature() const;
    UFUNCTION(BlueprintCallable, Category="IC|NPC Farm") void RefreshFarmPresentation();

private:
    UFUNCTION() void HandleClockChanged();
    FName GetPlotKey() const;
    FName GetActiveCropID() const;
    FName ChoosePlantableCropID() const;
    int32 GetStageCount() const;
    bool ProcessMidnightGrowth();
    bool HarvestToNPC(AICNPCBase& NPC);
    int32 LastObservedDay = INDEX_NONE;
};
