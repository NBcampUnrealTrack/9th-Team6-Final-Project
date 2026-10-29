#include "Life/World/ICFishingLine.h"
#include "Combat/Actors/ICWeaponActor.h"
#include "Life/World/ICLifeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AICFishingLine::AICFishingLine()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    for(int32 Index=0;Index<20;++Index)
    {
        auto* Segment=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Line%02d"),Index));
        Segment->SetupAttachment(RootComponent);Segment->SetStaticMesh(Cylinder);
        Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);Segment->SetCastShadow(false);
        Segments.Add(Segment);
    }
    FloatMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Float"));FloatMesh->SetupAttachment(RootComponent);
    FloatMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    FloatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);FloatMesh->SetCastShadow(false);
}
void AICFishingLine::InitializeLine(AICWeaponActor* Rod,AICLifeActor* Zone,const FVector& Landing)
{SourceRod=Rod;FishingZone=Zone;LandingPoint=Landing;Tick(0);}
FVector AICFishingLine::GetRodTip() const
{return SourceRod.IsValid()?SourceRod->GetMuzzleLocation():GetActorLocation();}
bool AICFishingLine::HasValidLanding() const
{
    if(!SourceRod.IsValid()||!FishingZone.IsValid()||!FishingZone->ContainsFishingLanding(LandingPoint))return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FishingTether),false);
    Query.AddIgnoredActor(this);Query.AddIgnoredActor(SourceRod.Get());Query.AddIgnoredActor(GetOwner());
    for(TActorIterator<AICLifeActor> It(GetWorld());It;++It)if(It->LifeType==EICDefinitionType::FishingPoint)Query.AddIgnoredActor(*It);
    FHitResult Hit;
    if(!GetWorld()->LineTraceSingleByChannel(Hit,GetRodTip(),LandingPoint+FVector(0,0,3),ECC_Visibility,Query))return true;
    return Hit.ImpactNormal.Z>=.5f&&FishingZone->ContainsFishingLanding(Hit.ImpactPoint)
        &&FVector::DistSquared(Hit.ImpactPoint,LandingPoint)<=FMath::Square(15.f);
}
void AICFishingLine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FVector Start=GetRodTip();
    // Sag falls away from both ends, keeping the tip and float attached exactly.
    auto Position=[&](float Alpha){return FMath::Lerp(Start,LandingPoint,Alpha)-FVector(0,0,4*SagCm*Alpha*(1-Alpha));};
    for(int32 Index=0;Index<Segments.Num();++Index)
    {
        const FVector A=Position(float(Index)/Segments.Num()),B=Position(float(Index+1)/Segments.Num());
        Segments[Index]->SetWorldLocationAndRotation((A+B)*.5f,FRotationMatrix::MakeFromZ(B-A).Rotator());
        Segments[Index]->SetWorldScale3D(FVector(LineRadiusCm/50,LineRadiusCm/50,(B-A).Size()/100));
    }
    FloatMesh->SetWorldLocation(LandingPoint+FVector(0,0,FloatDiameterCm*.3f));
    FloatMesh->SetWorldScale3D(FVector(FloatDiameterCm/100));
}
