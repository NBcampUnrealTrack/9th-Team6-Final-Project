#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICDayNightController.generated.h"

class ADirectionalLight;
class ASkyLight;
class UICTimeSubsystem;
class UDirectionalLightComponent;
class USkyLightComponent;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDayNightState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) double MinuteOfDay=0;
    UPROPERTY(BlueprintReadOnly) float SunElevationDegrees=0;
    UPROPERTY(BlueprintReadOnly) FRotator SunRotation=FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadOnly) float SunIntensityScale=0;
    UPROPERTY(BlueprintReadOnly) float SkyIntensityScale=0;
    UPROPERTY(BlueprintReadOnly) FLinearColor SunTint=FLinearColor::White;
};

/** Place a Blueprint instance in a level and assign its existing sky actors. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICDayNightController : public AActor
{
    GENERATED_BODY()
public:
    AICDayNightController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditInstanceOnly,BlueprintReadOnly,Category="IC|Day Night|Actors") TArray<TObjectPtr<ADirectionalLight>> SunLights;
    UPROPERTY(EditInstanceOnly,BlueprintReadOnly,Category="IC|Day Night|Actors") TArray<TObjectPtr<ASkyLight>> SkyLights;
    UPROPERTY(EditInstanceOnly,BlueprintReadOnly,Category="IC|Day Night|Actors") TArray<TObjectPtr<AActor>> SkySpheres;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Day Night|Sun",meta=(ClampMin="0",ClampMax="23.9")) float SunriseHour=6.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Day Night|Sun",meta=(ClampMin="0.1",ClampMax="24")) float SunsetHour=18.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Day Night|Sun",meta=(ClampMin="1",ClampMax="89")) float NoonElevationDegrees=60.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Day Night|Sun",meta=(Units="deg")) float SunriseAzimuthDegrees=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Day Night|Sun") FLinearColor HorizonSunTint=FLinearColor(1.f,.45f,.22f);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Day Night|Sky",meta=(ClampMin="0",ClampMax="1")) float NightSkyIntensityScale=.08f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Day Night|Sky") bool bRealTimeSkyCapture=true;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="IC|Day Night") FICDayNightState CurrentState;

    UFUNCTION(BlueprintPure,Category="IC|Day Night") FICDayNightState EvaluateTime(double GameMinute) const;
    UFUNCTION(BlueprintCallable,Category="IC|Day Night") void RefreshFromClock();

private:
    struct FSunBinding
    {
        TWeakObjectPtr<UDirectionalLightComponent> Light;
        float Intensity=0;
        FLinearColor Color=FLinearColor::White;
    };
    struct FSkyBinding
    {
        TWeakObjectPtr<USkyLightComponent> Light;
        float Intensity=0;
    };
    TArray<FSunBinding> Suns;
    TArray<FSkyBinding> Skies;
    TWeakObjectPtr<UICTimeSubsystem> Time;
    double LastAppliedMinute=-1;
    void BindLights();
};
