#include "Player/Character/ICPlayerAppearance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"

bool ICPlayerAppearance::Apply(USkeletalMeshComponent* Component,USkeletalMesh* Mesh,TSubclassOf<UAnimInstance> AnimClass,const FTransform& Transform)
{
    if(!Component||!Mesh)return false;
    Component->SetSkeletalMesh(Mesh);
    Component->SetRelativeTransform(Transform);
    if(AnimClass)Component->SetAnimInstanceClass(AnimClass);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetVisibility(true);Component->SetHiddenInGame(false);
    return true;
}
