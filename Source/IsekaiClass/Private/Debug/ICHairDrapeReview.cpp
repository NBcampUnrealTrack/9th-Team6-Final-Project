#include "Debug/ICCharacterSetupLibrary.h"

#if WITH_EDITOR
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "AnimNode_KawaiiPhysics.h"
#include "VrmMetaObject.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "NPC/Character/ICNPCBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "PreviewScene.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#endif

bool UICCharacterSetupLibrary::ReviewHairDrape(const FString& Label)
{
#if WITH_EDITOR
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("HairDrapeReview")/FPaths::MakeValidFileName(Label));
    IFileManager::Get().MakeDirectory(*Folder,true);
    FPreviewScene Scene{FPreviewScene::ConstructionValues()};
    auto* Actor=Scene.GetWorld()->SpawnActor<AICNPCBase>();
    auto* Mesh=Actor->GetMesh();
    Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    const FTransform BedAnchor(FVector(0,0,88));
    auto Tick=[&](float Delta)
    {
        // The runtime presentation update used to teleport the mesh and reset
        // all springs on every frame. Exercise that same path here, including
        // repeated calls with an unchanged bed transform.
        Actor->UpdatePresentationTransform(0,&BedAnchor);
        Mesh->TickAnimation(Delta,false);Mesh->RefreshBoneTransforms();Mesh->UpdateComponentToWorld();
    };
    FString CSV=TEXT("Character,Hz,MaxTailDropCm,MeanTailDropCm,MaxLocalLengthErrorCm,Finite,Passed\n");
    bool Passed=true;
    for(const int32 Number:{1,2,3,4,5,6,7,11})
    {
        const FString ID=FString::Printf(TEXT("NPC_R01_%03d"),Number);
        auto* Asset=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/IsekaiClass/NPC/SampleNPC/R01/%s/SK_R01_%d"),*ID,Number));
        auto* Profile=LoadObject<UICCharacterAnimationProfile>(nullptr,*(TEXT("/Game/IsekaiClass/Animation/NPC/")+ID+TEXT("/DA_ANIM_")+ID));
        if(!Asset||!Profile||!Profile->Sleep){Passed=false;continue;}
        Actor->AnimationProfile=Profile;
        Mesh->SetSkeletalMesh(Asset);Mesh->PlayAnimation(Profile->Sleep,true);Mesh->SetPlayRate(0);
        Mesh->SetPosition(Profile->Sleep->GetPlayLength()*.35f,false);
        Mesh->SetWorldTransform(FTransform(FRotator(0,-90,0),FVector::ZeroVector));Tick(0);
        auto* Post=Mesh->GetPostProcessInstance();TArray<FAnimNode_KawaiiPhysics*> Nodes;
        if(Post)for(TFieldIterator<FStructProperty> Property(Post->GetClass());Property;++Property)
            if(Property->Struct==FAnimNode_KawaiiPhysics::StaticStruct())
            {auto* Node=Property->ContainerPtrToValuePtr<FAnimNode_KawaiiPhysics>(Post);if(Node->RootBone.BoneName.ToString().Contains(TEXT("Hair")))Nodes.Add(Node);}
        const auto* Meta=LoadObject<UVrmMetaObject>(nullptr,*FString::Printf(TEXT("/Game/IsekaiClass/NPC/SampleNPC/R01/%s/VM_R01_%d_VrmMeta"),*ID,Number));
        if(Nodes.IsEmpty()||!Meta){Passed=false;continue;}
        const auto& Ref=Asset->GetRefSkeleton();TArray<int32> Tips;
        for(const auto& Spring:Meta->VRM1SpringBoneMeta.Springs)
            if(Spring.joints.Num()>1&&Spring.joints[0].boneName.Contains(TEXT("Hair")))
                Tips.Add(Spring.joints.Last().boneNo);
        if(Tips.IsEmpty()){Passed=false;continue;}
        TArray<FVector> OriginalGravity;for(auto* Node:Nodes)OriginalGravity.Add(Node->Gravity);
        for(const int32 Hz:{8,30,60,120})
        {
            // Freeze the same input pose and compare actual post-process output
            // with gravity disabled/enabled. Collisions stay enabled in both.
            TArray<float> WithoutGravity;
            float LengthError=0;bool Finite=true;
            for(const bool Gravity:{false,true})
            {
                for(int32 Index=0;Index<Nodes.Num();++Index)Nodes[Index]->Gravity=Gravity?OriginalGravity[Index]:FVector::ZeroVector;
                Mesh->ResetAnimInstanceDynamics(ETeleportType::ResetPhysics);Tick(0);
                for(int32 Frame=0;Frame<Hz*4;++Frame)
                {
                    Tick(1.f/Hz);
                    const auto& Local=Mesh->GetBoneSpaceTransforms();
                    for(int32 Bone=0;Bone<Local.Num();++Bone)
                    {
                        Finite&=!Local[Bone].ContainsNaN();
                        // Body/root translations belong to the authored sleep
                        // animation; only simulated hair segments must retain
                        // their reference length under these forces.
                        if(Ref.GetBoneName(Bone).ToString().Contains(TEXT("Hair")))
                            LengthError=FMath::Max(LengthError,float(FMath::Abs(Local[Bone].GetTranslation().Size()-Ref.GetRefBonePose()[Bone].GetTranslation().Size())));
                    }
                }
                if(!Gravity)for(const int32 Bone:Tips)WithoutGravity.Add(Mesh->GetBoneTransform(Bone).GetLocation().Z);
            }
            float MaxDrop=-BIG_NUMBER,TotalDrop=0;
            for(int32 Index=0;Index<Tips.Num();++Index)
            {
                const float Drop=WithoutGravity[Index]-Mesh->GetBoneTransform(Tips[Index]).GetLocation().Z;
                MaxDrop=FMath::Max(MaxDrop,Drop);TotalDrop+=Drop;
            }
            const float MeanDrop=TotalDrop/Tips.Num();
            const bool RowPassed=Finite&&LengthError<.1f&&MaxDrop>.5f&&MeanDrop>.25f;
            Passed&=RowPassed;
            CSV+=FString::Printf(TEXT("%s,%d,%.6f,%.6f,%.6f,%d,%d\n"),*ID,Hz,MaxDrop,MeanDrop,LengthError,Finite?1:0,RowPassed?1:0);
        }
        for(int32 Index=0;Index<Nodes.Num();++Index)Nodes[Index]->Gravity=OriginalGravity[Index];Mesh->SetPlayRate(1);
    }
    FFileHelper::SaveStringToFile(CSV,*(Folder/TEXT("HairGravity.csv")));
    return Passed;
#else
    return false;
#endif
}
