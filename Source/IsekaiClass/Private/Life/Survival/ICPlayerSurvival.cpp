#include "Life/Survival/ICPlayerSurvival.h"

bool FPlayerSurvivalState::IsValid() const
{
    return FMath::IsFinite(HP) && FMath::IsFinite(MaxHP) && FMath::IsFinite(MP) && FMath::IsFinite(MaxMP) && FMath::IsFinite(SP) && FMath::IsFinite(MaxSP)
        && MaxHP > 0.f && MaxMP > 0.f && MaxSP > 0.f
        && HP >= 0.f && HP <= MaxHP && MP >= 0.f && MP <= MaxMP && SP >= 0.f && SP <= MaxSP;
}

float FPlayerSurvivalState::RestoreHP(float Amount)
{
    const float Before = HP;
    HP = FMath::Clamp(HP + FMath::Max(0.f, Amount), 0.f, MaxHP);
    return HP - Before;
}

float FPlayerSurvivalState::RestoreSP(float Amount)
{
    const float Before = SP;
    SP = FMath::Clamp(SP + FMath::Max(0.f, Amount), 0.f, MaxSP);
    return SP - Before;
}

float FPlayerSurvivalState::ConsumeSP(float Amount)
{
    const float Before = SP;
    SP = FMath::Clamp(SP - FMath::Max(0.f, Amount), 0.f, MaxSP);
    return Before - SP;
}

int32 FPlayerSurvivalState::CalculateSleepMinutes(float RecoveryPerGameMinute) const
{
    if (RecoveryPerGameMinute <= 0.f || SP >= MaxSP) return 0;
    return FMath::CeilToInt((MaxSP - SP) / RecoveryPerGameMinute);
}

void FPlayerSurvivalState::CompleteSleep()
{
    HP = MaxHP;
    MP = MaxMP;
    SP = MaxSP;
}
