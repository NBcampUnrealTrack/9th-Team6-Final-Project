#pragma once
#include "CoreMinimal.h"
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimInstance;

namespace ICPlayerAppearance
{
    bool IsFirstPoseReady(USkeletalMeshComponent* Component);
    bool PrepareFirstPose(USkeletalMeshComponent* Component);
    bool Apply(USkeletalMeshComponent* Component,USkeletalMesh* Mesh,TSubclassOf<UAnimInstance> AnimClass,const FTransform& Transform);
}
