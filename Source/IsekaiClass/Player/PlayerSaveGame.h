#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerItemTypes.h"
#include "PlayerSurvival.h"
#include "PlayerSaveGame.generated.h"

ISEKAICLASS_API const TCHAR* GetPlayerSaveSlot();
ISEKAICLASS_API const TCHAR* GetPlayerManualSaveSlot(int32 Index);
UCLASS()
class ISEKAICLASS_API UPlayerPrototypeSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version=8;
    // V1 importer compatibility. Runtime saves use UICSaveSubsystem.
    UPROPERTY(SaveGame) float HP=60.f;
    UPROPERTY(SaveGame) float SP=70.f;
    UPROPERTY(SaveGame) int32 Strength=1;
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
    UPROPERTY(SaveGame) FPlayerSurvivalState Survival;
    UPROPERTY(SaveGame) int32 GameMinute=480;
    UPROPERTY(SaveGame) float PlaySeconds=0.f;
    UPROPERTY(SaveGame) FPlayerMainStats MainStats;
    UPROPERTY(SaveGame) int32 Gold=0;
};
