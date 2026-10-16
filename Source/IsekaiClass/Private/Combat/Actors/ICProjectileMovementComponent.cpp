#include "Combat/Actors/ICProjectileMovementComponent.h"
#include "Debug/ICLog.h"

UProjectileMovementComponent::EHandleBlockingHitResult UICProjectileMovementComponent::HandleBlockingHit(const FHitResult& Hit,float TimeTick,const FVector& MoveDelta,float& SubTickTimeRemaining)
{
    IC_TRACE_FUNCTION();
    auto* Actor=UpdatedComponent?UpdatedComponent->GetOwner():nullptr;
    if(!IsValid(Actor)||Actor->IsActorBeingDestroyed()||!CheckStillInWorld())return EHandleBlockingHitResult::Abort;
    // SafeMoveUpdatedComponent already delivered OnComponentHit to ICProjectile.
    SubTickTimeRemaining=TimeTick*(1.f-Hit.Time);
    return EHandleBlockingHitResult::AdvanceNextSubstep;
}
