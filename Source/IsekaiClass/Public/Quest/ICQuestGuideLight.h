#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICQuestGuideLight.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;

/** One Niagara wisp: attached core and world-space history, moving along a navigation route. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICQuestGuideLight : public AActor
{
    GENERATED_BODY()

public:
    AICQuestGuideLight();
    virtual void Tick(float DeltaSeconds) override;

    // Production pulses destroy themselves after travel/fade. Tests may opt into reuse.
    void PlayPath(const TArray<FVector>& Points, float MaxDistance, float Speed, float Height, bool bAutoDestroy = true);
    void Stop();
    void FadeOut();
    bool IsPlaying() const { return bPlaying || bFading; }
    bool IsFadingOut() const { return bFading; }


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="1", Units="cm"))
    float TailLength = 900.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0.01", Units="s"))
    float FadeOutDuration = .6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0"))
    float TrailBrightness = 1.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide")
    FLinearColor GuideColor = FLinearColor(1.f, .52f, .025f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0", Units="s"))
    float FadeInDuration = .12f;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|Quest Guide")
    TObjectPtr<UNiagaraComponent> TrailEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Quest Guide")
    TObjectPtr<UMaterialInterface> TrailBaseMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="IC|Quest Guide")
    TObjectPtr<UMaterialInterface> CoreBaseMaterial;

private:
    void FinishPlayback();
    void UpdateFade(float DeltaSeconds);
    void SetVisualOpacity(float Opacity);
    FVector SampleRoute(float Distance) const;
    void ConformPathToGround(float MaxDistance);

    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> TrailMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;
    TArray<FVector> PathPoints;
    TArray<float> PathDistances;
    float DistanceRemaining = 0;
    float PulseDistance = 0;
    float TravelSpeed = 600;
    float HeightOffset = 50;
    float FadeElapsed = 0;
    float VisualOpacity = 0;
    float FadeStartOpacity = 1;
    bool bDestroyOnFinish = true;
    bool bPlaying = false;
    bool bFading = false;
};
