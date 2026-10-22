#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICWeaponActor.generated.h"

class UStaticMeshComponent;
struct FICEquipmentRow;

/** Reusable weapon behavior. Blueprint children provide the mesh and its grip/tip calibration. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICWeaponActor : public AActor
{
    GENERATED_BODY()
public:
    AICWeaponActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> Grip;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> Muzzle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Geometry") FVector LengthAxis = FVector::UpVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Geometry", meta=(ClampMin="0",ClampMax="1")) float HandleFraction = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Geometry") bool bAutoMuzzleAtTip = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Geometry", meta=(ClampMin="0")) float MeleeReachAllowanceCm = 70.f;
    void Configure(const FICEquipmentRow& Definition);
    UFUNCTION(BlueprintPure, Category="Weapon") float GetScaledLengthCm() const;
    UFUNCTION(BlueprintPure, Category="Weapon") float GetMeleeReachCm() const;
    UFUNCTION(BlueprintPure, Category="Weapon") FVector GetMuzzleLocation() const;
    UFUNCTION(BlueprintPure, Category="Weapon") void GetBladeTrace(FVector& Start, FVector& End) const;
private:
    void RefreshGeometry();
};
