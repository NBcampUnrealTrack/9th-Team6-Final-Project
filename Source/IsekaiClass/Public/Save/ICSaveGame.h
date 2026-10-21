#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveGame.generated.h"

UCLASS()
class ISEKAICLASS_API UICSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UICSaveGame();
    static constexpr int32 CurrentVersion = 3;
    static bool EncodeForFile(UICSaveGame* Save, TArray<uint8>& Bytes);
    static UICSaveGame* DecodeFile(const TArray<uint8>& Bytes);
    static bool IsFutureFile(const TArray<uint8>& Bytes);
    bool UpgradeFromVersion2();
    UPROPERTY(SaveGame) int32 SaveVersion = CurrentVersion;
    UPROPERTY(SaveGame) int32 Slot = 0;
    UPROPERTY(SaveGame) FDateTime SavedAt;
    UPROPERTY(SaveGame) FICWorldState WorldState;
    UPROPERTY(SaveGame) FICClockState Clock;
    UPROPERTY(SaveGame) FICSavePreview Preview;
};
