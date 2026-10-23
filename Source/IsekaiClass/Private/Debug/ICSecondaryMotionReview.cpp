#include "Debug/ICCharacterSetupLibrary.h"

#if WITH_EDITOR
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PreviewScene.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#endif

bool UICCharacterSetupLibrary::ReviewSecondaryMotion(const FString& Label, bool bCapture)
{
#if WITH_EDITOR
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SecondaryMotionReview")/FPaths::MakeValidFileName(Label));
    IFileManager::Get().MakeDirectory(*Folder,true);
    FPreviewScene Scene(FPreviewScene::ConstructionValues().SetLightBrightness(4).SetSkyBrightness(1.5f));
    auto* World=Scene.GetWorld();
    auto MakeComponent=[&]()
    {
        auto* Actor=World->SpawnActor<AActor>();
        auto* Mesh=NewObject<USkeletalMeshComponent>(Actor);Actor->SetRootComponent(Mesh);Mesh->RegisterComponent();
        Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        return Mesh;
    };
    auto* Mesh=MakeComponent();
    USceneCaptureComponent2D* Capture=nullptr;UTextureRenderTarget2D* Target=nullptr;
    if(bCapture)
    {
        Capture=NewObject<USceneCaptureComponent2D>();Scene.AddComponent(Capture,FTransform::Identity);
        Target=NewObject<UTextureRenderTarget2D>();Target->InitCustomFormat(768,768,PF_B8G8R8A8,false);Target->UpdateResourceImmediate(true);
        Capture->TextureTarget=Target;Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
        Capture->CaptureSource=SCS_FinalColorLDR;Capture->ProjectionType=ECameraProjectionMode::Orthographic;Capture->OrthoWidth=225;
        Capture->ShowFlags.SetMotionBlur(false);Capture->ShowFlags.SetTemporalAA(false);
        Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
        Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=1;
    }
    auto Tick=[](USkeletalMeshComponent* Component,float Delta)
    {Component->TickAnimation(Delta,false);Component->RefreshBoneTransforms();Component->UpdateComponentToWorld();};
    FString CSV=TEXT("Character,Hz,Scenario,MaxLocalLengthErrorCm,MaxResetErrorCm,MaxResetAngleDeg,MaxFrameMotionCm,Finite\n");
    bool Passed=true;
    for(int32 Index=0;Index<13;++Index)
    {
        const bool Player=Index<2;const FString ID=Player?(Index==0?TEXT("PLAYER_M"):TEXT("PLAYER_F")):FString::Printf(TEXT("NPC_R01_%03d"),Index-1);
        const FString FolderPath=Player?FString::Printf(TEXT("/Game/IsekaiClass/Animation/Player/Main%s"),Index==0?TEXT("Male"):TEXT("Female")):TEXT("/Game/IsekaiClass/Animation/NPC/")+ID;
        const FString MeshPath=Player?FString::Printf(TEXT("/Game/IsekaiClass/Player/SamplePlayer/%s/SK_Player_%s"),Index==0?TEXT("Male"):TEXT("Female"),Index==0?TEXT("M"):TEXT("F")):FString::Printf(TEXT("/Game/IsekaiClass/NPC/SampleNPC/R01/%s/SK_R01_%d"),*ID,Index-1);
        auto* Profile=LoadObject<UICCharacterAnimationProfile>(nullptr,*(FolderPath+TEXT("/DA_ANIM_")+ID));
        auto* Asset=LoadObject<USkeletalMesh>(nullptr,*MeshPath);
        const auto* Set=Profile?Profile->FindSet(Profile->DefaultFamily):nullptr;
        if(!Asset||!Set||!Set->Locomotion){Passed=false;continue;}
        auto* Idle=Cast<UAnimSequence>(Set->Locomotion->GetBlendSample(0).Animation);
        auto* Run=Cast<UAnimSequence>(Set->Locomotion->GetBlendSample(2).Animation);
        auto* Attack=Set->Attack&&!Set->Attack->SlotAnimTracks.IsEmpty()&&!Set->Attack->SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty()
            ?Cast<UAnimSequence>(Set->Attack->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference()):nullptr;
        if(!Idle||!Run||!Attack||!Profile->Sleep){Passed=false;continue;}
        Mesh->SetSkeletalMesh(Asset);
        const auto& Ref=Asset->GetRefSkeleton();TArray<int32> Secondary;
        for(int32 Bone=0;Bone<Ref.GetNum();++Bone)
            if(Ref.GetBoneName(Bone).ToString().StartsWith(TEXT("J_Sec_"))||Ref.GetBoneName(Bone).ToString().Contains(TEXT("Hair")))Secondary.Add(Bone);
        for(const int32 Hz:{30,60,120,144})for(int32 Scenario=0;Scenario<6;++Scenario)
        {
            const FString Names[]={TEXT("Idle"),TEXT("RunTurn"),TEXT("Hitch"),TEXT("TeleportReset"),TEXT("SleepReset"),TEXT("Attack")};
            const FString Name=Names[Scenario];
            const float Delta=1.f/Hz;
            UAnimSequence* Sequence=Scenario==0?Idle:Scenario==5?Attack:Run;
            Mesh->SetWorldTransform(FTransform(FRotator(0,-90,0),FVector::ZeroVector));
            Mesh->PlayAnimation(Sequence,true);Mesh->SetPosition(0,false);
            Mesh->ResetAnimInstanceDynamics(ETeleportType::ResetPhysics);Tick(Mesh,0);
            bool Finite=true;float LengthError=0,ResetError=0,ResetAngle=0,FrameMotion=0;
            TArray<FTransform> Previous;
            for(int32 Frame=0;Frame<Hz;++Frame)
            {
                if(Scenario>0&&Scenario<4)
                    Mesh->SetWorldTransform(FTransform(FRotator(0,-90+90*Frame*Delta,0),FVector(Frame*Delta*300,0,0)));
                Tick(Mesh,Scenario==2&&Frame==Hz/2?.25f:Delta);
                const auto& Local=Mesh->GetBoneSpaceTransforms();const auto& Component=Mesh->GetComponentSpaceTransforms();
                for(const int32 Bone:Secondary)if(Local.IsValidIndex(Bone))
                {
                    LengthError=FMath::Max(LengthError,float(FMath::Abs(Local[Bone].GetTranslation().Size()-Ref.GetRefBonePose()[Bone].GetTranslation().Size())));
                    Finite&=!Local[Bone].ContainsNaN()&&!Component[Bone].ContainsNaN();
                    if(Previous.IsValidIndex(Bone))FrameMotion=FMath::Max(FrameMotion,float(FVector::Distance(Component[Bone].GetLocation(),Previous[Bone].GetLocation())));
                }
                Previous=Component;
            }
            if(Scenario==3||Scenario==4)
            {
                const FTransform Destination=Scenario==3?FTransform(FRotator(0,90,0),FVector(2300,-1700,900)):Profile->SleepMeshTransform;
                if(Scenario==4){Sequence=Profile->Sleep;Mesh->PlayAnimation(Sequence,true);}
                Mesh->SetWorldTransform(Destination);Mesh->SetPosition(Sequence->GetPlayLength()*.35f,false);
                Mesh->ResetAnimInstanceDynamics(ETeleportType::ResetPhysics);Tick(Mesh,0);
                const TArray<FTransform> Actual=Mesh->GetComponentSpaceTransforms();
                // Compare with the very same input pose and component, bypassing
                // the post process. A second component may still have a stale
                // single-node proxy immediately after SetPosition in the editor.
                Mesh->SetDisablePostProcessBlueprint(true);Tick(Mesh,0);
                const auto& Expected=Mesh->GetComponentSpaceTransforms();
                for(const int32 Bone:Secondary)if(Actual.IsValidIndex(Bone)&&Expected.IsValidIndex(Bone))
                {
                    ResetError=FMath::Max(ResetError,float(FVector::Distance(Actual[Bone].GetLocation(),Expected[Bone].GetLocation())));
                    ResetAngle=FMath::Max(ResetAngle,float(FMath::RadiansToDegrees(Actual[Bone].GetRotation().AngularDistance(Expected[Bone].GetRotation()))));
                }
                Mesh->SetDisablePostProcessBlueprint(false);
            }
            CSV+=FString::Printf(TEXT("%s,%d,%s,%.6f,%.6f,%.6f,%.6f,%d\n"),*ID,Hz,*Name,LengthError,ResetError,ResetAngle,FrameMotion,Finite?1:0);
            Passed&=Finite&&LengthError<.1f&&ResetError<.1f&&ResetAngle<.1f&&(Scenario!=0||FrameMotion<10.f);
            if(Capture&&Hz==60&&(Scenario==0||Scenario==1||Scenario==4))
            {
                FAssetCompilingManager::Get().FinishAllCompilation();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
                const FVector Focus=Mesh->GetComponentLocation()+FVector(0,0,90);
                Capture->SetWorldLocation(Focus+FVector(300,200,30));Capture->SetWorldRotation((Focus-Capture->GetComponentLocation()).Rotation());
                Scene.UpdateCaptureContents();Capture->CaptureScene();FlushRenderingCommands();
                TArray<FColor> Pixels;FReadSurfaceDataFlags Flags;Flags.SetLinearToGamma(false);
                if(Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))
                {for(auto& Pixel:Pixels)Pixel.A=255;TArray64<uint8> Bytes;FImageUtils::PNGCompressImageArray(768,768,Pixels,Bytes);FFileHelper::SaveArrayToFile(Bytes,*(Folder/(ID+TEXT("_")+Name+TEXT(".png"))));}
            }
        }
    }
    FFileHelper::SaveStringToFile(CSV,*(Folder/TEXT("Measurements.csv")));
    if(Capture)Scene.RemoveComponent(Capture);
    return Passed;
#else
    return false;
#endif
}
