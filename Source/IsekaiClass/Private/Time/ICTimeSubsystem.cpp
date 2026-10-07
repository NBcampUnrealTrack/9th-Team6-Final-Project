#include "Time/ICTimeSubsystem.h"
#include "Debug/ICLog.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UICTimeSubsystem::Tick(float DeltaTime)
{
    IC_TRACE_FUNCTION();
    PauseRequests.RemoveAll([](const FPauseRequest& Request) { IC_TRACE_FUNCTION(); return !Request.Owner.IsValid(); });
    UpdateWorldPause();
    AdvanceClock(DeltaTime, !GetWorld() || GetWorld()->IsPaused());
}

bool UICTimeSubsystem::IsTickable() const
{
    IC_TRACE_FUNCTION();
    return !IsTemplate() && GetWorld() && GetWorld()->IsGameWorld();
}

TStatId UICTimeSubsystem::GetStatId() const { IC_TRACE_FUNCTION(); RETURN_QUICK_DECLARE_CYCLE_STAT(UICTimeSubsystem, STATGROUP_Tickables); }

void UICTimeSubsystem::AdvanceClock(double DeltaSeconds, bool bWorldPaused)
{
    IC_TRACE_FUNCTION();
    if (!IsRunning() || bWorldPaused || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    const int64 PreviousMinute = FMath::FloorToInt64(Clock.GameMinute);
    const double GameNext = Clock.GameMinute + DeltaSeconds * 0.5;
    const double PlayNext = Clock.PlaySecond + DeltaSeconds;
    if (!FMath::IsFinite(GameNext) || !FMath::IsFinite(PlayNext)) return;
    Clock.GameMinute = GameNext; Clock.PlaySecond = PlayNext;
    OnPlaySecondsAdvanced.Broadcast(DeltaSeconds);
    const int64 Elapsed=FMath::FloorToInt64(GameNext)-PreviousMinute;
    if(Elapsed>0){OnGameMinutesAdvanced.Broadcast(static_cast<int32>(FMath::Min<int64>(Elapsed,MAX_int32)));OnClockChanged.Broadcast();}
}

bool UICTimeSubsystem::JumpGameMinutes(double Minutes)
{
    IC_TRACE_FUNCTION();
    if (!FMath::IsFinite(Minutes) || Minutes < 0 || !FMath::IsFinite(Clock.GameMinute + Minutes)){UE_LOG(LogICTime, Verbose, TEXT("[Time.Jump.Rejected] Minutes=%.2f Current=%.2f"),Minutes,Clock.GameMinute);return false;}
    const double From = Clock.GameMinute;
    Clock.GameMinute += Minutes;
    UE_LOG(LogICTime, Log, TEXT("[Time.Jump] From=%.2f To=%.2f PlaySeconds=%.2f"),From,Clock.GameMinute,Clock.PlaySecond);
    OnTimeJumped.Broadcast(From, Clock.GameMinute);
    OnClockChanged.Broadcast();
    return true;
}

bool UICTimeSubsystem::RestoreClock(const FICClockState& Saved)
{
    IC_TRACE_FUNCTION();
    if (!Saved.IsValid()){UE_LOG(LogICTime, Verbose, TEXT("[Time.Restore.Rejected] GameMinute=%.2f PlaySecond=%.2f"),Saved.GameMinute,Saved.PlaySecond);return false;}
    Clock = Saved;
    UE_LOG(LogICTime, Log, TEXT("[Time.Restored] GameMinute=%.2f PlaySecond=%.2f"),Clock.GameMinute,Clock.PlaySecond);
    OnClockChanged.Broadcast();
    return true;
}

void UICTimeSubsystem::ResetClock() { IC_TRACE_FUNCTION(); RestoreClock(FICClockState()); }

void UICTimeSubsystem::SetPauseRequest(UObject* Owner, FName Reason, bool Paused)
{
    IC_TRACE_FUNCTION();
    if (!IsValid(Owner) || Reason.IsNone()){UE_LOG(LogICTime, Verbose, TEXT("[Time.Pause.Rejected] Owner=%s Reason=%s"),*GetNameSafe(Owner),*Reason.ToString());return;}
    const bool bHadRequest=PauseRequests.ContainsByPredicate([Owner,Reason](const FPauseRequest& Request){ IC_TRACE_FUNCTION();return Request.Owner.Get()==Owner && Request.Reason==Reason;});
    PauseRequests.RemoveAll([Owner, Reason](const FPauseRequest& Request)
    { IC_TRACE_FUNCTION(); return !Request.Owner.IsValid() || (Request.Owner.Get() == Owner && Request.Reason == Reason); });
    if (Paused) PauseRequests.Add({Owner, Reason});
    if(bHadRequest!=Paused) UE_LOG(LogICTime, Log, TEXT("[Time.Pause] Owner=%s Reason=%s Paused=%d Requests=%d Running=%d"),*GetNameSafe(Owner),*Reason.ToString(),Paused,PauseRequests.Num(),IsRunning());
    UpdateWorldPause();
}

void UICTimeSubsystem::ReleasePauseRequests(UObject* Owner)
{
    IC_TRACE_FUNCTION();
    const int32 Before=PauseRequests.Num();
    PauseRequests.RemoveAll([Owner](const FPauseRequest& Request)
    { IC_TRACE_FUNCTION(); return !Request.Owner.IsValid() || Request.Owner.Get() == Owner; });
    if(Before!=PauseRequests.Num()) UE_LOG(LogICTime, Log, TEXT("[Time.PauseReleased] Owner=%s Removed=%d Remaining=%d Running=%d"),*GetNameSafe(Owner),Before-PauseRequests.Num(),PauseRequests.Num(),IsRunning());
    UpdateWorldPause();
}

void UICTimeSubsystem::PrepareForTravel()
{
    IC_TRACE_FUNCTION();
    bRunning = false;
    bMenuPaused = false;
    PauseRequests.Reset();
    UE_LOG(LogICTime, Log, TEXT("[Time.Travel] 시간 진행 중지 및 정지 요청 정리"));
    UpdateWorldPause();
}

void UICTimeSubsystem::UpdateWorldPause()
{
    IC_TRACE_FUNCTION();
    UWorld* World = GetWorld();
    if (PausedWorld.Get() != World) { bOwnsWorldPause = false; PausedWorld = World; }
    if (!World || !World->IsGameWorld()) return;
    if (!PauseRequests.IsEmpty() && !World->IsPaused())
    {
        bOwnsWorldPause = UGameplayStatics::SetGamePaused(World, true);
        UE_LOG(LogICTime, Verbose, TEXT("[Time.WorldPause] World=%s Applied=%d Requests=%d"),*GetNameSafe(World),bOwnsWorldPause,PauseRequests.Num());
    }
    else if (PauseRequests.IsEmpty() && bOwnsWorldPause)
    {
        UGameplayStatics::SetGamePaused(World, false);
        bOwnsWorldPause = false;
    }
}

void UICTimeSubsystem::SetRunning(bool Running)
{
    IC_TRACE_FUNCTION();
    if(bRunning!=Running)UE_LOG(LogICTime, Log, TEXT("[Time.Running] Before=%d After=%d PauseRequests=%d MenuPaused=%d"),bRunning,Running,PauseRequests.Num(),bMenuPaused);
    bRunning=Running;
}
void UICTimeSubsystem::SetMenuPaused(bool Paused)
{
    IC_TRACE_FUNCTION();
    if(bMenuPaused!=Paused)UE_LOG(LogICTime, Log, TEXT("[Time.MenuPause] Before=%d After=%d PauseRequests=%d"),bMenuPaused,Paused,PauseRequests.Num());
    bMenuPaused=Paused;
}
