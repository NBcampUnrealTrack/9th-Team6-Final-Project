#include "Core/ICMainMenuGameMode.h"
#include "Core/Controller/ICMainMenuPlayerController.h"

AICMainMenuGameMode::AICMainMenuGameMode()
{
    PlayerControllerClass = AICMainMenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
}
