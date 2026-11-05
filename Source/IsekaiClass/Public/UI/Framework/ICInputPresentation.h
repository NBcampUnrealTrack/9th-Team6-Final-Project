#pragma once

#include "CoreMinimal.h"
#include "Input/ICInputTypes.h"

class AICPlayerController;
class ULocalPlayer;
class UTextBlock;
class UUserWidget;

/** Shared device-aware prompts. Input hints never control pointer availability. */
namespace ICInputPresentation
{
    bool UsesGamepad(const ULocalPlayer* Player);
    FText KeyLabel(FKey Key);
    FText CommandLabel(const AICPlayerController* Controller, EICInputCommand Command, int32 Slot = 0);
    FText ForDevice(const AICPlayerController* Controller, const FText& Keyboard, const FText& Gamepad);
    /** Gamepad templates may use {Accept}, {Back}, {FaceLeft} and {FaceTop}. */
    void SetHint(UTextBlock* Label, AICPlayerController* Controller, const FText& Keyboard, const FText& Gamepad);
    void RefreshHints(ULocalPlayer* Player);
    void BindTreeHints(UUserWidget* Widget);
}
