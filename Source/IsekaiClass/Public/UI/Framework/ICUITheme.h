#pragma once
#include "CoreMinimal.h"
class UWidget;
class UWidgetTree;
class UButton;
struct FSlateBrush;
namespace ICUITheme
{
    ISEKAICLASS_API void Apply(UWidgetTree* Tree);
    ISEKAICLASS_API void SetSelected(UButton* Button,bool Selected);
    ISEKAICLASS_API FSlateBrush PanelBrush();
    void UpdateFocus(UWidget* Root);
}
