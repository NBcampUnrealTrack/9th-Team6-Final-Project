#pragma once
#include "CoreMinimal.h"

class USkeletalMeshComponent;
struct FICMonsterRow;

namespace ICMonsterPresentation
{
    // The existing ten normal monsters have a median authored height of 160.49 cm.
    // Use one stable reference for field monsters and both companion categories.
    inline constexpr double NormalFieldHeightCm=160.;
    inline constexpr double NormalPetRatio=.35;
    inline constexpr double RegionalBossPetRatio=.5;
    FBoxSphereBounds VisualBounds(const USkeletalMeshComponent* Mesh);
    double VisualHeight(const USkeletalMeshComponent* Mesh);
    double PetHeight(const FICMonsterRow& Definition);
    bool FitHeight(USkeletalMeshComponent* Mesh,double HeightCm);
    void SetFeetHeight(USkeletalMeshComponent* Mesh,double WorldZ);
}
