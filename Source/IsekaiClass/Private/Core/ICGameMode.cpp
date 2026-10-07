#include "Core/ICGameMode.h"
#include "Debug/ICLog.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Core/Controller/ICPlayerController.h"
#include "Core/ICGameState.h"
#include "Core/ICPlayerState.h"
#include "Engine/World.h"

AICGameMode::AICGameMode()
{
    IC_TRACE_FUNCTION();
    DefaultPawnClass=AICPlayerCharacter::StaticClass();
    PlayerControllerClass=AICPlayerController::StaticClass();
    GameStateClass=AICGameState::StaticClass();
    PlayerStateClass=AICPlayerState::StaticClass();
    UE_LOG(LogICCore, VeryVerbose, TEXT("[GameMode.Defaults] Class=%s Pawn=%s Controller=%s GameState=%s PlayerState=%s"),*GetNameSafe(GetClass()),*GetNameSafe(DefaultPawnClass.Get()),*GetNameSafe(PlayerControllerClass.Get()),*GetNameSafe(GameStateClass.Get()),*GetNameSafe(PlayerStateClass.Get()));
}

void AICGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    IC_TRACE_FUNCTION();
    Super::InitGame(MapName, Options, ErrorMessage);
    if (IsMainMenuLevel()) DefaultPawnClass = nullptr;
    UE_LOG(LogICCore, Log, TEXT("[GameMode.Level] Map=%s MainMenu=%d Pawn=%s Controller=%s"),
        *MapName, IsMainMenuLevel(), *GetNameSafe(DefaultPawnClass.Get()), *GetNameSafe(PlayerControllerClass.Get()));
}

bool AICGameMode::IsMainMenuLevel() const
{
    IC_TRACE_FUNCTION();
    const UWorld* World = GetWorld();
    return World && !MainMenuMap.IsNull()
        && UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) == MainMenuMap.GetLongPackageName();
}

void AICGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    IC_TRACE_FUNCTION();
    // A menu controller needs no gameplay pawn or restart attempt.
    if (IsMainMenuLevel()) return;
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}
