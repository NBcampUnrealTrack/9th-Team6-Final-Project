#include "PlayerInventoryPreview.h"
#include "PlayerAppearance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"

APlayerInventoryPreview::APlayerInventoryPreview()
{
    PrimaryActorTick.bCanEverTick=false;
    SetActorEnableCollision(false);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("StudioRoot"));
    PortraitMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PortraitMesh"));
    PortraitMesh->SetupAttachment(RootComponent);
    PortraitMesh->SetRelativeRotation(FRotator(0,-90,0));
    PortraitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PortraitMesh->SetCastShadow(false);
    Capture=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCamera"));
    Capture->SetupAttachment(RootComponent);
    Capture->SetRelativeLocation(FVector(350,0,90));
    Capture->SetRelativeRotation(FRotator(0,180,0));
    Capture->FOVAngle=35.f;
    Capture->CaptureSource=SCS_FinalColorLDR;
    Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->bCaptureEveryFrame=false;
    Capture->bCaptureOnMovement=false;
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetFog(false);
    Light=CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(180,-100,210));
    Light->SetIntensity(12000.f);
    Light->SetAttenuationRadius(650.f);
    Light->SetCastShadows(false);
}

void APlayerInventoryPreview::UpdatePreset(int32 Preset)
{
    if(!Target)
    {
        Target=NewObject<UTextureRenderTarget2D>(this);
        Target->ClearColor=FLinearColor(0.025f,0.025f,0.025f,1.f);
        Target->InitCustomFormat(512,640,PF_FloatRGB,true);
        Capture->TextureTarget=Target;
        Capture->ShowOnlyComponent(PortraitMesh);
    }
    if(CurrentPreset!=Preset)
    {
        if(PlayerAppearance::Apply(PortraitMesh,Preset,0.f)) CurrentPreset=Preset;
    }
    Capture->CaptureScene();
}
