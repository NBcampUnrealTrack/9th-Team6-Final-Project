#include "PlayerTimeSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "../Core/ICTimeSubsystem.h"

void UPlayerTimeSubsystem::Tick(float DeltaSeconds)
{
    if(auto* Instance=GetWorld()?GetWorld()->GetGameInstance():nullptr)
    {
        if(auto* Core=Instance->GetSubsystem<UICTimeSubsystem>())
        {const auto Clock=Core->GetClock();GameMinute=static_cast<int32>(FMath::FloorToInt64(Clock.GameMinute)%1440);PlaySeconds=static_cast<float>(Clock.PlaySecond);return;}
    }
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
    if(auto* Instance=GetWorld()?GetWorld()->GetGameInstance():nullptr)
    {
        if(auto* Core=Instance->GetSubsystem<UICTimeSubsystem>())
        {if(Core->JumpGameMinutes(Minutes)){GameMinute=static_cast<int32>(FMath::FloorToInt64(Core->GetClock().GameMinute)%1440);OnTimeJumped.Broadcast(From,GameMinute);}return;}
    }
    GameMinute=(GameMinute+Minutes)%1440;
    OnTimeJumped.Broadcast(From,GameMinute);
}

void UPlayerTimeSubsystem::Restore(int32 InGameMinute,float InPlaySeconds)
{
    if(auto* Instance=GetWorld()?GetWorld()->GetGameInstance():nullptr)
    {
        if(auto* Core=Instance->GetSubsystem<UICTimeSubsystem>())
        {FICClockState Clock;Clock.GameMinute=InGameMinute;Clock.PlaySecond=InPlaySeconds;if(!Core->RestoreClock(Clock))return;}
    }
    GameMinute=FMath::Clamp(InGameMinute,0,1439);
    PlaySeconds=FMath::Max(0.f,InPlaySeconds);
    GameMinuteAccumulator=0.f;
}
