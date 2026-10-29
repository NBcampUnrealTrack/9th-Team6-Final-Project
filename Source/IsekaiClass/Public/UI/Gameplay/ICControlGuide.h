#pragma once
#include "CoreMinimal.h"
class AICPlayerController;
class UWidgetTree;
class UCanvasPanel;

struct FICControlGuidePage
{
    FText Title;
    FText Body;
    FName HUDTarget;
};

/** Shared guide content, reusable by a future settings/help tab. */
namespace ICControlGuide
{
    ISEKAICLASS_API TArray<FICControlGuidePage> Pages(const AICPlayerController* PC);
    void Build(UWidgetTree* Tree,UCanvasPanel* Canvas);
}
