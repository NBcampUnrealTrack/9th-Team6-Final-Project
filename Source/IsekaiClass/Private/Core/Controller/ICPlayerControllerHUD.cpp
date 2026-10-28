#include "Core/Controller/ICPlayerController.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Time/ICTimeSubsystem.h"

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
