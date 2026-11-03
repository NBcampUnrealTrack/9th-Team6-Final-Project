#include "Player/Character/ICPlayerAppearance.h"
#include "Debug/ICLog.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Animation/BlendSpace.h"

bool ICPlayerAppearance::IsFirstPoseReady(USkeletalMeshComponent* Component)
{
    auto* Mesh=Component?Component->GetSkeletalMeshAsset():nullptr;
    auto* Anim=Component?Cast<UICCharacterAnimInstance>(Component->GetAnimInstance()):nullptr;
    if(!Mesh||!Anim||!Anim->Locomotion||Component->IsRunningParallelEvaluation())return false;
    const auto Pose=Component->GetBoneSpaceTransformsView();
    const auto& Skeleton=Mesh->GetRefSkeleton();const auto& Reference=Skeleton.GetRefBonePose();
    // A created AnimInstance can still contain only the mesh's reference T-pose.
    for(const TCHAR* Name:{TEXT("J_Bip_L_UpperArm"),TEXT("J_Bip_R_UpperArm"),TEXT("J_Bip_L_LowerArm"),TEXT("J_Bip_R_LowerArm")})
    {
        const int32 Index=Skeleton.FindBoneIndex(Name);
        if(Pose.IsValidIndex(Index)&&Reference.IsValidIndex(Index)&&!Pose[Index].GetRotation().Equals(Reference[Index].GetRotation(),.01f))return true;
    }
    return false;
}

bool ICPlayerAppearance::PrepareFirstPose(USkeletalMeshComponent* Component)
{
    if(!Component||!Component->IsRegistered()||!Component->GetSkeletalMeshAsset()||!Component->GetAnimInstance())return false;
#if WITH_EDITOR
    if(Component->GetSkeletalMeshAsset()->IsCompiling())return false;
#endif
    Component->HandleExistingParallelEvaluationTask(true,true);
    // Evaluate explicitly: the loading menu pauses the world, so ordinary mesh
    // ticks cannot produce the first pose before the covering screen closes.
    Component->TickAnimation(1.f/60.f,false);
    if(auto* Anim=Cast<UICCharacterAnimInstance>(Component->GetAnimInstance()))
    {
        if(!Anim->Locomotion)return false;
    }
    Component->RefreshBoneTransforms();
    Component->HandleExistingParallelEvaluationTask(true,true);
    Component->UpdateBounds();Component->MarkRenderDynamicDataDirty();
    return IsFirstPoseReady(Component);
}

bool ICPlayerAppearance::Apply(USkeletalMeshComponent* Component,USkeletalMesh* Mesh,TSubclassOf<UAnimInstance> AnimClass,const FTransform& Transform)
{
    IC_TRACE_FUNCTION();
    if(!Component||!Mesh){UE_LOG(LogICCore, Verbose, TEXT("[Appearance.Apply.Rejected] Component=%s Mesh=%s"),*GetNameSafe(Component),*GetNameSafe(Mesh));return false;}
    Component->SetSkeletalMesh(Mesh);
    // Presets use their mesh's authored materials, including when slot counts differ.
    Component->EmptyOverrideMaterials();
    Component->SetRelativeTransform(Transform);
    if(AnimClass)Component->SetAnimInstanceClass(AnimClass);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrepareFirstPose(Component);
    Component->SetVisibility(true);Component->SetHiddenInGame(false);
    UE_LOG(LogICCore, Verbose, TEXT("[Appearance.Applied] Component=%s Mesh=%s AnimationClass=%s"),*GetNameSafe(Component),*GetPathNameSafe(Mesh),*GetNameSafe(AnimClass.Get()));
    return true;
}
