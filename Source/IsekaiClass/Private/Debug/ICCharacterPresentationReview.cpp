#include "Debug/ICCharacterSetupLibrary.h"
#if WITH_EDITOR
#include "PreviewScene.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Blueprint.h"
#include "SkeletalRenderPublic.h"
#include "StaticMeshResources.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "FileHelpers.h"
#include "Combat/Actors/ICWeaponActor.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#include "Framework/Application/SlateApplication.h"
#endif

FString UICCharacterSetupLibrary::InspectCharacterPresentation(bool bApplyCalibration)
{
#if WITH_EDITOR
    if(!FSlateApplication::IsInitialized())return FString();
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("CharacterReview"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    TArray<UPackage*> Packages;
    auto Dirty=[&](UObject* Object){Object->MarkPackageDirty();Packages.AddUnique(Object->GetOutermost());};
    if(bApplyCalibration)
    {
        for(const FString Name:{TEXT("ArmingSword"),TEXT("Bow2"),TEXT("BoneSpear2"),TEXT("AncietStaff")})
        {
            auto* BP=LoadObject<UBlueprint>(nullptr,*(TEXT("/Game/IsekaiClass/Items/Weapons/Blueprints/BP_WPN_")+Name));
            auto* Gear=BP?Cast<AICWeaponActor>(BP->GeneratedClass->GetDefaultObject()):nullptr;
            UStaticMesh* Static=Gear?Gear->Visual->GetStaticMesh():nullptr;
            if(!Static)continue;
            FAssetCompilingManager::Get().FinishAllCompilation();
            const auto& Positions=Static->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
            FVector Centre=FVector::ZeroVector;
            for(uint32 V=0;V<Positions.GetNumVertices();++V)Centre+=FVector(Positions.VertexPosition(V));
            Centre/=FMath::Max(1u,Positions.GetNumVertices());
            FVector Axis=Gear->LengthAxis;
            for(int32 Iteration=0;Iteration<24;++Iteration)
            {
                FVector Next=FVector::ZeroVector;
                for(uint32 V=0;V<Positions.GetNumVertices();++V)
                {const FVector Point=FVector(Positions.VertexPosition(V))-Centre;Next+=Point*FVector::DotProduct(Point,Axis);}
                Axis=Next.GetSafeNormal();
            }
            if(FVector::DotProduct(Axis,Gear->LengthAxis)<0)Axis=-Axis;
            float Min=BIG_NUMBER,Max=-BIG_NUMBER;
            for(uint32 V=0;V<Positions.GetNumVertices();++V)
            {const float D=FVector::DotProduct(FVector(Positions.VertexPosition(V))-Centre,Axis);Min=FMath::Min(Min,D);Max=FMath::Max(Max,D);}
            const bool Bow=Name==TEXT("Bow2"),Sword=Name==TEXT("ArmingSword");
            const float Fraction=Bow?.5f:Sword?.12f:.35f;
            const float DesiredLength=Bow?120.f:Sword?95.f:Name==TEXT("BoneSpear2")?170.f:150.f;
            const float Scale=DesiredLength/(Max-Min);
            const FQuat Rotation=FQuat::FindBetweenNormals(Axis,FVector::UpVector);
            Gear->Modify();Gear->Visual->Modify();Gear->LengthAxis=Axis;Gear->HandleFraction=Fraction;
            Gear->Visual->SetRelativeScale3D(FVector(Scale));Gear->Visual->SetRelativeRotation(Rotation);
            Gear->Visual->SetRelativeLocation(-Rotation.RotateVector((Centre+Axis*FMath::Lerp(Min,Max,Fraction))*Scale));
            if(Bow)Gear->Muzzle->SetRelativeLocation(FVector::ZeroVector);
            FKismetEditorUtilities::CompileBlueprint(BP);Dirty(BP);
        }
    }
    FPreviewScene Scene(FPreviewScene::ConstructionValues().SetLightBrightness(4).SetSkyBrightness(1.5f));
    auto* World=Scene.GetWorld();
    auto* BedType=LoadClass<AActor>(nullptr,TEXT("/Game/IsekaiClass/World/SampleLevel/DreamscapeSeries/DreamscapeFarm/Meshes/Props/Interior/Cloth/BP_Farm_Bed.BP_Farm_Bed_C"));
    auto* Bed=BedType?World->SpawnActor<AActor>(BedType,FVector::ZeroVector,FRotator(0,-90,0)):nullptr;
    if(Bed)Bed->SetActorHiddenInGame(true);
    auto* Floor=World->SpawnActor<AStaticMeshActor>();
    Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetActorScale3D(FVector(8,8,.05));Floor->SetActorLocation(FVector(0,0,-2.5));
    auto* Actor=World->SpawnActor<AActor>();auto* Mesh=NewObject<USkeletalMeshComponent>(Actor);Actor->SetRootComponent(Mesh);Mesh->RegisterComponent();
    Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);Mesh->SetWorldRotation(FRotator(0,-90,0));
    auto* Capture=NewObject<USceneCaptureComponent2D>();Scene.AddComponent(Capture,FTransform::Identity);
    auto* Target=NewObject<UTextureRenderTarget2D>();Target->InitCustomFormat(768,768,PF_B8G8R8A8,false);Target->UpdateResourceImmediate(true);
    Capture->TextureTarget=Target;Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
    Capture->CaptureSource=SCS_FinalColorLDR;Capture->ProjectionType=ECameraProjectionMode::Orthographic;Capture->OrthoWidth=270;
    Capture->ShowFlags.SetMotionBlur(false);Capture->ShowFlags.SetTemporalAA(false);
    Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=1;
    FString CSV=TEXT("Character,Family,Pose,Time,FootL_Z,FootR_Z,Hips_X,Hips_Y,Hips_Z,Head_X,Head_Y,Head_Z,Hand_X,Hand_Y,Hand_Z,MeshMinZ,GroundOffset,FinitePhysics\n");
    auto SaveImage=[&](const FString& Name,FVector Focus,FVector Camera,float Width)
    {
        Capture->OrthoWidth=Width;Capture->SetWorldLocation(Camera);Capture->SetWorldRotation((Focus-Camera).Rotation());
        Scene.UpdateCaptureContents();Capture->CaptureScene();FlushRenderingCommands();TArray<FColor> Pixels;FReadSurfaceDataFlags Flags;Flags.SetLinearToGamma(false);
        if(Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))
        {for(auto& Pixel:Pixels)Pixel.A=255;TArray64<uint8> Bytes;FImageUtils::PNGCompressImageArray(768,768,Pixels,Bytes);FFileHelper::SaveArrayToFile(Bytes,*(Folder/(Name+TEXT(".png"))));}
    };
    for(int32 Index=0;Index<13;++Index)
    {
        const bool Player=Index<2;const FString ID=Player?(Index==0?TEXT("PLAYER_M"):TEXT("PLAYER_F")):FString::Printf(TEXT("NPC_R01_%03d"),Index-1);
        const FString FolderPath=Player?FString::Printf(TEXT("/Game/IsekaiClass/Animation/Player/Main%s"),Index==0?TEXT("Male"):TEXT("Female")):TEXT("/Game/IsekaiClass/Animation/NPC/")+ID;
        const FString MeshPath=Player?FString::Printf(TEXT("/Game/IsekaiClass/Player/SamplePlayer/%s/SK_Player_%s"),Index==0?TEXT("Male"):TEXT("Female"),Index==0?TEXT("M"):TEXT("F")):FString::Printf(TEXT("/Game/IsekaiClass/NPC/SampleNPC/R01/%s/SK_R01_%d"),*ID,Index-1);
        auto* Profile=LoadObject<UICCharacterAnimationProfile>(nullptr,*(FolderPath+TEXT("/DA_ANIM_")+ID));
        auto* Asset=LoadObject<USkeletalMesh>(nullptr,*MeshPath);if(!Profile||!Asset)continue;
        if(bApplyCalibration){PrepareCharacterMesh(Asset);Dirty(Asset);}
        Mesh->SetSkeletalMesh(Asset);Mesh->SetWorldLocation(FVector::ZeroVector);Mesh->SetWorldRotation(FRotator(0,-90,0));
        for(auto& Pair:Profile->WeaponSets)
        {
            const FString Family=StaticEnum<EICWeaponFamily>()->GetNameStringByValue(int64(Pair.Key));auto& Set=Pair.Value;
            const FString Weapon=Family==TEXT("Sword")?TEXT("ArmingSword"):Family==TEXT("Bow")?TEXT("Bow2"):Family==TEXT("Staff")?TEXT("BoneSpear2"):TEXT("AncietStaff");
            auto* Type=LoadClass<AICWeaponActor>(nullptr,*(TEXT("/Game/IsekaiClass/Items/Weapons/Blueprints/BP_WPN_")+Weapon+TEXT(".BP_WPN_")+Weapon+TEXT("_C")));
            auto* Gear=Type?World->SpawnActor<AICWeaponActor>(Type):nullptr;
            if(Gear)Gear->AttachToComponent(Mesh,FAttachmentTransformRules::SnapToTargetNotIncludingScale,Family==TEXT("Bow")?TEXT("weapon_l"):TEXT("weapon_r"));
            const FString Names[]={TEXT("idle"),TEXT("walk"),TEXT("run"),TEXT("attack"),TEXT("skill"),TEXT("hit"),TEXT("sleep")};
            for(int32 Pose=0;Pose<7;++Pose)
            {
                UAnimSequence* Sequence=nullptr;
                if(Pose<3&&Set.Locomotion)Sequence=Cast<UAnimSequence>(Set.Locomotion->GetBlendSample(Pose).Animation);
                else if(Pose==6)Sequence=Profile->Sleep;
                else {auto* Montage=Pose==3?Set.Attack.Get():Pose==4?Set.Skill.Get():Set.Hit.Get();if(Montage&&!Montage->SlotAnimTracks.IsEmpty()&&!Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty())Sequence=Cast<UAnimSequence>(Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference());}
                if(!Sequence)continue;
                if(bApplyCalibration)
                {
                    Sequence->bForceRootLock=true;Sequence->RootMotionRootLock=ERootMotionRootLock::RefPose;
                    Sequence->PostEditChange();Dirty(Sequence);
                    if(Pose==0&&Set.Jump){Set.Jump->bForceRootLock=true;Set.Jump->RootMotionRootLock=ERootMotionRootLock::RefPose;Set.Jump->PostEditChange();Dirty(Set.Jump);}
                }
                Mesh->SetWorldLocation(FVector::ZeroVector);Mesh->SetWorldRotation(FRotator(0,-90,0));
                Mesh->PlayAnimation(Sequence,true);Mesh->SetPosition(Sequence->GetPlayLength()*.35f,false);
                Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();Mesh->UpdateComponentToWorld();
                auto Bone=[&](const TCHAR* Name){return Mesh->GetSocketLocation(FName(Name));};
                const auto L=Bone(TEXT("J_Bip_L_Foot")),R=Bone(TEXT("J_Bip_R_Foot")),Hips=Bone(TEXT("J_Bip_C_Hips")),Head=Bone(TEXT("J_Bip_C_Head")),Hand=Bone(Family==TEXT("Bow")?TEXT("J_Bip_L_Hand"):TEXT("J_Bip_R_Hand"));
                TArray<FFinalSkinVertex> Vertices;Mesh->GetCPUSkinnedVertices(Vertices,0);float Minimum=BIG_NUMBER;
                for(const auto& Vertex:Vertices)Minimum=FMath::Min(Minimum,float(Mesh->GetComponentTransform().TransformPosition(FVector(Vertex.Position)).Z));
                if(Vertices.IsEmpty())Minimum=FMath::Min(L.Z,R.Z)-5;
                if(bApplyCalibration&&Pose==0){Set.GroundOffsetCm=-Minimum+.5f;Dirty(Profile);}
                if(bApplyCalibration&&Pose==6)
                {
                    Profile->SleepMeshTransform=FTransform(FRotator(0,-90,0),FVector(-84-Head.X,-36-Head.Y,112-(Hips.Z-8)-88));Dirty(Profile);
                }
                const float GroundOffset=Set.GroundOffsetCm;
                Mesh->SetWorldTransform(Pose==6?Profile->SleepMeshTransform*FTransform(FVector(0,0,88)):FTransform(FRotator(0,-90,0),FVector(0,0,GroundOffset)));
                if(Bed)Bed->SetActorHiddenInGame(Pose!=6);if(Gear)Gear->SetActorHiddenInGame(Pose==6);
                Mesh->ResetAnimInstanceDynamics(ETeleportType::ResetPhysics);
                bool Finite=true;
                // Exercise the existing VRM post process at 30/60/120 Hz without installing a second spring solver.
                for(const float Delta:{1.f/30,1.f/60,1.f/120})
                    for(int32 Frame=0;Frame<12;++Frame)
                    {
                        Mesh->TickAnimation(Delta,false);Mesh->RefreshBoneTransforms();
                        for(const auto& Transform:Mesh->GetComponentSpaceTransforms())
                            Finite&=!Transform.ContainsNaN()&&Transform.GetTranslation().SizeSquared()<FMath::Square(10000.f);
                    }
                CSV+=FString::Printf(TEXT("%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d\n"),*ID,*Family,*Names[Pose],Sequence->GetPlayLength()*.35f,L.Z,R.Z,Hips.X,Hips.Y,Hips.Z,Head.X,Head.Y,Head.Z,Hand.X,Hand.Y,Hand.Z,Minimum,GroundOffset,Finite?1:0);
                Mesh->SetPosition(Sequence->GetPlayLength()*.35f,false);Mesh->TickAnimation(0,false);
                Mesh->UpdateComponentToWorld();Mesh->RefreshBoneTransforms();
                FAssetCompilingManager::Get().FinishAllCompilation();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
                Mesh->UpdateBounds();FBox PoseBounds=Mesh->Bounds.GetBox();
                if(Gear&&Pose!=6){Gear->Visual->UpdateBounds();PoseBounds+=Gear->Visual->Bounds.GetBox();}
                const FVector Focus=Pose==6?FVector(0,0,85):PoseBounds.GetCenter();
                SaveImage(ID+TEXT("_")+Family+TEXT("_")+Names[Pose],Focus,Focus+FVector(300,200,Pose==6?175:50),Pose==6?330:FMath::Max(270.f,float(PoseBounds.GetExtent().Size()*2.1)));
                if(Pose==0)SaveImage(ID+TEXT("_")+Family+TEXT("_grip"),Hand+FVector(0,0,GroundOffset),Hand+FVector(170,70,30+GroundOffset),105);
            }
            if(Gear)Gear->Destroy();
        }
    }
    if(bApplyCalibration)UEditorLoadingAndSavingUtils::SavePackages(Packages,true);
    const FString Path=Folder/TEXT("PoseMeasurements.csv");FFileHelper::SaveStringToFile(CSV,*Path);Scene.RemoveComponent(Capture);return Path;
#else
    return FString();
#endif
}
