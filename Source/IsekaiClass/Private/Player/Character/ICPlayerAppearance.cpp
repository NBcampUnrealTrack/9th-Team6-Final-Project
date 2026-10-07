#include "Player/Character/ICPlayerAppearance.h"
#include "Debug/ICLog.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"

bool ICPlayerAppearance::Apply(USkeletalMeshComponent* Component,USkeletalMesh* Mesh,TSubclassOf<UAnimInstance> AnimClass,const FTransform& Transform)
{
    IC_TRACE_FUNCTION();
    if(!Component||!Mesh){UE_LOG(LogICCore, Verbose, TEXT("[Appearance.Apply.Rejected] Component=%s Mesh=%s"),*GetNameSafe(Component),*GetNameSafe(Mesh));return false;}
    Component->SetSkeletalMesh(Mesh);
    Component->SetRelativeTransform(Transform);
    if(AnimClass)Component->SetAnimInstanceClass(AnimClass);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetVisibility(true);Component->SetHiddenInGame(false);
    UE_LOG(LogICCore, Verbose, TEXT("[Appearance.Applied] Component=%s Mesh=%s AnimationClass=%s"),*GetNameSafe(Component),*GetPathNameSafe(Mesh),*GetNameSafe(AnimClass.Get()));
    return true;
}
