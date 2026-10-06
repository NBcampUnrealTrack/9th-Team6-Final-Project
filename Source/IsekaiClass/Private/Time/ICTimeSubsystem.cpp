#include "Time/ICTimeSubsystem.h"
#include "Engine/World.h"

void UICTimeSubsystem::Tick(float DeltaTime)
{
    AdvanceClock(DeltaTime, !GetWorld() || GetWorld()->IsPaused());
}

bool UICTimeSubsystem::IsTickable() const
{
    return !IsTemplate() && GetWorld() && GetWorld()->IsGameWorld() && IsRunning();
}

TStatId UICTimeSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UICTimeSubsystem, STATGROUP_Tickables); }

void UICTimeSubsystem::AdvanceClock(double DeltaSeconds, bool bWorldPaused)
{
    if (!IsRunning() || bWorldPaused || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    const int64 PreviousMinute = FMath::FloorToInt64(Clock.GameMinute);
    const double GameNext = Clock.GameMinute + DeltaSeconds * 0.5;
    const double PlayNext = Clock.PlaySecond + DeltaSeconds;
    if (!FMath::IsFinite(GameNext) || !FMath::IsFinite(PlayNext)) return;
    Clock.GameMinute = GameNext; Clock.PlaySecond = PlayNext;
    const int64 Elapsed=FMath::FloorToInt64(GameNext)-PreviousMinute;
    if(Elapsed>0){OnGameMinutesAdvanced.Broadcast(static_cast<int32>(FMath::Min<int64>(Elapsed,MAX_int32)));OnClockChanged.Broadcast();}
}

bool UICTimeSubsystem::JumpGameMinutes(double Minutes)
{
    if (!FMath::IsFinite(Minutes) || Minutes < 0 || !FMath::IsFinite(Clock.GameMinute + Minutes)) return false;
    const double From = Clock.GameMinute;
    Clock.GameMinute += Minutes;
    OnTimeJumped.Broadcast(From, Clock.GameMinute);
    OnClockChanged.Broadcast();
    return true;
}

bool UICTimeSubsystem::RestoreClock(const FICClockState& Saved)
{
    if (!Saved.IsValid()) return false;
    Clock = Saved;
    OnClockChanged.Broadcast();
    return true;
}

void UICTimeSubsystem::ResetClock() { RestoreClock(FICClockState()); }
