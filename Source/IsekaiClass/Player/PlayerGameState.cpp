#include "PlayerGameState.h"
#include "PlayerTimeSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

APlayerPrototypeGameState::APlayerPrototypeGameState()
{
    PrimaryActorTick.bCanEverTick=true;
}

void APlayerPrototypeGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* Time=GetWorld()?GetWorld()->GetSubsystem<UPlayerTimeSubsystem>():nullptr;if(!Time)return;
    if(auto* Instance=GetGameInstance())
    {if(auto* Core=Instance->GetSubsystem<UICTimeSubsystem>())GameMinute=static_cast<int32>(FMath::FloorToInt64(Core->GetClock().GameMinute)%1440);else GameMinute=Time->GameMinute;}
    else GameMinute=Time->GameMinute;
    const int32 Delta=(GameMinute-LastBroadcastMinute+1440)%1440;
    if(Delta>0&&Delta<720)OnGameMinutesAdvanced.Broadcast(Delta);
    LastBroadcastMinute=GameMinute;
}

void APlayerPrototypeGameState::SetPhase(EPlayerGamePhase NewPhase)
{
    Phase=NewPhase;
}

void APlayerPrototypeGameState::AdvanceGameMinutes(int32 Minutes)
{
    if(Minutes<=0)return;const int32 From=GameMinute;
    if(auto* Time=GetWorld()?GetWorld()->GetSubsystem<UPlayerTimeSubsystem>():nullptr){Time->AdvanceGameMinutes(Minutes);GameMinute=Time->GameMinute;LastBroadcastMinute=GameMinute;}
    OnTimeJumped.Broadcast(From,GameMinute);
}
