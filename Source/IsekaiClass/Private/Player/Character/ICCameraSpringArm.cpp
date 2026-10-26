#include "Player/Character/ICCameraSpringArm.h"

FVector UICCameraSpringArm::BlendLocations(const FVector& Desired,const FVector& Hit,bool bHit,float DeltaTime)
{
    const FVector Origin=GetComponentLocation()+TargetOffset;
    const FVector SafeLocation=bHit?Hit:Desired;
    const FVector Offset=SafeLocation-Origin;
    const float SafeDistance=Offset.Size();
    // Retract immediately for collision; ease only outward, never beyond the safe trace.
    PreviousDistance=PreviousDistance<0.f||SafeDistance<PreviousDistance?SafeDistance:
        FMath::FInterpTo(PreviousDistance,SafeDistance,DeltaTime,8.f);
    return Origin+Offset.GetSafeNormal()*FMath::Min(PreviousDistance,SafeDistance);
}
