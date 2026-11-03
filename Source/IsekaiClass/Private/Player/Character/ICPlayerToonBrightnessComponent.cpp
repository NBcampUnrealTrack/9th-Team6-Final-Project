#include "Player/Character/ICPlayerToonBrightnessComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
const FName BrightnessParameter(TEXT("ICCharacterBrightness"));
}

UICPlayerToonBrightnessComponent::UICPlayerToonBrightnessComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickInterval=.1f;
}

void UICPlayerToonBrightnessComponent::RefreshLights()
{
    SunLights.Reset();SkyLight.Reset();LightCacheRemaining=2.f;
    if(!GetWorld())return;
    for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)
        if(auto* Light=Cast<UDirectionalLightComponent>(It->GetLightComponent()))SunLights.Add(Light);
    for(TActorIterator<ASkyLight> It(GetWorld());It;++It)
        if(auto* Light=It->GetLightComponent()){SkyLight=Light;break;}
}

float UICPlayerToonBrightnessComponent::DesiredBrightness()
{
    if(LightCacheRemaining<=0.f)RefreshLights();
    const auto* Owner=GetOwner();auto* World=GetWorld();
    // Preview worlds without environment lights keep the original material output.
    if(!Owner||!World||(SunLights.IsEmpty()&&!SkyLight.IsValid()))return 1.f;
    const auto* Sky=SkyLight.Get();
    const float SkyIntensity=Sky&&Sky->IsVisible()&&Sky->bAffectsWorld?FMath::Max(0.f,Sky->Intensity):0.f;
    float VisibleSunIntensity=0.f;
    const FVector Start=Owner->GetActorLocation()+FVector(0,0,40);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ICPlayerToonBrightness),true,Owner);
    for(const auto& Binding:SunLights)if(const auto* Light=Binding.Get())
    {
        if(!Light->IsVisible()||!Light->bAffectsWorld||Light->Intensity<=0.f)continue;
        const FVector End=Start-Light->GetForwardVector()*50000.f;
        if(!World->LineTraceTestByChannel(Start,End,ECC_Visibility,Query))VisibleSunIntensity+=Light->Intensity;
    }
    // Match comparison method 7. This estimates light; it does not sample Lumen irradiance.
    return FMath::Clamp(MinimumBrightness+SkyContribution*FMath::Sqrt(SkyIntensity)
        +SunContribution*FMath::Sqrt(VisibleSunIntensity/FMath::Max(.01f,ReferenceSunIntensity)),
        MinimumBrightness,FMath::Max(MinimumBrightness,MaximumBrightness));
}

void UICPlayerToonBrightnessComponent::RefreshMaterials()
{
    auto* Player=Cast<AICPlayerCharacter>(GetOwner());
    auto* Body=Player?Player->GetMesh():nullptr;
    auto* Mesh=Body?Body->GetSkeletalMeshAsset():nullptr;
    CachedMesh=Mesh;Materials.Reset();CurrentBrightness=DesiredBrightness();
    if(!Mesh)return;
    // A female preset has more slots than the male preset. Discard its old MIDs.
    Body->EmptyOverrideMaterials();
    const auto& Slots=Mesh->GetMaterials();Materials.SetNum(Slots.Num());
    for(int32 Index=0;Index<Slots.Num();++Index)
    {
        auto* Source=Slots[Index].MaterialInterface.Get();float Value=0.f;
        // Original and unrelated material types remain untouched.
        if(!Source||!Source->GetScalarParameterValue(FHashedMaterialParameterInfo(FMaterialParameterInfo(BrightnessParameter)),Value))continue;
        auto* Dynamic=Body->CreateDynamicMaterialInstance(Index,Source);
        if(Dynamic){Materials[Index]=Dynamic;Dynamic->SetScalarParameterValue(BrightnessParameter,CurrentBrightness);}
    }
}

void UICPlayerToonBrightnessComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime,TickType,TickFunction);
    LightCacheRemaining-=DeltaTime;
    const auto* Player=Cast<AICPlayerCharacter>(GetOwner());
    if(!Player||!Player->GetMesh())return;
    if(CachedMesh.Get()!=Player->GetMesh()->GetSkeletalMeshAsset())RefreshMaterials();
    CurrentBrightness=FMath::FInterpTo(CurrentBrightness,DesiredBrightness(),DeltaTime,FMath::Max(.1f,TransitionSpeed));
    for(const auto& Material:Materials)if(Material)Material->SetScalarParameterValue(BrightnessParameter,CurrentBrightness);
}
