#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ICGameMode.generated.h"
// Create BP_ICGameMode and select the project's Blueprint classes in Class Defaults.
UCLASS(Blueprintable)
class ISEKAICLASS_API AICGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AICGameMode();
};
