#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerItemTypes.h"
#include "PlayerSurvival.h"
#include "PlayerWorldStateSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FPlayerPersistentSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,SaveGame) FName CharacterID=TEXT("CHR_PLAYER");
    UPROPERTY(BlueprintReadWrite,SaveGame) FPlayerMainStats MainStats;
    UPROPERTY(BlueprintReadWrite,SaveGame) FPlayerSurvivalState Survival;
    UPROPERTY(BlueprintReadWrite,SaveGame) int32 Gold=0;
    UPROPERTY(BlueprintReadWrite,SaveGame) FPlayerContainer Bag;
    UPROPERTY(BlueprintReadWrite,SaveGame) TArray<FPlayerItemInstance> QuestItems;
    UPROPERTY(BlueprintReadWrite,SaveGame) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(BlueprintReadWrite,SaveGame) TArray<FName> QuickSlots;
    UPROPERTY(BlueprintReadWrite,SaveGame) TMap<FName,FPlayerContainer> Storage;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICCharacterStateChanged,FName,CharacterID);

UCLASS()
class ISEKAICLASS_API UPlayerWorldStateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICCharacterStateChanged OnCharacterStateChanged;
    UPROPERTY(BlueprintReadOnly) TMap<FName,FPlayerPersistentSnapshot> Characters;
    UPROPERTY(BlueprintReadOnly) TArray<FName> PartySlots;
    UFUNCTION(BlueprintCallable) void SetSnapshot(const FPlayerPersistentSnapshot& Snapshot);
    UFUNCTION(BlueprintPure) bool GetSnapshot(FName CharacterID,FPlayerPersistentSnapshot& OutSnapshot) const;
    UFUNCTION(BlueprintCallable) int32 AddMainStat(FName CharacterID,EPlayerMainStat Stat,int32 Amount);
    UFUNCTION(BlueprintPure) FName GetPartyTarget(int32 PartyIndex) const;
};
