#include "Debug/ICCharacterSetupLibrary.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#if WITH_EDITOR
#include "VrmMetaObject.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"
#include "PreviewScene.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"
#include "Misc/App.h"
#endif

bool UICCharacterSetupLibrary::PrepareImportedNPCMesh(USkeletalMesh* Mesh, UObject* Metadata)
{
#if WITH_EDITOR
    const auto* Meta = Cast<UVrmMetaObject>(Metadata);
    if (!Mesh || !Meta || Meta->SkeletalMesh != Mesh) return false;
    const auto& Ref = Mesh->GetRefSkeleton();
    auto Bone = [&](const FString& Key) -> FName
    {
        const FString* Name = Meta->humanoidBoneTable.Find(Key);
        return Name ? FName(**Name) : NAME_None;
    };
    const FName Hands[] = {Bone(TEXT("rightHand")), Bone(TEXT("leftHand"))};
    for (const auto Hand : Hands) if (Hand.IsNone() || Ref.FindBoneIndex(Hand) == INDEX_NONE) return false;
    TArray<FTransform> Pose;
    for (int32 Index = 0; Index < Ref.GetNum(); ++Index)
    {
        auto Transform = Ref.GetRefBonePose()[Index];
        const int32 Parent = Ref.GetParentIndex(Index);
        if (Parent != INDEX_NONE) Transform *= Pose[Parent];
        Pose.Add(Transform);
    }
    Mesh->Modify();
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FName Name = Index == 0 ? TEXT("weapon_r") : TEXT("weapon_l");
        // A reviewed authored grip must survive another setup run.
        if (Mesh->FindSocket(Name)) continue;
        auto* Socket = NewObject<USkeletalMeshSocket>(Mesh);
        Socket->SocketName = Name; Socket->BoneName = Hands[Index];
        const FString Side = Index == 0 ? TEXT("right") : TEXT("left");
        const int32 Finger = Ref.FindBoneIndex(Bone(Side + TEXT("IndexProximal")));
        const int32 Little = Ref.FindBoneIndex(Bone(Side + TEXT("LittleProximal")));
        if (Finger != INDEX_NONE && Little != INDEX_NONE)
        {
            const auto& Hand = Pose[Ref.FindBoneIndex(Hands[Index])];
            const FVector A = Hand.InverseTransformPosition(Pose[Finger].GetLocation());
            const FVector B = Hand.InverseTransformPosition(Pose[Little].GetLocation());
            Socket->RelativeLocation = (A + B) * .5f;
            Socket->RelativeRotation = FRotationMatrix::MakeFromZX((A-B).GetSafeNormal(), (A+B).GetSafeNormal()).Rotator();
        }
        Mesh->GetMeshOnlySocketList().Add(Socket);
    }
    Mesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool UICCharacterSetupLibrary::CalibrateImportedNPC(USkeletalMesh* Mesh, UICCharacterAnimationProfile* Profile)
{
#if WITH_EDITOR
    if (!Mesh || !Profile || Profile->WeaponSets.IsEmpty() || !FApp::CanEverRender()) return false;
    FPreviewScene Scene{FPreviewScene::ConstructionValues()};
    auto* World = Scene.GetWorld();
    auto* Owner = World->SpawnActor<AActor>();
    auto* Component = NewObject<USkeletalMeshComponent>(Owner);
    Owner->AddInstanceComponent(Component); Owner->SetRootComponent(Component);
    Component->SetSkeletalMesh(Mesh); Component->SetDisablePostProcessBlueprint(true);
    Component->RegisterComponent(); Component->SetWorldRotation(FRotator(0,-90,0));
    bool Valid = true;
    for (auto& Pair : Profile->WeaponSets)
    {
        auto& Set = Pair.Value;
        auto* Idle = Set.Locomotion && Set.Locomotion->GetNumberOfBlendSamples() > 0
            ? Cast<UAnimSequence>(Set.Locomotion->GetBlendSample(0).Animation) : nullptr;
        if (!Idle || Idle->GetSkeleton() != Mesh->GetSkeleton()) { Valid = false; break; }
        Component->PlayAnimation(Idle, true); Component->SetPosition(Idle->GetPlayLength()*.35f, false);
        Component->TickAnimation(0, false); Component->RefreshBoneTransforms(); Component->UpdateComponentToWorld();
        if (!Component->GetMeshObject()) { Valid = false; break; }
        TArray<FFinalSkinVertex> Vertices; Component->GetCPUSkinnedVertices(Vertices, 0);
        if (Vertices.IsEmpty()) { Valid = false; break; }
        double Minimum = BIG_NUMBER;
        for (const auto& Vertex : Vertices) Minimum = FMath::Min(Minimum, double(Component->GetComponentTransform().TransformPosition(FVector(Vertex.Position)).Z));
        if (!FMath::IsFinite(Minimum) || FMath::Abs(Minimum) > 300) { Valid = false; break; }
        Set.GroundOffsetCm = -Minimum + .5f;
    }
    Component->UnregisterComponent(); Owner->Destroy();
    if (Valid) Profile->MarkPackageDirty();
    return Valid;
#else
    return false;
#endif
}
