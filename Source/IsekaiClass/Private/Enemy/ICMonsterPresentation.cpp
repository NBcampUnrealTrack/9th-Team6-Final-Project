#include "Enemy/ICMonsterPresentation.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

FBoxSphereBounds ICMonsterPresentation::VisualBounds(const USkeletalMeshComponent* Mesh)
{
    const auto* Asset=Mesh?Mesh->GetSkeletalMeshAsset():nullptr;
    // Imported mesh bounds are stable across idle/flight frames; live animation
    // bounds would cause differently timed animations to pick different scales.
    return Asset?Asset->GetBounds().TransformBy(Mesh->GetComponentTransform()):FBoxSphereBounds(ForceInit);
}

double ICMonsterPresentation::VisualHeight(const USkeletalMeshComponent* Mesh)
{
    return VisualBounds(Mesh).BoxExtent.Z*2.;
}

double ICMonsterPresentation::PetHeight(const FICMonsterRow& Definition)
{
    return NormalFieldHeightCm*(Definition.Kind==EICMonsterKind::RegionalBoss?RegionalBossPetRatio:NormalPetRatio);
}

void ICMonsterPresentation::SetFeetHeight(USkeletalMeshComponent* Mesh,double WorldZ)
{
    if(!Mesh||!Mesh->GetSkeletalMeshAsset()||!FMath::IsFinite(WorldZ))return;
    const auto Bounds=VisualBounds(Mesh);
    Mesh->AddWorldOffset(FVector(0,0,WorldZ-(Bounds.Origin.Z-Bounds.BoxExtent.Z)));
}

bool ICMonsterPresentation::FitHeight(USkeletalMeshComponent* Mesh,double HeightCm)
{
    const auto Bounds=VisualBounds(Mesh);
    const double Height=Bounds.BoxExtent.Z*2.;
    if(!Mesh||!FMath::IsFinite(Height)||Height<.01||!FMath::IsFinite(HeightCm)||HeightCm<=0)return false;
    const double Feet=Bounds.Origin.Z-Bounds.BoxExtent.Z;
    Mesh->SetRelativeScale3D(Mesh->GetRelativeScale3D()*(HeightCm/Height));
    SetFeetHeight(Mesh,Feet);
    return true;
}
