#include "Combat/VFX/ICCombatVFX.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Combat/Actors/ICAttackEffect.h"
#include "Engine/World.h"
#include "Misc/App.h"

const FICCombatVFXRow* ICCombatVFX::Resolve(const FICCombatPresentation& Presentation)
{
    return Presentation.Visuals.IsNull()?nullptr:Presentation.Visuals.GetRow<FICCombatVFXRow>(TEXT("Combat VFX"));
}
UNiagaraComponent* ICCombatVFX::Attach(AActor* Owner,USceneComponent* Parent,const FICNiagaraEffect& Style,
    float DefaultDuration,float Radius,float Angle,float Floor,float Slope)
{
    if(!Owner||!Parent||!Style.bEnabled||Owner->GetNetMode()==NM_DedicatedServer)return nullptr;
    auto* System=Style.System.LoadSynchronous();if(!System)return nullptr;
    // Imported systems can queue activation until their GPU scripts are ready.
    // Rejecting the first activation dropped their hit effects entirely.
    // The original procedural effects retain their existing mesh fallback.
    if(FApp::CanEverRender()&&!System->IsReadyToRun()&&!Style.bUseComponentScale)return nullptr;
    auto* Component=NewObject<UNiagaraComponent>(Owner);
    Owner->AddInstanceComponent(Component);Component->SetAutoActivate(false);Component->SetAutoDestroy(false);
    Component->SetupAttachment(Parent);Component->SetAsset(System);
    Component->SetRelativeLocationAndRotation(Style.Offset,Style.Rotation);
    const float Duration=Style.Duration>0?Style.Duration:DefaultDuration;
    const float SimulationDuration=Style.ReferenceDuration>0&&Duration>0?Style.ReferenceDuration:Duration;
    if(Style.ReferenceDuration>0&&Duration>0)Component->SetCustomTimeDilation(Style.ReferenceDuration/Duration);
    Component->SetVariableLinearColor(TEXT("User.IC_Color"),Style.Color*FMath::Max(0.f,Style.Intensity));
    Component->SetVariableVec3(TEXT("User.IC_Scale"),Style.Scale.GetAbs());
    if(Style.bUseComponentScale||Style.ReferenceDuration>0)Component->SetRelativeScale3D(Style.Scale.GetAbs());
    Component->SetVariableFloat(TEXT("User.IC_Duration"),SimulationDuration>0?SimulationDuration:100000.f);
    Component->SetVariableFloat(TEXT("User.IC_Radius"),Radius);Component->SetVariableFloat(TEXT("User.IC_Angle"),Angle);
    Component->SetVariableFloat(TEXT("User.IC_Floor"),Floor);Component->SetVariableFloat(TEXT("User.IC_Slope"),Slope);
    Component->SetVariableFloat(TEXT("User.IC_Width"),FMath::Max(.01f,Style.WidthCm));
    Component->RegisterComponent();Component->Activate(true);return Component;
}
void ICCombatVFX::Spawn(UWorld* World,const FTransform& Transform,const FICNiagaraEffect& Style,float DefaultDuration,USceneComponent* Follow)
{
    if(!World||!Style.bEnabled||Style.System.IsNull()||World->GetNetMode()==NM_DedicatedServer)return;
    auto* Effect=World->SpawnActor<AICAttackEffect>(AICAttackEffect::StaticClass(),Transform);
    if(!Effect)return;
    if(Follow)Effect->AttachToComponent(Follow,FAttachmentTransformRules::KeepWorldTransform);
    Effect->UseNiagaraLifetime(Style.Duration>0?Style.Duration:DefaultDuration);
    if(!Attach(Effect,Effect->GetRootComponent(),Style,DefaultDuration))Effect->Destroy();
}
