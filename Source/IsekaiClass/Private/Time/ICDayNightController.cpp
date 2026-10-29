#include "Time/ICDayNightController.h"

#include "Time/ICTimeSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "UObject/UnrealType.h"

AICDayNightController::AICDayNightController()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("DayNightRoot"));
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickInterval=.1f;
#if WITH_EDITORONLY_DATA
    bIsSpatiallyLoaded=false;
#endif
}

void AICDayNightController::BeginPlay()
{
    Super::BeginPlay();
    BindLights();
    Time=GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;
    if(Time.IsValid())Time->OnClockChanged.AddUniqueDynamic(this,&ThisClass::RefreshFromClock);
    RefreshFromClock();
}

void AICDayNightController::EndPlay(const EEndPlayReason::Type Reason)
{
    if(Time.IsValid())Time->OnClockChanged.RemoveDynamic(this,&ThisClass::RefreshFromClock);
    Time.Reset();Suns.Reset();Skies.Reset();
    Super::EndPlay(Reason);
}

void AICDayNightController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshFromClock();
}

void AICDayNightController::BindLights()
{
    Suns.Reset();Skies.Reset();
    for(const auto& Actor:SunLights)if(IsValid(Actor))
        if(auto* Light=Cast<UDirectionalLightComponent>(Actor->GetLightComponent()))
        {
            Light->SetMobility(EComponentMobility::Movable);
            const bool bPrimarySun=Suns.IsEmpty();
            Light->SetForwardShadingPriority(bPrimarySun?1:0);
            Light->SetAtmosphereSunLight(bPrimarySun);
            if(bPrimarySun)Light->SetAtmosphereSunLightIndex(0);
            Suns.Add({Light,Light->Intensity,Light->GetLightColor()});
        }
    for(const auto& Actor:SkyLights)if(IsValid(Actor))
        if(auto* Light=Actor->GetLightComponent())
        {
            Light->SetMobility(EComponentMobility::Movable);
            if(bRealTimeSkyCapture&&Light->SourceType==SLS_CapturedScene)Light->SetRealTimeCaptureEnabled(true);
            Skies.Add({Light,Light->Intensity});
        }
    // The existing engine sky Blueprint stores a directional-light reference
    // under a display name containing spaces. Bind by property type instead.
    for(const auto& Sphere:SkySpheres)if(IsValid(Sphere)&&!SunLights.IsEmpty()&&IsValid(SunLights[0]))
        for(TFieldIterator<FObjectPropertyBase> It(Sphere->GetClass());It;++It)
            if(It->PropertyClass&&It->PropertyClass->IsChildOf(ADirectionalLight::StaticClass()))
                It->SetObjectPropertyValue_InContainer(Sphere,SunLights[0]);
}

FICDayNightState AICDayNightController::EvaluateTime(double GameMinute) const
{
    FICDayNightState State;
    State.MinuteOfDay=FMath::IsFinite(GameMinute)?FMath::Fmod(FMath::Fmod(GameMinute,1440.)+1440.,1440.):0.;
    const double Rise=FMath::Clamp(double(SunriseHour)*60.,0.,1438.);
    const double Set=FMath::Clamp(double(SunsetHour)*60.,Rise+1.,1440.);
    const double DayLength=Set-Rise;
    const double Minute=State.MinuteOfDay;
    const bool bDay=Minute>=Rise&&Minute<Set;
    const double NightElapsed=Minute>=Set?Minute-Set:Minute+1440.-Set;
    const double Angle=bDay?PI*(Minute-Rise)/DayLength:PI+PI*NightElapsed/(1440.-DayLength);
    const double Tilt=FMath::DegreesToRadians(FMath::Clamp(double(NoonElevationDegrees),1.,89.));
    // This tilted orbit rises on one horizon and sets on the opposite horizon.
    // A pitch-only sine would incorrectly rise and set in the same direction.
    const FVector Direction=FVector(FMath::Cos(Angle),FMath::Sin(Angle)*FMath::Cos(Tilt),FMath::Sin(Angle)*FMath::Sin(Tilt)).RotateAngleAxis(SunriseAzimuthDegrees,FVector::UpVector);
    State.SunRotation=(-Direction).Rotation();
    State.SunElevationDegrees=FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Direction.Z,-1.,1.)));
    State.SunIntensityScale=FMath::SmoothStep(-2.f,12.f,State.SunElevationDegrees);
    const float SkyAlpha=FMath::SmoothStep(-8.f,12.f,State.SunElevationDegrees);
    State.SkyIntensityScale=FMath::Lerp(FMath::Clamp(NightSkyIntensityScale,0.f,1.f),1.f,SkyAlpha);
    State.SunTint=FMath::Lerp(HorizonSunTint,FLinearColor::White,FMath::SmoothStep(0.f,20.f,State.SunElevationDegrees));
    return State;
}

void AICDayNightController::RefreshFromClock()
{
    if(!Time.IsValid())return;
    const double Minute=Time->GetClock().GameMinute;
    if(FMath::IsNearlyEqual(Minute,LastAppliedMinute,1.e-6))return;
    LastAppliedMinute=Minute;CurrentState=EvaluateTime(Minute);
    for(const auto& Binding:Suns)if(auto* Light=Binding.Light.Get())
    {
        Light->SetWorldRotation(CurrentState.SunRotation);
        Light->SetIntensity(Binding.Intensity*CurrentState.SunIntensityScale);
        Light->SetLightColor(Binding.Color*CurrentState.SunTint,false);
    }
    for(const auto& Binding:Skies)if(auto* Light=Binding.Light.Get())
        Light->SetIntensity(Binding.Intensity*CurrentState.SkyIntensityScale);
    for(const auto& Sphere:SkySpheres)if(IsValid(Sphere))
        if(auto* Update=Sphere->FindFunction(TEXT("UpdateSunDirection"));Update&&Update->NumParms==0)
            Sphere->ProcessEvent(Update,nullptr);
}
