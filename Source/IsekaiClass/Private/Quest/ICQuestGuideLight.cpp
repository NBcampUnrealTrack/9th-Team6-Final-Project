#include "Quest/ICQuestGuideLight.h"

#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "Time/ICTimeSubsystem.h"

AICQuestGuideLight::AICQuestGuideLight()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    bReplicates = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("GuideRoot"));
    TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
    TrailEffect->SetupAttachment(RootComponent);
    TrailEffect->SetAutoActivate(false);
    TrailEffect->SetAutoDestroy(false);
    TrailEffect->SetCanEverAffectNavigation(false);
    TrailEffect->AddTickPrerequisiteActor(this);
    SetActorHiddenInGame(true);
}

void AICQuestGuideLight::PlayPath(const TArray<FVector>& Points, float MaxDistance, float Speed, float Height, bool bAutoDestroy)
{
    Stop();
    bDestroyOnFinish = bAutoDestroy;
    if (Points.Num() < 2 || !FMath::IsFinite(MaxDistance) || MaxDistance <= 0
        || !FMath::IsFinite(Speed) || Speed <= 0 || !FMath::IsFinite(Height)) { FinishPlayback(); return; }
    for (const FVector& Point : Points) if (Point.ContainsNaN()) { FinishPlayback(); return; }
    PathPoints.Add(Points[0]);
    PathDistances.Add(0.f);
    float Remaining = MaxDistance;
    for (int32 Index = 1; Index < Points.Num() && Remaining > UE_KINDA_SMALL_NUMBER; ++Index)
    {
        const FVector Segment = Points[Index] - PathPoints.Last();
        const float Length = Segment.Size();
        if (Length <= UE_KINDA_SMALL_NUMBER) continue;
        const float Advance = FMath::Min(Length, Remaining);
        PathPoints.Add(PathPoints.Last() + Segment * (Advance / Length));
        PulseDistance += Advance;
        PathDistances.Add(PulseDistance);
        Remaining -= Advance;
    }
    if (PathPoints.Num() < 2 || PulseDistance <= UE_KINDA_SMALL_NUMBER) { FinishPlayback(); return; }
    TravelSpeed = Speed;
    HeightOffset = Height;
    ConformPathToGround(MaxDistance);
    if (PathPoints.Num() < 2 || PulseDistance <= UE_KINDA_SMALL_NUMBER) { FinishPlayback(); return; }
    DistanceRemaining = PulseDistance;
    SetActorLocation(PathPoints[0] + FVector(0, 0, HeightOffset));
    SetActorRotation((PathPoints[1] - PathPoints[0]).Rotation());
    TrailEffect->SetRelativeTransform(FTransform::Identity);
    const float Lifetime = FMath::IsFinite(TailLength) ? FMath::Max(.05f, TailLength / Speed) : .4f;
    TrailEffect->SetVariableFloat(TEXT("User.TrailLifetime"), Lifetime);
    if (!TrailMaterial && TrailBaseMaterial)
        TrailMaterial = UMaterialInstanceDynamic::Create(TrailBaseMaterial, this);
    if (TrailMaterial) TrailEffect->SetVariableMaterial(TEXT("User.TrailMaterial"), TrailMaterial);
    if (!CoreMaterial && CoreBaseMaterial)
        CoreMaterial = UMaterialInstanceDynamic::Create(CoreBaseMaterial, this);
    if (CoreMaterial) TrailEffect->SetVariableMaterial(TEXT("User.CoreMaterial"), CoreMaterial);
    SetVisualOpacity(FadeInDuration > 0.f ? 0.f : 1.f);
    SetActorHiddenInGame(false);
    TrailEffect->SetVisibility(true, true);
    TrailEffect->Activate(true);
    bPlaying = true;
    SetActorTickEnabled(true);
}

void AICQuestGuideLight::ConformPathToGround(float MaxDistance)
{
    UWorld* World = GetWorld();
    if (!World) return;
    const TArray<FVector> Route = MoveTemp(PathPoints);
    PathPoints.Reset();
    PathDistances.Reset();
    PulseDistance = 0.f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(QuestGuideGround), false, GetOwner());
    Query.AddIgnoredActor(this);
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    // Stay near the selected NavMesh floor; a roof on another storey must not
    // pull the guide upwards. Elevated step bridges keep their existing height.
    auto AboveGround = [&](const FVector& Point)
    {
        FVector Visual = Point + FVector(0, 0, HeightOffset);
        FHitResult Ground;
        if (World->LineTraceSingleByObjectType(Ground,
            Point + FVector(0, 0, 100), Point - FVector(0, 0, 200), Objects, Query)
            && Ground.ImpactNormal.Z >= .45f)
            Visual.Z = FMath::Max(Visual.Z, Ground.ImpactPoint.Z + FMath::Max(20.f, HeightOffset));
        return Visual;
    };
    FVector Previous = AboveGround(Route[0]);
    PathPoints.Add(Previous - FVector(0, 0, HeightOffset));
    PathDistances.Add(0.f);
    int32 Samples = 0;
    for (int32 Index = 1; Index < Route.Num(); ++Index)
    {
        const double Length = FVector::Distance(Route[Index - 1], Route[Index]);
        const int32 Steps = FMath::CeilToInt(FMath::Min(512., Length / 25.));
        for (int32 Step = 1; Step <= Steps; ++Step)
        {
            if (++Samples > 512) return;
            FVector Visual = AboveGround(FMath::Lerp(Route[Index - 1], Route[Index], double(Step) / Steps));
            const double Distance = FVector::Distance(Previous, Visual);
            if (Distance <= UE_KINDA_SMALL_NUMBER) continue;
            const double Advance = FMath::Min(Distance, double(MaxDistance - PulseDistance));
            Visual = FMath::Lerp(Previous, Visual, Advance / Distance);
            FHitResult Hit;
            // Check the entire segment as well as its sampled endpoints. If
            // terrain or a wall still blocks it, stop before drawing through it.
            if (World->SweepSingleByObjectType(Hit, Previous, Visual, FQuat::Identity,
                Objects, FCollisionShape::MakeSphere(12.f), Query)) return;
            PulseDistance += Advance;
            PathPoints.Add(Visual - FVector(0, 0, HeightOffset));
            PathDistances.Add(PulseDistance);
            Previous = Visual;
            if (PulseDistance >= MaxDistance - UE_KINDA_SMALL_NUMBER) return;
        }
    }
}

void AICQuestGuideLight::SetVisualOpacity(float Opacity)
{
    VisualOpacity = FMath::Clamp(Opacity, 0.f, 1.f);
    const float Brightness = FMath::IsFinite(TrailBrightness) ? FMath::Max(0.f, TrailBrightness) : 2.f;
    for (UMaterialInstanceDynamic* Material : {TrailMaterial.Get(), CoreMaterial.Get()})
    {
        if (!Material) continue;
        Material->SetScalarParameterValue(TEXT("GuideOpacity"), VisualOpacity);
        Material->SetScalarParameterValue(TEXT("GuideBrightness"), Brightness);
        Material->SetVectorParameterValue(TEXT("GuideColor"), GuideColor);
    }
}

void AICQuestGuideLight::Stop()
{
    bPlaying = false;
    bFading = false;
    FadeElapsed = 0;
    PulseDistance = 0;
    DistanceRemaining = 0;
    SetActorTickEnabled(false);
    SetActorHiddenInGame(true);
    SetVisualOpacity(0.f);
    TrailEffect->DeactivateImmediate();
    PathPoints.Reset();
    PathDistances.Reset();
}

void AICQuestGuideLight::FadeOut()
{
    if (!bPlaying || bFading) return;
    bPlaying = false;
    bFading = true;
    FadeElapsed = 0;
    FadeStartOpacity = VisualOpacity;
    TrailEffect->Deactivate();
}

FVector AICQuestGuideLight::SampleRoute(float Distance) const
{
    for (int32 Index = 1; Index < PathPoints.Num(); ++Index)
    {
        if (Distance <= PathDistances[Index])
        {
            const float SegmentLength = PathDistances[Index] - PathDistances[Index - 1];
            return FMath::Lerp(PathPoints[Index - 1], PathPoints[Index],
                FMath::Clamp((Distance - PathDistances[Index - 1]) / SegmentLength, 0.f, 1.f));
        }
    }
    return PathPoints.Last();
}

void AICQuestGuideLight::FinishPlayback()
{
    Stop();
    if (bDestroyOnFinish) Destroy();
}

void AICQuestGuideLight::UpdateFade(float DeltaSeconds)
{
    FadeElapsed += DeltaSeconds;
    const float Duration = FMath::IsFinite(FadeOutDuration) ? FMath::Max(.01f, FadeOutDuration) : .6f;
    const float T = FMath::Clamp(FadeElapsed / Duration, 0.f, 1.f);
    const float Opacity = FadeStartOpacity * (1.f - T * T * (3.f - 2.f * T));
    SetVisualOpacity(Opacity);
    if (T >= 1.f) FinishPlayback();
}

void AICQuestGuideLight::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsPlaying() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    const UICTimeSubsystem* Time = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICTimeSubsystem>() : nullptr;
    if (Time && !Time->IsRunning()) { FinishPlayback(); return; }
    if (bFading) { UpdateFade(DeltaSeconds); return; }
    const float TimeToEnd = DistanceRemaining / TravelSpeed;
    DistanceRemaining = FMath::Max(0.f, DistanceRemaining - TravelSpeed * DeltaSeconds);
    SetActorLocation(SampleRoute(PulseDistance - DistanceRemaining) + FVector(0, 0, HeightOffset));
    const float Traveled = PulseDistance - DistanceRemaining;
    const float FadeIn = FMath::IsFinite(FadeInDuration) ? FMath::Max(0.f, FadeInDuration) : .12f;
    const float Appearance = FadeIn > 0.f ? FMath::Clamp(Traveled / TravelSpeed / FadeIn, 0.f, 1.f) : 1.f;
    SetVisualOpacity(Appearance * Appearance * (3.f - 2.f * Appearance));
    FVector Direction = SampleRoute(FMath::Min(PulseDistance, Traveled + 1.f)) - SampleRoute(Traveled);
    if (Direction.IsNearlyZero()) Direction = SampleRoute(Traveled) - SampleRoute(FMath::Max(0.f, Traveled - 1.f));
    if (!Direction.IsNearlyZero()) SetActorRotation(Direction.Rotation());
    if (DistanceRemaining <= UE_KINDA_SMALL_NUMBER)
    {
        FadeOut();
        if (DeltaSeconds > TimeToEnd) UpdateFade(DeltaSeconds - TimeToEnd);
    }
}
