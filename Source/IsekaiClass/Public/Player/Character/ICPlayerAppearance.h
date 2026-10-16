#pragma once
#include "CoreMinimal.h"
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimInstance;

namespace ICPlayerAppearance
{
    bool Apply(USkeletalMeshComponent* Component,USkeletalMesh* Mesh,TSubclassOf<UAnimInstance> AnimClass,const FTransform& Transform);
}
