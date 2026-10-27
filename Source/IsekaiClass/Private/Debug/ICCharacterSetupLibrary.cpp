#include "Debug/ICCharacterSetupLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace1D.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/Skeleton.h"
#include "AnimationBlueprintLibrary.h"
#include "Combat/Animation/ICCombatAnimNotify.h"
#include "Combat/Animation/ICWeaponAnimNotify.h"
#include "Misc/PackageName.h"
#include "Components/BoxComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "EngineUtils.h"
#include "Core/Controller/ICPlayerController.h"
#include "Save/ICSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Debug/ICMonsterTestLayout.h"
#include "UnrealClient.h"
#include "Editor.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#endif

bool UICCharacterSetupLibrary::CaptureMonsterTestHUD(UWorld* World,const FString& File)
{
#if WITH_EDITOR
    auto* Viewport=World&&World->WorldType==EWorldType::PIE?World->GetGameViewport():nullptr;
    auto Window=Viewport?Viewport->GetWindow():nullptr;
    if(!Window.IsValid()||!FSlateApplication::IsInitialized())return false;
    TArray<FColor> Pixels;FIntVector Size;
    if(!FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(),Pixels,Size))return false;
    for(auto& Pixel:Pixels)Pixel.A=255;
    TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
    return FFileHelper::SaveArrayToFile(PNG,*File);
#else
    return false;
#endif
}

bool UICCharacterSetupLibrary::PressUIReviewKey(UWorld* World,FKey Key)
{
#if WITH_EDITOR
    auto* Viewport=World&&World->WorldType==EWorldType::PIE?World->GetGameViewport():nullptr;
    auto Window=Viewport?Viewport->GetWindow():nullptr;
    if(!Window.IsValid()||!FSlateApplication::IsInitialized())return false;
    auto& App=FSlateApplication::Get();
    App.ProcessWindowActivatedEvent(FWindowActivateEvent(FWindowActivateEvent::EA_Activate,Window.ToSharedRef()));
    const bool Handled=App.ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
    App.ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));return Handled;
#else
    return false;
#endif
}

bool UICCharacterSetupLibrary::BeginMonsterTestReview()
{
#if WITH_EDITOR
    if(!GEditor||GEditor->PlayWorld)return false;
    auto* Settings=DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(),GetTransientPackage());
    Settings->NewWindowWidth=1600;Settings->NewWindowHeight=900;
    FRequestPlaySessionParams Session;
    Session.WorldType=EPlaySessionWorldType::PlayInEditor;
    Session.EditorPlaySettings=Settings;
    GEditor->RequestPlaySession(Session);return true;
#else
    return false;
#endif
}

bool UICCharacterSetupLibrary::PrepareLiveCharacterReview(UWorld* World)
{
#if WITH_EDITOR
    if(!World||World->WorldType!=EWorldType::PIE)return false;
    auto* PC=Cast<AICPlayerController>(UGameplayStatics::GetPlayerController(World,0));
    if(!PC||PC->bCreated||PC->Menu!=TEXT("Create")||!PC->GetSaves())return false;
    if(!PC->GetSaves()->ConfigureServices(PC->GetWorldState(),PC->GetTime(),TEXT("IC_Visual_")+FGuid::NewGuid().ToString(EGuidFormats::Digits)))return false;
    PC->PlayerName=TEXT("VisualReview");
    return PC->DebugCreatePlayer();
#else
    return false;
#endif
}

void UICCharacterSetupLibrary::CaptureLiveCharacterReview(UWorld* World,const FString& Name)
{
#if WITH_EDITOR
    if(!World||World->WorldType!=EWorldType::PIE)return;
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("LiveCharacterReview"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    FScreenshotRequest::RequestScreenshot(Folder/(FPaths::MakeValidFileName(Name)+TEXT(".png")),false,false);
#endif
}

bool UICCharacterSetupLibrary::PrepareTestNavigation(UWorld* World)
{
#if WITH_EDITOR
    if(!World||!World->GetPackage()->GetName().EndsWith(TEXT("/L_TestMap")))return false;
    ANavMeshBoundsVolume* Volume=nullptr;
    for(TActorIterator<ANavMeshBoundsVolume> It(World);It;++It)if(It->ActorHasTag(TEXT("IC.NPCTestNavigation"))){Volume=*It;break;}
    if(!Volume)
    {
        Volume=World->SpawnActor<ANavMeshBoundsVolume>();
        if(!Volume)return false;
        Volume->SetActorLabel(TEXT("IC_NPC_Test_Navigation"));Volume->Tags.Add(TEXT("IC.NPCTestNavigation"));
        Volume->SetActorLocation(FVector(1500,0,200));
    }
    // NavigationSystem uses GetComponentsBoundingBox(true) for bounds volumes.
    // A serialized box also works in the command-line editor without a BSP rebuild.
    auto* Bounds=Volume->FindComponentByClass<UBoxComponent>();
    if(!Bounds)
    {
        Bounds=NewObject<UBoxComponent>(Volume,TEXT("TestNavigationBounds"),RF_Transactional);
        Volume->AddInstanceComponent(Bounds);Bounds->SetupAttachment(Volume->GetRootComponent());
        Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bounds->SetCanEverAffectNavigation(false);
        Bounds->SetBoxExtent(FVector(4000,4000,500));Bounds->RegisterComponent();
    }
    if(TActorIterator<AICMonsterTestLayout>(World))
    {
        Volume->SetActorLocation(FVector(1500,5000,200));
        Bounds->SetBoxExtent(FVector(6000,10000,500));
    }
    Bounds->UpdateBounds();Volume->PostEditChange();
    if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        // The command-line editor keeps this lock until its first editor tick.
        // Finish loading before explicitly building this test map's navigation.
        FlushAsyncLoading();
        Nav->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock,UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
        Nav->OnNavigationBoundsUpdated(Volume);Nav->Build();
        for(TActorIterator<ANavigationData> It(World);It;++It)It->EnsureBuildCompletion();
    }
    World->MarkPackageDirty();return true;
#else
    return false;
#endif
}

TArray<FName> UICCharacterSetupLibrary::GetCharacterBones(USkeletalMesh* Mesh)
{
    TArray<FName> Names;
    if(Mesh)for(const auto& Bone:Mesh->GetRefSkeleton().GetRefBoneInfo())Names.Add(Bone.Name);
    return Names;
}
bool UICCharacterSetupLibrary::PrepareCharacterMesh(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
    if(!Mesh)return false;
    const FName Bones[]={TEXT("J_Bip_R_Hand"),TEXT("J_Bip_L_Hand")};
    const FName Names[]={TEXT("weapon_r"),TEXT("weapon_l")};
    const auto& Ref=Mesh->GetRefSkeleton();TArray<FTransform> Reference;
    for(int32 Index=0;Index<Ref.GetNum();++Index)
    {auto Transform=Ref.GetRefBonePose()[Index];const int32 Parent=Ref.GetParentIndex(Index);if(Parent!=INDEX_NONE)Transform*=Reference[Parent];Reference.Add(Transform);}
    for(int32 Index=0;Index<2;++Index)
    {
        const int32 HandIndex=Ref.FindBoneIndex(Bones[Index]);if(HandIndex==INDEX_NONE)return false;
        auto* Socket=Mesh->FindSocket(Names[Index]);
        if(!Socket)
        {
            Socket=NewObject<USkeletalMeshSocket>(Mesh);Socket->SocketName=Names[Index];Socket->BoneName=Bones[Index];
            Mesh->GetMeshOnlySocketList().Add(Socket);
        }
        const int32 Finger=Ref.FindBoneIndex(Index==0?TEXT("J_Bip_R_Index1"):TEXT("J_Bip_L_Index1"));
        const int32 Little=Ref.FindBoneIndex(Index==0?TEXT("J_Bip_R_Little1"):TEXT("J_Bip_L_Little1"));
        if(Finger!=INDEX_NONE&&Little!=INDEX_NONE)
        {
            const auto& Hand=Reference[HandIndex];
            const FVector A=Hand.InverseTransformPosition(Reference[Finger].GetLocation());
            const FVector B=Hand.InverseTransformPosition(Reference[Little].GetLocation());
            // Canonical weapon +Z runs across the closed grip, toward the index finger.
            // Put the grip in the palm rather than at the wrist bone origin.
            Socket->RelativeLocation=(A+B)*.5f;
            Socket->RelativeRotation=FRotationMatrix::MakeFromZX((A-B).GetSafeNormal(),(A+B).GetSafeNormal()).Rotator();
        }
    }
    Mesh->MarkPackageDirty();return true;
#else
    return false;
#endif
}

UBlendSpace1D* UICCharacterSetupLibrary::CreateLocomotion(const FString& Path, UAnimSequence* Idle, UAnimSequence* Walk, UAnimSequence* Run)
{
#if WITH_EDITOR
    if (!Path.StartsWith(TEXT("/Game/IsekaiClass/Animation/")) || !Idle || !Walk || !Run
        || Idle->GetSkeleton()!=Walk->GetSkeleton() || Idle->GetSkeleton()!=Run->GetSkeleton()) return nullptr;
    if (auto* Existing=LoadObject<UBlendSpace1D>(nullptr,*Path)) return Existing;
    auto* Asset=NewObject<UBlendSpace1D>(CreatePackage(*Path),*FPackageName::GetLongPackageAssetName(Path),RF_Public|RF_Standalone);
    Asset->SetSkeleton(Idle->GetSkeleton());
    auto& Parameter=const_cast<FBlendParameter&>(Asset->GetBlendParameter(0));
    Parameter.DisplayName=TEXT("Speed");Parameter.Min=0;Parameter.Max=600;
    Asset->AddSample(Idle,FVector(0,0,0));Asset->AddSample(Walk,FVector(180,0,0));Asset->AddSample(Run,FVector(600,0,0));
    Asset->ValidateSampleData();Asset->ResampleData();Asset->PostEditChange();
    FAssetRegistryModule::AssetCreated(Asset);Asset->MarkPackageDirty();return Asset;
#else
    return nullptr;
#endif
}
UBlendSpace* UICCharacterSetupLibrary::CreateCombatLocomotion(const FString& Path,UAnimSequence* Idle,const TArray<UAnimSequence*>& Directions)
{
#if WITH_EDITOR
    if(!Path.StartsWith(TEXT("/Game/IsekaiClass/Animation/"))||!Idle||Directions.Num()!=8)return nullptr;
    for(auto* Clip:Directions)if(!Clip||Clip->GetSkeleton()!=Idle->GetSkeleton())return nullptr;
    if(auto* Existing=LoadObject<UBlendSpace>(nullptr,*Path))return Existing;
    auto* Asset=NewObject<UBlendSpace>(CreatePackage(*Path),*FPackageName::GetLongPackageAssetName(Path),RF_Public|RF_Standalone);
    Asset->SetSkeleton(Idle->GetSkeleton());
    for(int32 Axis=0;Axis<2;++Axis)
    {
        auto& Parameter=const_cast<FBlendParameter&>(Asset->GetBlendParameter(Axis));
        Parameter.DisplayName=Axis==0?TEXT("ForwardSpeed"):TEXT("RightSpeed");
        Parameter.Min=-450;Parameter.Max=450;
    }
    Asset->AddSample(Idle,FVector::ZeroVector);
    for(int32 Index=0;Index<8;++Index)
    {
        const float Angle=Index*PI/4.f;
        Asset->AddSample(Directions[Index],FVector(FMath::Cos(Angle)*450,FMath::Sin(Angle)*450,0));
    }
    Asset->ValidateSampleData();Asset->ResampleData();Asset->PostEditChange();
    FAssetRegistryModule::AssetCreated(Asset);Asset->MarkPackageDirty();return Asset;
#else
    return nullptr;
#endif
}

UAnimMontage* UICCharacterSetupLibrary::CreateActionMontage(const FString& Path,UAnimSequence* Sequence,bool bCombatNotify)
{
#if WITH_EDITOR
    if (!Path.StartsWith(TEXT("/Game/IsekaiClass/Animation/")) || !Sequence) return nullptr;
    auto* Asset=LoadObject<UAnimMontage>(nullptr,*Path);const bool Existing=Asset!=nullptr;
    if(!Asset)Asset=NewObject<UAnimMontage>(CreatePackage(*Path),*FPackageName::GetLongPackageAssetName(Path),RF_Public|RF_Standalone);
    Asset->Modify();Asset->SlotAnimTracks.Reset();
    Asset->SetSkeleton(Sequence->GetSkeleton());
    Sequence->GetSkeleton()->RegisterSlotNode(TEXT("DefaultSlot"));
    Asset->SlotAnimTracks.SetNum(1);Asset->SlotAnimTracks[0].SlotName=TEXT("DefaultSlot");
    FAnimSegment Segment;Segment.SetAnimReference(Sequence,true);Segment.StartPos=0;Segment.AnimStartTime=0;
    Segment.AnimEndTime=Sequence->GetPlayLength();Segment.AnimPlayRate=1;Segment.LoopingCount=1;
    Asset->SlotAnimTracks[0].AnimTrack.AnimSegments.Add(Segment);Asset->SetCompositeLength(Sequence->GetPlayLength());
    if(Asset->GetSectionIndex(TEXT("Default"))==INDEX_NONE)Asset->AddAnimCompositeSection(TEXT("Default"),0);
    if(bCombatNotify&&!Existing)
    {
        UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Asset,TEXT("ICCombat"));
        auto* Notify=Cast<UICCombatAnimNotify>(UAnimationBlueprintLibrary::AddAnimationNotifyEvent(Asset,TEXT("ICCombat"),FMath::Min(.15f,Sequence->GetPlayLength()*.35f),UICCombatAnimNotify::StaticClass()));
        if(Notify)Notify->Event=TEXT("AN_EffectCommit");
    }
    Asset->PostEditChange();if(!Existing)FAssetRegistryModule::AssetCreated(Asset);Asset->MarkPackageDirty();return Asset;
#else
    return nullptr;
#endif
}

UAnimMontage* UICCharacterSetupLibrary::CreateNormalAttackMontage(const FString& Path,UAnimSequence* Sequence,float HitTime,bool bProjectile)
{
#if WITH_EDITOR
    auto* Montage=CreateActionMontage(Path,Sequence,false);if(!Montage)return nullptr;
    const FName TrackName=TEXT("ICNormalAttack");
    if(Montage->AnimNotifyTracks.ContainsByPredicate([&](const FAnimNotifyTrack& Track){return Track.TrackName==TrackName;}))
        UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(Montage,TrackName);
    UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Montage,TrackName);
    auto* Notify=Cast<UICCombatAnimNotify>(UAnimationBlueprintLibrary::AddAnimationNotifyEvent(Montage,TrackName,FMath::Clamp(HitTime,0.f,Sequence->GetPlayLength()-.01f),UICCombatAnimNotify::StaticClass()));
    if(!Notify)return nullptr;Notify->Event=bProjectile?TEXT("AN_ProjectileFire"):TEXT("AN_HitStart");Notify->HitIndex=0;
    Montage->BlendIn.SetBlendTime(.06f);Montage->BlendOut.SetBlendTime(.08f);
    Montage->PostEditChange();Montage->MarkPackageDirty();return Montage;
#else
    return nullptr;
#endif
}
UAnimMontage* UICCharacterSetupLibrary::CreateWeaponTransitionMontage(const FString& Path,UAnimSequence* Sequence,bool bDraw,float VisibilityFraction)
{
#if WITH_EDITOR
    auto* Montage=CreateActionMontage(Path,Sequence,false);
    if(!Montage)return nullptr;
    const FName TrackName=TEXT("ICWeapon");
    if(Montage->AnimNotifyTracks.ContainsByPredicate([&](const FAnimNotifyTrack& Track){return Track.TrackName==TrackName;}))
        UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(Montage,TrackName);
    UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Montage,TrackName);
    auto* Notify=Cast<UICWeaponAnimNotify>(UAnimationBlueprintLibrary::AddAnimationNotifyEvent(
        Montage,TrackName,Sequence->GetPlayLength()*FMath::Clamp(VisibilityFraction,.05f,.95f),UICWeaponAnimNotify::StaticClass()));
    if(!Notify)return nullptr;
    Notify->bDraw=bDraw;
    Montage->BlendIn.SetBlendTime(.12f);Montage->BlendOut.SetBlendTime(.12f);
    Montage->PostEditChange();Montage->MarkPackageDirty();
    return Montage;
#else
    return nullptr;
#endif
}
