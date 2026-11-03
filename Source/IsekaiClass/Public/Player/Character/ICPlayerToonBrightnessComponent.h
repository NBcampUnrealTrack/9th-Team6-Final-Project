#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ICPlayerToonBrightnessComponent.generated.h"

class USkeletalMesh;
class UMaterialInstanceDynamic;
class UDirectionalLightComponent;
class USkyLightComponent;

/** Preserve the authored Unlit toon face while estimating brightness from world lights. */
UCLASS(ClassGroup=(IC),meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICPlayerToonBrightnessComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICPlayerToonBrightnessComponent();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    UFUNCTION(BlueprintCallable,Category="IC|Appearance") void RefreshMaterials();
    UFUNCTION(BlueprintPure,Category="IC|Appearance") float GetBrightness() const { return CurrentBrightness; }
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance",meta=(ClampMin="0",ClampMax="1")) float MinimumBrightness=.18f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance",meta=(ClampMin="0",ClampMax="1")) float MaximumBrightness=.82f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance",meta=(ClampMin="0")) float SkyContribution=.34f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance",meta=(ClampMin="0")) float SunContribution=.30f;
    /** Sum of the existing world's three daytime directional lights, in lux. */
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance",meta=(ClampMin="0.01",Units="lux")) float ReferenceSunIntensity=13.7f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance",meta=(ClampMin="0.1")) float TransitionSpeed=3.f;
private:
    TWeakObjectPtr<USkeletalMesh> CachedMesh;
    TArray<TWeakObjectPtr<UDirectionalLightComponent>> SunLights;
    TWeakObjectPtr<USkyLightComponent> SkyLight;
    float LightCacheRemaining=0.f;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    float CurrentBrightness=1.f;
    void RefreshLights();
    float DesiredBrightness();
};
