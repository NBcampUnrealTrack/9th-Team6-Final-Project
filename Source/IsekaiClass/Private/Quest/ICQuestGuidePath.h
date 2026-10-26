#pragma once

#include "CoreMinimal.h"
#include "AI/Navigation/NavigationTypes.h"

class ARecastNavMesh;
class UWorld;

namespace ICQuestGuidePath
{
    // Route points use ground coordinates, including optional step bridges. The caller adds VisualHeight.
    bool BuildStepBridge(UWorld& World, const FVector& From, const FVector& To,
        float VisualHeight, const AActor* Owner, TArray<FVector>& Points);
    bool FindAcrossSteps(UWorld& World, const ARecastNavMesh& Mesh,
        const FNavLocation& Start, const FNavLocation& Goal, float MaxHorizontalGap,
        float VisualHeight, const AActor* Owner, TArray<FVector>& Points);
}
