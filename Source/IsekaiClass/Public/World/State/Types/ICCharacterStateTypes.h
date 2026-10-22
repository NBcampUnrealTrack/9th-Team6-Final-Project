#pragma once
#include "CoreMinimal.h"
#include "Debug/ICLog.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "World/State/Types/ICCombatStateTypes.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICCharacterStateTypes.generated.h"

UENUM(BlueprintType)
enum class EICMainStat : uint8 { STR , DEX , INT , CON , CHA UMETA(Hidden), LUK  };

UENUM(BlueprintType)
enum class EICCharacterKind : uint8 { Player , Recruitable , Resident , Background  };

UENUM(BlueprintType)
enum class EICCharacterMode : uint8 { Scheduled , Companion , Downed UMETA(Hidden) };

UENUM(BlueprintType)
enum class EICCharacterLifeState : uint8 { Alive , Downed  };

USTRUCT(BlueprintType)
struct FICMainStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 STR = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 DEX = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 INT = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 CON = 1;
    UPROPERTY(SaveGame) int32 CHA = 1; // Preserve old saves without exposing retired gameplay.
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICCharacterLifeState LifeState = EICCharacterLifeState::Alive;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 PartySlot = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Affinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FICMainStats MainStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float HP = 60;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float SP = 70;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxHP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxMP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float MaxSP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double LastSimGameMinute = 480;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double LastSimPlaySecond = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) double SPDrainElapsedPlaySeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName CurrentAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FVector Position = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TSet<FName> UnlockedSkillIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<EICWeaponFamily, FName> EquippedWeaponSkills;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICAttackDisposition AttackDisposition = EICAttackDisposition::Defensive;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICLowHPDisposition LowHPDisposition = EICLowHPDisposition::Wait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EICTargetPriority TargetPriority = EICTargetPriority::PlayerTargetFirst;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bAutoSkillsEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FICCombatState Combat;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FICDebuffState> Debuffs;
    bool IsDowned() const { IC_TRACE_FUNCTION(); return LifeState == EICCharacterLifeState::Downed; }
    bool HasStats() const { IC_TRACE_FUNCTION(); return Kind == EICCharacterKind::Player || Kind == EICCharacterKind::Recruitable; }
};
