#include "Core/ICGameMode.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/ICGameState.h"
#include "Core/ICPlayerState.h"

AICGameMode::AICGameMode()
{
    DefaultPawnClass=AICPlayerCharacter::StaticClass();
    PlayerControllerClass=AICPlayerController::StaticClass();
    GameStateClass=AICGameState::StaticClass();
    PlayerStateClass=AICPlayerState::StaticClass();
}
