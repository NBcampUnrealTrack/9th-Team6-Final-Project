#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ICCharacterAnimInstance.generated.h"

class UICCharacterAnimationProfile;
class UBlendSpace;
class UAnimSequence;

/** Shared native animation graph: locomotion -> airborne -> sleep -> DefaultSlot. */
UCLASS(Transient, Blueprintable)
class ISEKAICLASS_API UICCharacterAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;

    UPROPERTY(Transient, BlueprintReadOnly) TObjectPtr<UBlendSpace> Locomotion;
    UPROPERTY(Transient, BlueprintReadOnly) TObjectPtr<UAnimSequence> Jump;
    UPROPERTY(Transient, BlueprintReadOnly) TObjectPtr<UAnimSequence> Sleep;
    UPROPERTY(Transient, BlueprintReadOnly) float Speed = 0;
    UPROPERTY(Transient, BlueprintReadOnly) bool bAirborne = false;
    UPROPERTY(Transient, BlueprintReadOnly) bool bSleeping = false;
private:
    float PreviousHP = -1;
    FVector PreviousPosition = FVector::ZeroVector;
    bool bHasPreviousPosition = false;
};
