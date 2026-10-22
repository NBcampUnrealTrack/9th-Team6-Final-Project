#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "ICProjectile.generated.h"
class USphereComponent;
class UProjectileMovementComponent;
UCLASS()
class ISEKAICLASS_API AICProjectile : public AActor
{
    GENERATED_BODY()
public:
    AICProjectile();
    virtual void Tick(float Seconds) override;
    void InitializeShot(UICAbilitySystemComponent* Source, const FICSkillEffectRow& Effect, const FICSkillHitDefinition& Hit, const FICAttackCapture& Capture, FVector Direction, const FICCombatPresentation& Presentation);
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UProjectileMovementComponent> Movement;
    UFUNCTION() void Hit(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Result);
private:
    TWeakObjectPtr<UICAbilitySystemComponent> Source;
    FICSkillEffectRow Effect;
    FICCombatPresentation Presentation;
    FICAttackCapture Captured;
    TSet<TWeakObjectPtr<AActor>> HitActors;
    bool bPierce = false, bStopAtWorld = true, bEnemySource = false;
    double ExpiresAt = 0;
    int32 RemainingTargets = 1;
};
