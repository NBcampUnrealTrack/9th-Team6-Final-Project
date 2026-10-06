#pragma once
#include "CoreMinimal.h"
#include "InputAction.h"
#include "ICInputTypes.generated.h"

UENUM(BlueprintType)
enum class EICInputCommand : uint8
{
    Jump, Attack, Block, Dodge, NextTarget, Interact, Skill, CompanionSkill, QuickSlot,
    Inventory, Quest, Map, Party, Menu, Save, Load, Confirm,
    Equipment, Stats, Skills, Codex, SprintDodge, TargetToggle, ClearTarget
};

USTRUCT(BlueprintType)
struct FICInputCommandBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> Action;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") EICInputCommand Command = EICInputCommand::Interact;
    // Zero-based slot for Skill, CompanionSkill and QuickSlot; ignored by other commands.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(ClampMin="0", ClampMax="3")) int32 Slot = 0;
};
