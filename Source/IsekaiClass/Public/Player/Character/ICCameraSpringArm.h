#pragma once
#include "GameFramework/SpringArmComponent.h"
#include "ICCameraSpringArm.generated.h"

UCLASS()
class ISEKAICLASS_API UICCameraSpringArm : public USpringArmComponent
{
    GENERATED_BODY()
protected:
    virtual FVector BlendLocations(const FVector& DesiredArmLocation,const FVector& TraceHitLocation,bool bHitSomething,float DeltaTime) override;
private:
    float PreviousDistance=-1.f;
};
