#pragma once
#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ICProjectileMovementComponent.generated.h"

UCLASS(meta=(ToolTip="IC 투사체의 관통 이동 컴포넌트입니다. 충돌 결과에 따른 파괴·무시 여부는 ICProjectile이 결정하며 살아 있는 관통 투사체의 속도와 남은 이동 시간을 보존합니다."))
class ISEKAICLASS_API UICProjectileMovementComponent : public UProjectileMovementComponent
{
    GENERATED_BODY()
protected:
    virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit,float TimeTick,const FVector& MoveDelta,float& SubTickTimeRemaining) override;
};
