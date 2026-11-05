#pragma once

#include "CoreMinimal.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "World/State/Types/ICCharacterStateTypes.h"
#include "World/State/Types/ICInventoryStateTypes.h"
#include "World/State/Types/ICQuestStateTypes.h"
#include "World/State/Types/ICWorldProgressStateTypes.h"
#include "World/State/Types/ICClockStateTypes.h"
#include "World/State/Types/ICLifeStateTypes.h"
#include "World/State/Types/ICCodexStateTypes.h"
#include "ICWorldStateTypes.generated.h"

USTRUCT(BlueprintType)
struct FICWorldState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bPlayerCreated = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) FString PlayerName;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Preset = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Job = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) FName FirstSkill;
    UPROPERTY(BlueprintReadOnly, SaveGame) FICCreationAnswers CreationAnswers;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Gold=200;
    UPROPERTY(BlueprintReadOnly, SaveGame) FVector PlayerPosition = FVector(0, 0, 120);
    UPROPERTY(BlueprintReadOnly, SaveGame) FRotator PlayerRotation = FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICCharacterState> Characters;
    UPROPERTY(BlueprintReadOnly, SaveGame) FICInventoryState Inventory;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICQuestState> Quests;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> UnlockedWarps;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, double> MonsterKilledAt;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> DefeatedBosses;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, double> ResourceReadyAt;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, double> ResourceReadyAtGameMinute;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName,FICRanchAnimalState> RanchAnimals;
    UPROPERTY(BlueprintReadOnly,SaveGame) TMap<FName,double> FurnitureLootReadyAt;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FICDropState> Drops;

    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FGuid, FICProductionJobState> ProductionJobs;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICCropState> Crops;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICNPCFarmPlotState> NPCFarmPlots;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICSpawnSlotState> SpawnSlots;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICResourcePointState> ResourcePoints;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICContainerOwnershipState> ContainerOwnership;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, FICStorageInstallationState> StorageInstallations;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> DiscoveredRegionIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> DiscoveredAnchorIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> CodexEntryIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame) FICCodexState Codex;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FName> UnlockedRecipeIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, int32> ProgressionStages;
    UPROPERTY(BlueprintReadOnly, SaveGame) TMap<FName, bool> Flags;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FGuid> CompletedTransactionIDs;
    UPROPERTY(BlueprintReadOnly, SaveGame) TSet<FGuid> ProcessedEventIDs;
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
struct FICSavePreview
{
    GENERATED_BODY()
    // Stable region ID (currently R01); resolve its display name through a String Table.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FString Location;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FName> PartyIds;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<uint8> ThumbnailPNG;
};

USTRUCT(BlueprintType)
struct FICSaveSlotInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Slot = 0;
    UPROPERTY(BlueprintReadOnly) bool bExists = false;
    UPROPERTY(BlueprintReadOnly) bool bValid = false;
    UPROPERTY(BlueprintReadOnly) bool bUsesBackup = false;
    UPROPERTY(BlueprintReadOnly) FString PlayerName;
    UPROPERTY(BlueprintReadOnly) FDateTime SavedAt;
    UPROPERTY(BlueprintReadOnly) int32 Preset = 0;
    UPROPERTY(BlueprintReadOnly) FICSavePreview Preview;
    UPROPERTY(BlueprintReadOnly) FICClockState Clock;
};
