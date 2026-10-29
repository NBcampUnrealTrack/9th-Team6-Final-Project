#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Time/ICDayNightController.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include <limits>

namespace
{
struct FDayNightWorld
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);
    UWorld* World=nullptr;
    FDayNightWorld(){GI->InitializeStandalone();World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);}
    ~FDayNightWorld(){GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSunOrbitCheck,"IC.Time.DayNight.Orbit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSunOrbitCheck::RunTest(const FString&)
{
    FDayNightWorld Session;auto* Sky=Session.World->SpawnActor<AICDayNightController>();
    const auto Rise=Sky->EvaluateTime(6*60),Noon=Sky->EvaluateTime(12*60),Set=Sky->EvaluateTime(18*60),Night=Sky->EvaluateTime(0);
    TestTrue(TEXT("Sunrise and sunset meet the horizon"),FMath::Abs(Rise.SunElevationDegrees)<.001f&&FMath::Abs(Set.SunElevationDegrees)<.001f);
    TestTrue(TEXT("Sun rises and sets on opposite horizons"),FVector::DotProduct(Rise.SunRotation.Vector(),Set.SunRotation.Vector())<-.999);
    TestTrue(TEXT("Noon reaches the editable maximum elevation"),FMath::IsNearlyEqual(Noon.SunElevationDegrees,60.f,.001f));
    TestTrue(TEXT("Noon sunlight travels downward"),Noon.SunRotation.Vector().Z<-.8);
    TestEqual(TEXT("Noon retains authored light intensity"),Noon.SunIntensityScale,1.f);
    TestTrue(TEXT("Midnight sun is below the ground"),Night.SunElevationDegrees< -59.f&&Night.SunRotation.Vector().Z> .8);
    TestEqual(TEXT("Midnight has no direct sunlight"),Night.SunIntensityScale,0.f);
    TestEqual(TEXT("Night retains the configured ambient light"),Night.SkyIntensityScale,Sky->NightSkyIntensityScale);
    TestTrue(TEXT("Sunrise has a warmer tint than noon"),Rise.SunTint.G<Noon.SunTint.G&&Rise.SunTint.B<Noon.SunTint.B);
    TestTrue(TEXT("The orbit repeats on following days"),Noon.SunRotation.Equals(Sky->EvaluateTime(3*1440+12*60).SunRotation,.001));
    TestTrue(TEXT("The orbit is continuous through midnight"),Sky->EvaluateTime(1439.999).SunRotation.Vector().Equals(Sky->EvaluateTime(.001).SunRotation.Vector(),.0001));
    TestTrue(TEXT("Invalid time cannot produce an invalid rotation"),!Sky->EvaluateTime(std::numeric_limits<double>::infinity()).SunRotation.ContainsNaN());
    Sky->SunriseHour=5;Sky->SunsetHour=20;Sky->NoonElevationDegrees=45;
    TestTrue(TEXT("Custom sunrise remains on the horizon"),FMath::Abs(Sky->EvaluateTime(5*60).SunElevationDegrees)<.001);
    TestTrue(TEXT("Custom daylight duration places noon halfway between rise and set"),FMath::IsNearlyEqual(Sky->EvaluateTime(12.5*60).SunElevationDegrees,45.f,.001f));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSunClockBindingsCheck,"IC.Time.DayNight.ClockBindings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSunClockBindingsCheck::RunTest(const FString&)
{
    FDayNightWorld Session;auto* GI=Session.GI;auto* World=Session.World;
    auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    auto* Sun=World->SpawnActor<ADirectionalLight>();auto* OtherSun=World->SpawnActor<ADirectionalLight>();auto* Ambient=World->SpawnActor<ASkyLight>();
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sun->GetLightComponent()->SetIntensity(6);
    OtherSun->GetLightComponent()->SetMobility(EComponentMobility::Movable);OtherSun->GetLightComponent()->SetIntensity(3);
    Ambient->GetLightComponent()->SetMobility(EComponentMobility::Movable);Ambient->GetLightComponent()->SetIntensity(1);
    auto* Cycle=World->SpawnActor<AICDayNightController>();Cycle->SunLights={Sun,OtherSun};Cycle->SkyLights={Ambient};Cycle->bRealTimeSkyCapture=false;
    FICClockState Saved;Saved.GameMinute=8*60;Time->RestoreClock(Saved);Time->SetRunning(true);Time->SetMenuPaused(false);
    Cycle->DispatchBeginPlay();
    TestTrue(TEXT("Level entry uses the current saved clock"),Sun->GetActorRotation().Equals(Cycle->EvaluateTime(Saved.GameMinute).SunRotation,.001));
    TestTrue(TEXT("Every existing sun follows the same clock"),OtherSun->GetActorRotation().Equals(Sun->GetActorRotation(),.001));
    TestTrue(TEXT("Clock change callback is registered"),Time->OnClockChanged.Contains(Cycle,TEXT("RefreshFromClock")));
    const FRotator Before=Sun->GetActorRotation();Time->AdvanceClock(.5,false);Cycle->Tick(.1f);
    TestFalse(TEXT("Fractional game minutes move the sun smoothly"),Sun->GetActorRotation().Equals(Before,.0001));
    Time->SetRunning(false);const FRotator Paused=Sun->GetActorRotation();Time->AdvanceClock(60,false);Cycle->Tick(.1f);
    TestTrue(TEXT("Paused game time freezes the sun"),Sun->GetActorRotation().Equals(Paused,.0001));Time->SetRunning(true);
    Time->JumpGameMinutes(23*60-Time->GetClock().GameMinute);
    TestEqual(TEXT("Sleeping into night immediately removes all direct sunlight"),Sun->GetLightComponent()->Intensity+OtherSun->GetLightComponent()->Intensity,0.f);
    TestEqual(TEXT("Sleeping into night adjusts the existing skylight"),Ambient->GetLightComponent()->Intensity,Cycle->NightSkyIntensityScale);
    Saved.GameMinute=12*60;Time->RestoreClock(Saved);
    TestEqual(TEXT("Loading a daytime save restores the original sun strength"),Sun->GetLightComponent()->Intensity,6.f);
    TestEqual(TEXT("A second sun retains its own authored strength"),OtherSun->GetLightComponent()->Intensity,3.f);
    TestEqual(TEXT("Loading a daytime save restores the skylight"),Ambient->GetLightComponent()->Intensity,1.f);
    TestTrue(TEXT("Loading refreshes the angle immediately"),Sun->GetActorRotation().Equals(Cycle->EvaluateTime(Saved.GameMinute).SunRotation,.001));
    Cycle->Destroy();TestFalse(TEXT("Leaving the level releases the clock callback"),Time->OnClockChanged.Contains(Cycle,TEXT("RefreshFromClock")));
    return !HasAnyErrors();
}
#endif
