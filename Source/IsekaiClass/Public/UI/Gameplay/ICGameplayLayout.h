#pragma once
#include "CoreMinimal.h"
class UWidgetTree;

namespace ICGameplayLayout
{
    // Shared by the editor asset authoring tool and native widget fallbacks.
    bool Build(UWidgetTree* Tree, FName Kind);
    bool EnsureGameClock(UWidgetTree* Tree);
}
