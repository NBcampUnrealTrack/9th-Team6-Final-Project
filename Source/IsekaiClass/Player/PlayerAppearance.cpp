#include "PlayerAppearance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#include "Misc/CommandLine.h"
#endif

bool PlayerAppearance::Apply(USkeletalMeshComponent* Component,int32 Preset,float FeetZ)
{
    const TCHAR* Path=Preset==1
        ?TEXT("/Game/IsekaiClass/Characters/Player/MainFemale/MainFemale1/SK_MainCharacter1.SK_MainCharacter1")
        :TEXT("/Game/IsekaiClass/Characters/Player/MainMale/Mainmale1/SK_MainCharacter_2.SK_MainCharacter_2");
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,Path);
    if(!Component||!Mesh) {UE_LOG(LogTemp,Error,TEXT("Player appearance could not load: %s"),Path);return false;}
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("PlayerPrototypeSmoke"))&&GShaderCompilingManager)
        GShaderCompilingManager->FinishAllCompilation();
#endif
    Component->SetSkeletalMesh(Mesh);
    Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Component->SetAnimation(nullptr);
    const auto Bounds=Mesh->GetBounds();
    const float Scale=170.f/FMath::Max(1.f,Bounds.BoxExtent.Z*2.f);
    Component->SetRelativeScale3D(FVector(Scale));
    Component->SetRelativeRotation(FRotator(0,-90,0));
    Component->SetRelativeLocation(FVector(0,0,FeetZ-(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale));
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetVisibility(true);
    Component->SetHiddenInGame(false);
    Component->TickAnimation(0.f,false);
    Component->RefreshBoneTransforms();
    return true;
}
