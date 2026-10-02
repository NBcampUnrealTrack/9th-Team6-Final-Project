#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlayerGameMode.generated.h"
UCLASS()
class ISEKAICLASS_API APlayerPrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    APlayerPrototypeGameMode();
    virtual void StartPlay() override;
};
