#include "Life/Survival/ICPlayerSurvival.h"
#include "Debug/ICLog.h"

bool FPlayerSurvivalState::IsValid() const
{
    IC_TRACE_FUNCTION();
    return FMath::IsFinite(HP) && FMath::IsFinite(MaxHP) && FMath::IsFinite(MP) && FMath::IsFinite(MaxMP) && FMath::IsFinite(SP) && FMath::IsFinite(MaxSP)
        && MaxHP > 0.f && MaxMP > 0.f && MaxSP > 0.f
        && HP >= 0.f && HP <= MaxHP && MP >= 0.f && MP <= MaxMP && SP >= 0.f && SP <= MaxSP
        && FMath::IsFinite(SPDrainElapsedPlaySeconds) && SPDrainElapsedPlaySeconds >= 0 && SPDrainElapsedPlaySeconds < SPDrainIntervalPlaySeconds;
}

float FPlayerSurvivalState::RestoreHP(float Amount)
{
    IC_TRACE_FUNCTION();
    const float Before = HP;
    HP = FMath::Clamp(HP + FMath::Max(0.f, Amount), 0.f, MaxHP);
    UE_LOG(LogICCore, Verbose, TEXT("[Survival.RestoreHP] Before=%.2f After=%.2f Requested=%.2f"),Before,HP,Amount);
    return HP - Before;
}

float FPlayerSurvivalState::RestoreSP(float Amount)
{
    IC_TRACE_FUNCTION();
    const float Before = SP;
    SP = FMath::Clamp(SP + FMath::Max(0.f, Amount), 0.f, MaxSP);
    UE_LOG(LogICCore, Verbose, TEXT("[Survival.RestoreSP] Before=%.2f After=%.2f Requested=%.2f"),Before,SP,Amount);
    return SP - Before;
}

float FPlayerSurvivalState::ConsumeSP(float Amount)
{
    IC_TRACE_FUNCTION();
    const float Before = SP;
    SP = FMath::Clamp(SP - FMath::Max(0.f, Amount), 0.f, MaxSP);
    if(Before!=SP) UE_LOG(LogICCore, Verbose, TEXT("[Survival.ConsumeSP] Before=%.2f After=%.2f Requested=%.2f"),Before,SP,Amount);
    return Before - SP;
}

int32 FPlayerSurvivalState::CalculateSleepMinutes(float RecoveryPerGameMinute) const
{
    IC_TRACE_FUNCTION();
    if (!FMath::IsFinite(RecoveryPerGameMinute) || RecoveryPerGameMinute < 1.f) return 0;
    const int64 Recovery = FMath::FloorToInt64(RecoveryPerGameMinute);
    const int64 Missing = FMath::CeilToInt64(FMath::Max(0.f, MaxSP - SP));
    return static_cast<int32>(FMath::Clamp<int64>((Missing + Recovery - 1) / Recovery, 120, MAX_int32));
}

float FPlayerSurvivalState::AdvanceNaturalSP(double PlaySeconds)
{
    IC_TRACE_FUNCTION();
    if (!IsValid() || !FMath::IsFinite(PlaySeconds) || PlaySeconds <= 0) return 0;
    const double Total = SPDrainElapsedPlaySeconds + PlaySeconds;
    if (!FMath::IsFinite(Total)) return 0;
    SPDrainElapsedPlaySeconds = FMath::Fmod(Total, SPDrainIntervalPlaySeconds);
    return ConsumeSP(static_cast<float>(FMath::Min(FMath::FloorToDouble(Total / SPDrainIntervalPlaySeconds), static_cast<double>(SP))));
}

void FPlayerSurvivalState::CompleteSleep()
{
    IC_TRACE_FUNCTION();
    HP = MaxHP;
    MP = MaxMP;
    SP = MaxSP;
    SPDrainElapsedPlaySeconds = 0;
    UE_LOG(LogICCore, Log, TEXT("[Survival.Sleep] HP=%.2f MP=%.2f SP=%.2f: 완전 회복"),HP,MP,SP);
}
