#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerInventory.h"
#include "PlayerSaveGame.generated.h"

ISEKAICLASS_API const TCHAR* GetPlayerSaveSlot();
UCLASS()
class ISEKAICLASS_API UPlayerPrototypeSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version=1;
    UPROPERTY(SaveGame) FString PlayerName;
    UPROPERTY(SaveGame) int32 Preset=0;
    UPROPERTY(SaveGame) int32 Job=0;
    UPROPERTY(SaveGame) FName FirstSkill;
    UPROPERTY(SaveGame) FPlayerContainer Bag;
    UPROPERTY(SaveGame) TArray<FPlayerItemInstance> QuestItems;
    UPROPERTY(SaveGame) TArray<FPlayerItemInstance> Equipment;
    UPROPERTY(SaveGame) TArray<FName> QuickSlots;
    UPROPERTY(SaveGame) TMap<FName,FPlayerContainer> Storage;
    UPROPERTY(SaveGame) FVector Position;
    UPROPERTY(SaveGame) FRotator Rotation;
    UPROPERTY(SaveGame) float HP=60;
    UPROPERTY(SaveGame) float SP=70;
    UPROPERTY(SaveGame) int32 Strength=1;
};
