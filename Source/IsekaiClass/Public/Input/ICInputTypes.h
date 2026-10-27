#pragma once
#include "CoreMinimal.h"
#include "InputAction.h"
#include "ICInputTypes.generated.h"

UENUM(BlueprintType)
enum class EICInputCommand : uint8
{
    Jump = 0,
    Attack = 1,
    Block = 2 UMETA(Hidden),
    Dodge = 3,
    NextTarget = 4,
    Interact = 5,
    Skill = 6,
    CompanionSkill = 7,
    QuickSlot = 8,
    Inventory = 9,
    Quest = 10,
    Map = 11,
    Party = 12,
    Menu = 13,
    Save = 14,
    Load = 15,
    Confirm = 16,
    Equipment = 17,
    Stats = 18,
    Skills = 19,
    Codex = 20,
    SprintDodge = 21 UMETA(Hidden),
    TargetToggle = 22 UMETA(Hidden),
    ClearTarget = 23,
    SwitchWeapon = 24,
    Settings = 25,
    Fishing = 26,
    Roll = 27,
    QuestFollow = 28
};

USTRUCT(BlueprintType)
struct FICInputCommandBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> Action;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") EICInputCommand Command = EICInputCommand::Interact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(ClampMin="0", ClampMax="3")) int32 Slot = 0;
};
