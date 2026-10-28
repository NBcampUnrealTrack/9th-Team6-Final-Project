#pragma once
#include "CoreMinimal.h"
class UWidgetTree;
class UCanvasPanel;
namespace ICFirefightHUDLayout
{
    bool Status(UWidgetTree* Tree, bool Player, bool Alternate = false);
    bool QuickSlot(UWidgetTree* Tree);
    bool QuickSlots(UWidgetTree* Tree);
    void Target(UWidgetTree* Tree, UCanvasPanel* HUD);
}
