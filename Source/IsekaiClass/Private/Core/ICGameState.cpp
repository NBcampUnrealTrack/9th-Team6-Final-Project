#include "Core/ICGameState.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AICGameState::AICGameState()
{
    PrimaryActorTick.bCanEverTick=false;
}

void AICGameState::BeginPlay()
{
    Super::BeginPlay();
    if (UICTimeSubsystem* Time=GetTime())
    {
        Time->OnClockChanged.AddUniqueDynamic(this,&ThisClass::RefreshClock);
        Time->OnGameMinutesAdvanced.AddUniqueDynamic(this,&ThisClass::HandleMinutesAdvanced);
        Time->OnTimeJumped.AddUniqueDynamic(this,&ThisClass::HandleTimeJumped);
        RefreshClock();
    }
}

void AICGameState::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UICTimeSubsystem* Time=GetTime())
    {
        Time->OnClockChanged.RemoveDynamic(this,&ThisClass::RefreshClock);
        Time->OnGameMinutesAdvanced.RemoveDynamic(this,&ThisClass::HandleMinutesAdvanced);
        Time->OnTimeJumped.RemoveDynamic(this,&ThisClass::HandleTimeJumped);
    }
    Super::EndPlay(Reason);
}

UICTimeSubsystem* AICGameState::GetTime() const
{ return GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr; }

void AICGameState::RefreshClock()
{ if(const UICTimeSubsystem* Time=GetTime())GameMinute=static_cast<int32>(FMath::FloorToInt64(Time->GetClock().GameMinute)%1440); }

void AICGameState::HandleMinutesAdvanced(int32 Minutes)
{ RefreshClock();OnGameMinutesAdvanced.Broadcast(Minutes); }

void AICGameState::HandleTimeJumped(double From,double To)
{ RefreshClock();OnTimeJumped.Broadcast(static_cast<int32>(FMath::FloorToInt64(From)%1440),GameMinute); }

void AICGameState::SetPhase(EICGamePhase NewPhase)
{
    if(Phase==NewPhase)return;
    Phase=NewPhase;
    OnPhaseChanged.Broadcast(Phase);
}

void AICGameState::AdvanceGameMinutes(int32 Minutes)
{
    if(Minutes>0)if(UICTimeSubsystem* Time=GetTime())Time->JumpGameMinutes(Minutes);
}
