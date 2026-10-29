#include "Core/Controller/ICPlayerController.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "EngineUtils.h"

void AICPlayerController::NotifyPlayerHitEnemy(AICCharacterBase* Target)
{
    if(!Cast<AICEnemyBase>(Target))return;
    LastAttackedHUDTarget=Target;
    AttackedHUDTargetUntil=(GetTime()?GetTime()->GetClock().PlaySecond:0.)+8.;
}
AICCharacterBase* AICPlayerController::GetHUDCombatTarget() const
{
    auto Living=[](AICCharacterBase* Actor)
    {
        const auto* Enemy=Cast<AICEnemyBase>(Actor);
        return Enemy&&!Enemy->IsReturning()&&Enemy->GetICAbilitySystem()&&Enemy->GetICAbilitySystem()->IsAlive();
    };
    if(bDead||!bCreated)return nullptr;
    if(bExplicitCombatTarget&&Living(CombatTarget.Get()))return CombatTarget.Get();
    const double Now=GetTime()?GetTime()->GetClock().PlaySecond:0.;
    return Now<AttackedHUDTargetUntil&&Living(LastAttackedHUDTarget.Get())?LastAttackedHUDTarget.Get():nullptr;
}

AICEnemyBase* AICPlayerController::GetHUDBossTarget() const
{
    if(bDead||!bCreated||!GetPawn())return nullptr;
    auto Eligible=[this](AICEnemyBase* Enemy)
    {
        return IsValid(Enemy)&&!Enemy->IsReturning()&&Enemy->GetMonsterDefinition().Kind!=EICMonsterKind::Normal
            &&Enemy->GetICAbilitySystem()&&Enemy->GetICAbilitySystem()->IsAlive()&&GetPawn()->GetDistanceTo(Enemy)<6000.f;
    };
    if(auto* Target=Cast<AICEnemyBase>(GetHUDCombatTarget());Eligible(Target))return Target;
    AICEnemyBase* Best=nullptr;float Distance=MAX_flt;
    for(TActorIterator<AICEnemyBase> It(GetWorld());It;++It)
        if(Eligible(*It)&&It->GetICAbilitySystem()->HasRecentCombat())
        {const float Candidate=GetPawn()->GetDistanceTo(*It);if(Candidate<Distance){Distance=Candidate;Best=*It;}}
    return Best;
}
