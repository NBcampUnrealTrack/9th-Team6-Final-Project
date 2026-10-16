#include "Core/ICGameState.h"
#include "Debug/ICLog.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AICGameState::AICGameState()
{
    IC_TRACE_FUNCTION();
    PrimaryActorTick.bCanEverTick=false;
}

void AICGameState::BeginPlay()
{
    IC_TRACE_FUNCTION();
    Super::BeginPlay();
    UE_LOG(LogICCore, Log, TEXT("[GameState.BeginPlay] Object=%s Time=%s"),*GetNameSafe(this),*GetNameSafe(GetTime()));
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
    IC_TRACE_FUNCTION();
    if (UICTimeSubsystem* Time=GetTime())
    {
        Time->OnClockChanged.RemoveDynamic(this,&ThisClass::RefreshClock);
        Time->OnGameMinutesAdvanced.RemoveDynamic(this,&ThisClass::HandleMinutesAdvanced);
        Time->OnTimeJumped.RemoveDynamic(this,&ThisClass::HandleTimeJumped);
    }
    Super::EndPlay(Reason);
    UE_LOG(LogICCore, Log, TEXT("[GameState.EndPlay] Object=%s Reason=%d: 시간 이벤트 구독 정리"),*GetNameSafe(this),static_cast<int32>(Reason));
}

UICTimeSubsystem* AICGameState::GetTime() const
{ IC_TRACE_FUNCTION(); return GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr; }

void AICGameState::RefreshClock()
{ IC_TRACE_FUNCTION(); if(const UICTimeSubsystem* Time=GetTime())GameMinute=static_cast<int32>(FMath::FloorToInt64(Time->GetClock().GameMinute)%1440); }

void AICGameState::HandleMinutesAdvanced(int32 Minutes)
{ IC_TRACE_FUNCTION(); RefreshClock();OnGameMinutesAdvanced.Broadcast(Minutes); }

void AICGameState::HandleTimeJumped(double From,double To)
{ IC_TRACE_FUNCTION(); RefreshClock();OnTimeJumped.Broadcast(static_cast<int32>(FMath::FloorToInt64(From)%1440),GameMinute); }

void AICGameState::SetPhase(EICGamePhase NewPhase)
{
    IC_TRACE_FUNCTION();
    if(Phase==NewPhase)return;
    UE_LOG(LogICCore, Log, TEXT("[Game.Phase] From=%d To=%d"),static_cast<int32>(Phase),static_cast<int32>(NewPhase));
    Phase=NewPhase;
    OnPhaseChanged.Broadcast(Phase);
}

void AICGameState::AdvanceGameMinutes(int32 Minutes)
{
    IC_TRACE_FUNCTION();
    if(Minutes>0)if(UICTimeSubsystem* Time=GetTime())Time->JumpGameMinutes(Minutes);
}
