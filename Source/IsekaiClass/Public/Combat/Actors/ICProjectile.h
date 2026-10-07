#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "ICProjectile.generated.h"
class USphereComponent;
class UProjectileMovementComponent;
UCLASS(meta=(ToolTip="공통 전투 투사체입니다. 발사 시 공격 수치를 고정하고 적중 시 방어자를 검사합니다. 월드 충돌·첫 유효 대상 또는 정의된 PlayClock 수명에서 종료합니다."))
class ISEKAICLASS_API AICProjectile : public AActor
{
    GENERATED_BODY()
public:
    AICProjectile();
    virtual void Tick(float Seconds) override;
    void InitializeShot(UICAbilitySystemComponent* Source, const FICSkillEffectRow& Effect, const FICSkillHitDefinition& Hit, const FICAttackCapture& Capture, FVector Direction);
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(ToolTip="투사체 충돌 구체입니다. 벽과 대상 충돌을 감지하며 연출 메시는 파생 BP에 추가할 수 있습니다.")) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(ToolTip="투사체 이동 컴포넌트입니다. 속도는 정의의 cm/s 값을 사용하고 중력은 적용하지 않습니다.")) TObjectPtr<UProjectileMovementComponent> Movement;
    UFUNCTION(meta=(ToolTip="투사체 충돌을 처리하는 내부 콜백입니다. 발사 시 캡처된 수치를 쓰고 같은 대상을 중복 처리하지 않습니다.\n@param Component 충돌을 보고한 투사체 컴포넌트입니다.\n@param Other 충돌한 액터입니다.\n@param OtherComponent 대상의 충돌 컴포넌트입니다.\n@param Impulse 물리 충돌 충격량입니다.\n@param Result 충돌 위치·법선·대상이 들어 있는 검사 결과입니다.")) void Hit(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Result);
private:
    TWeakObjectPtr<UICAbilitySystemComponent> Source;
    FICSkillEffectRow Effect;
    FICAttackCapture Captured;
    TSet<TWeakObjectPtr<AActor>> HitActors;
    bool bPierce = false, bStopAtWorld = true, bEnemySource = false;
    double ExpiresAt = 0;
    int32 RemainingTargets = 1;
};
