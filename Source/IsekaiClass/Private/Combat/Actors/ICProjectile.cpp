#include "Combat/Actors/ICProjectile.h"
#include "Character/Base/ICCharacterBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Combat/Actors/ICProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"
AICProjectile::AICProjectile()
{
    IC_TRACE_FUNCTION();PrimaryActorTick.bCanEverTick=true;
    Collision=CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));SetRootComponent(Collision);Collision->InitSphereRadius(8);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));Collision->SetNotifyRigidBodyCollision(true);Collision->OnComponentHit.AddDynamic(this,&ThisClass::Hit);
    Movement=CreateDefaultSubobject<UICProjectileMovementComponent>(TEXT("ProjectileMovement"));Movement->ProjectileGravityScale=0;Movement->bRotationFollowsVelocity=true;
}
void AICProjectile::InitializeShot(UICAbilitySystemComponent* InSource,const FICSkillEffectRow& InEffect,const FICSkillHitDefinition& Rule,const FICAttackCapture& InCapture,FVector Direction)
{
    IC_TRACE_FUNCTION();Source=InSource;Effect=InEffect;Captured=InCapture;bPierce=Rule.bPierceTargets;bStopAtWorld=Rule.bStopAtWorldCollision;
    RemainingTargets=FMath::Max(1,Rule.MaximumTargets);bEnemySource=InSource&&InSource->GetAvatarActor()&&InSource->GetAvatarActor()->ActorHasTag(TEXT("IC.Enemy"));
    if(InSource&&InSource->GetAvatarActor())Collision->IgnoreActorWhenMoving(InSource->GetAvatarActor(),true);
    for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("IC.Enemy"))==bEnemySource)Collision->IgnoreActorWhenMoving(*It,true);
    Movement->Velocity=Direction.GetSafeNormal()*Rule.ProjectileSpeedCmPerSecond;Movement->InitialSpeed=Movement->MaxSpeed=Rule.ProjectileSpeedCmPerSecond;
    ExpiresAt=(InSource?InSource->PlaySecond():0)+Rule.ProjectileLifetimePlaySeconds;
}
void AICProjectile::Tick(float Seconds)
{
    IC_TRACE_FUNCTION();Super::Tick(Seconds);auto* Time=GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;
    if(Time&&Time->GetClock().PlaySecond>=ExpiresAt)Destroy();
}
void AICProjectile::Hit(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Result)
{
    IC_TRACE_FUNCTION();if(!Other||Other==GetOwner()||HitActors.Contains(Other))return;
    auto* Character=Cast<AICCharacterBase>(Other);
    if(Character&&Character->ActorHasTag(TEXT("IC.Enemy"))!=bEnemySource&&Character->GetICAbilitySystem())
    {
        HitActors.Add(Other);auto* Target=Character->GetICAbilitySystem();
        if(Source.IsValid())Source->ExecuteEffect(Effect,Target,&Captured);
        else if(Effect.Kind==EICSkillEffectKind::Damage)Target->ApplyDamage(Captured,nullptr,Effect.bCanCritical,Effect.bCanBeGuarded,Effect.bCanBeAutoDodged,Effect.bCancelsCastingOnEffectiveHit);
        if(!bPierce||--RemainingTargets<=0)Destroy();else {Collision->IgnoreActorWhenMoving(Other,true);Movement->Activate();}
    }
    else if(!Character&&bStopAtWorld)Destroy();
    else Collision->IgnoreActorWhenMoving(Other,true);
}
