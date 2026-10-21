#pragma once
#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ICProjectileMovementComponent.generated.h"

UCLASS()
class ISEKAICLASS_API UICProjectileMovementComponent : public UProjectileMovementComponent
{
    GENERATED_BODY()
protected:
    virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit,float TimeTick,const FVector& MoveDelta,float& SubTickTimeRemaining) override;
};
