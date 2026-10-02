#pragma once
#include "CoreMinimal.h"
class USkeletalMeshComponent;

namespace PlayerAppearance
{
    bool Apply(USkeletalMeshComponent* Component,int32 Preset,float FeetZ);
}
