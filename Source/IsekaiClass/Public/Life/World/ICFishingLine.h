#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICFishingLine.generated.h"
class AICWeaponActor;
class AICLifeActor;
class UStaticMeshComponent;

/** A rod-tip tether and float. The float's fixed landing point determines the fishing zone. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICFishingLine : public AActor
{
    GENERATED_BODY()
public:
    AICFishingLine();
    virtual void Tick(float DeltaSeconds) override;
    void InitializeLine(AICWeaponActor* Rod,AICLifeActor* Zone,const FVector& Landing);
    UFUNCTION(BlueprintPure,Category="Fishing") bool HasValidLanding() const;
    UFUNCTION(BlueprintPure,Category="Fishing") FVector GetLandingPoint() const {return LandingPoint;}
    UFUNCTION(BlueprintPure,Category="Fishing") FVector GetRodTip() const;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Fishing",meta=(ClampMin="0.05")) float LineRadiusCm=.22f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Fishing",meta=(ClampMin="0")) float SagCm=18.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Fishing",meta=(ClampMin="1")) float FloatDiameterCm=7.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> FloatMesh;
private:
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Segments;
    TWeakObjectPtr<AICWeaponActor> SourceRod;
    TWeakObjectPtr<AICLifeActor> FishingZone;
    FVector LandingPoint=FVector::ZeroVector;
};
