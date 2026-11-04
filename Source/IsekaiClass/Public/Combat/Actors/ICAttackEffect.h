#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICAttackEffect.generated.h"

class AICCharacterBase;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AICWeaponActor;

/** Short-lived, cooked mesh effects: directional sword arcs and projectile impacts. */
UCLASS()
class ISEKAICLASS_API AICAttackEffect : public AActor
{
    GENERATED_BODY()
public:
    AICAttackEffect();
    virtual void Tick(float Seconds) override;
    static FLinearColor FamilyColor(EICWeaponFamily Family);
    static void SpawnSlash(AICCharacterBase* Character,double Radius,double Angle,int32 Stage,const FICCombatPresentation& Presentation=FICCombatPresentation());
    static void SpawnImpact(UWorld* World,const FVector& Location,EICWeaponFamily Family);
    void UseNiagaraLifetime(float Seconds);
    bool IsFollowingSword() const { return bFollowSword; }
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Glow;
    UPROPERTY() TSoftObjectPtr<UMaterialInterface> GlowMaterial;
    FLinearColor Color;
    float Age=0, Duration=.24f;
    bool bImpact=false;
    bool bNiagara=false;
    bool bFollowSword=false;
    TWeakObjectPtr<AICCharacterBase> SwordOwner;
    TWeakObjectPtr<AICWeaponActor> SwordWeapon;
    FGuid SwordAction;
    FVector PreviousSwordTip=FVector::ZeroVector;
    FVector SwingTangent=FVector::RightVector;
    bool InitializeSwordSwing(AICCharacterBase* Character);
    void UpdateSwordSwing();
    void SetColor(FLinearColor Value);
    void Line(int32 Index,FVector Start,FVector End,float Width);
};
