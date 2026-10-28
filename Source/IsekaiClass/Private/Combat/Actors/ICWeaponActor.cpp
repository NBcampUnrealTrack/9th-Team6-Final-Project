#include "Combat/Actors/ICWeaponActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Data/Rows/Items/ICItemRows.h"

AICWeaponActor::AICWeaponActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Grip = CreateDefaultSubobject<USceneComponent>(TEXT("Grip")); RootComponent = Grip;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual")); Visual->SetupAttachment(Grip);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle")); Muzzle->SetupAttachment(Grip);
}
void AICWeaponActor::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); RefreshGeometry(); }
void AICWeaponActor::Configure(const FICEquipmentRow& Definition)
{
    if (auto* Mesh = Definition.WeaponMesh.LoadSynchronous()) Visual->SetStaticMesh(Mesh);
    if(Definition.bOverrideWeaponGeometry){Visual->SetRelativeTransform(Definition.WeaponVisualTransform);LengthAxis=Definition.WeaponLengthAxis;}
    RefreshGeometry();
}
void AICWeaponActor::GetBladeTrace(FVector& Start, FVector& End) const
{
    const UStaticMesh* Mesh = Visual->GetStaticMesh();
    if (!Mesh) { Start = End = Visual->GetComponentLocation(); return; }
    const FBox Box = Mesh->GetBoundingBox();
    const FVector Axis = LengthAxis.GetSafeNormal(UE_SMALL_NUMBER,FVector::UpVector);
    const float HalfLength = FVector::DotProduct(Axis.GetAbs(), Box.GetExtent());
    const FVector Base = Box.GetCenter() - Axis * HalfLength;
    Start = Visual->GetComponentTransform().TransformPosition(Base + Axis * (2.f * HalfLength * HandleFraction));
    End = Visual->GetComponentTransform().TransformPosition(Box.GetCenter() + Axis * HalfLength);
}
float AICWeaponActor::GetScaledLengthCm() const
{
    const UStaticMesh* Mesh = Visual->GetStaticMesh(); if (!Mesh) return 0;
    const FVector Axis = LengthAxis.GetSafeNormal(UE_SMALL_NUMBER,FVector::UpVector);
    return 2.f * FVector::DotProduct(Axis.GetAbs(), Mesh->GetBoundingBox().GetExtent())
        * Visual->GetComponentTransform().TransformVector(Axis).Length();
}
float AICWeaponActor::GetMeleeReachCm() const { return GetScaledLengthCm() + MeleeReachAllowanceCm; }
FVector AICWeaponActor::GetMuzzleLocation() const
{
    if(bAutoMuzzleAtTip && Visual->GetStaticMesh()){FVector Start,End;GetBladeTrace(Start,End);return End;}
    return Muzzle->GetComponentLocation();
}
void AICWeaponActor::RefreshGeometry()
{
    if (bAutoMuzzleAtTip && Visual->GetStaticMesh())
    {
        FVector Start, End; GetBladeTrace(Start, End);
        Muzzle->SetWorldLocation(End);
    }
}
