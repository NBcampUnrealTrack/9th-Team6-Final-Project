#include "Core/ICMainMenuGameMode.h"
#include "Player/ICMainMenuPlayerController.h"

AICMainMenuGameMode::AICMainMenuGameMode()
{
    PlayerControllerClass = AICMainMenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
}
