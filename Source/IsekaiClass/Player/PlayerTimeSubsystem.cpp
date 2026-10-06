#include "PlayerTimeSubsystem.h"
#include "Engine/World.h"

void UPlayerTimeSubsystem::Tick(float DeltaSeconds)
{
    if(!GetWorld()||GetWorld()->IsPaused()||DeltaSeconds<=0.f)return;
    PlaySeconds+=DeltaSeconds;
    GameMinuteAccumulator+=DeltaSeconds*0.5f;
    const int32 Minutes=FMath::FloorToInt(GameMinuteAccumulator);
    if(Minutes<=0)return;
    GameMinuteAccumulator-=Minutes;
    GameMinute=(GameMinute+Minutes)%1440;
    OnGameMinutesAdvanced.Broadcast(Minutes);
}

void UPlayerTimeSubsystem::AdvanceGameMinutes(int32 Minutes)
{
    if(Minutes<=0)return;
    const int32 From=GameMinute;
    GameMinute=(GameMinute+Minutes)%1440;
    OnTimeJumped.Broadcast(From,GameMinute);
}

void UPlayerTimeSubsystem::Restore(int32 InGameMinute,float InPlaySeconds)
{
    GameMinute=FMath::Clamp(InGameMinute,0,1439);
    PlaySeconds=FMath::Max(0.f,InPlaySeconds);
    GameMinuteAccumulator=0.f;
}
