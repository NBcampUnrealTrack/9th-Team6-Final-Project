#pragma once

#include "CoreMinimal.h"
#include "ICInventoryTypes.h"
#include "ICWorldStateTypes.generated.h"

UENUM(BlueprintType)
enum class EICMainStat : uint8 { STR, DEX, INT, CON, CHA, LUK };

UENUM(BlueprintType)
enum class EICCharacterKind : uint8 { Player, Recruitable, Resident, Background };

UENUM(BlueprintType)
enum class EICCharacterMode : uint8 { Scheduled, Companion, Downed };

USTRUCT(BlueprintType)
struct FICMainStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 STR = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 DEX = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 INT = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 CON = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 CHA = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 LUK = 1;
    bool IsValid() const;
    bool Set(EICMainStat Stat, int32 Value);
};

USTRUCT(BlueprintType)
struct FICDebuffState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double RemainingSeconds = 0;
};

USTRUCT(BlueprintType)
struct FICCharacterState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName CharacterId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICCharacterKind Kind = EICCharacterKind::Recruitable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICCharacterMode Mode = EICCharacterMode::Scheduled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 PartySlot = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Affinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FICMainStats MainStats;
    // Prototype defaults. GAS owns these values when a live participant is registered.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float HP = 60;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float SP = 70;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxHP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxMP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxSP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double LastSimGameMinute = 360;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FICDebuffState> Debuffs;
    bool HasStats() const { return Kind == EICCharacterKind::Player || Kind == EICCharacterKind::Recruitable; }
};

USTRUCT(BlueprintType)
struct FICInventoryState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) FPlayerContainer Bag;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FPlayerItemInstance> QuestItems;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FName> QuickSlots;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FPlayerContainer> Storage;
    FICInventoryState()
    {
        Bag.Slots.SetNum(20);
        Equipment.SetNum(4);
        QuickSlots.SetNum(4);
        Storage.Add(TEXT("PlayerHome")).Slots.SetNum(40);
    }
};

USTRUCT(BlueprintType)
struct FICQuestState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bAccepted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bCompleted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, int32> Objectives;
};

USTRUCT(BlueprintType)
struct FICDropState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FGuid ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FPlayerItemInstance> Contents;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double CreatedPlaySecond = 0;
};

USTRUCT(BlueprintType)
struct FICClockState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) double GameMinute = 360;
    UPROPERTY(BlueprintReadOnly, SaveGame) double PlaySecond = 0;
    bool IsValid() const { return FMath::IsFinite(GameMinute) && GameMinute >= 0 && FMath::IsFinite(PlaySecond) && PlaySecond >= 0; }
};

USTRUCT(BlueprintType)
struct FICWorldState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bPlayerCreated = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) FString PlayerName;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Preset = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Job = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) FName FirstSkill;
    UPROPERTY(BlueprintReadOnly, SaveGame) FVector PlayerPosition = FVector(0, 0, 120);
    UPROPERTY(BlueprintReadOnly, SaveGame) FRotator PlayerRotation = FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICCharacterState> Characters;
    UPROPERTY(BlueprintReadOnly, SaveGame) FICInventoryState Inventory;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICQuestState> Quests;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> UnlockedWarps;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, double> MonsterKilledAt;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> DefeatedBosses;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, double> ResourceReadyAt;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FICDropState> Drops;
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bEndingCleared = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) FName EndingKind;
};

USTRUCT(BlueprintType)
struct FICSaveSafety
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bInCombat = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bInDialogue = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bInEvent = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bDead = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEffectPending = false;
};

USTRUCT(BlueprintType)
struct FICSaveSlotInfo
{
    GENERATED_BODY()
    // 0 = auto, 1..4 = manual.
    UPROPERTY(BlueprintReadOnly) int32 Slot = 0;
    UPROPERTY(BlueprintReadOnly) bool bExists = false;
    UPROPERTY(BlueprintReadOnly) bool bValid = false;
    UPROPERTY(BlueprintReadOnly) bool bUsesBackup = false;
    UPROPERTY(BlueprintReadOnly) FString PlayerName;
    UPROPERTY(BlueprintReadOnly) FDateTime SavedAt;
};
