#include "Combat/Actors/ICAttackEffect.h"
#include "Character/Base/ICCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Combat/VFX/ICCombatVFX.h"

AICAttackEffect::AICAttackEffect()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    GlowMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Combat/Effects/M_AttackGlow.M_AttackGlow")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    for(int32 I=0;I<26;++I)
    {
        auto* Piece=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Arc%d"),I));
        Piece->SetupAttachment(RootComponent);Piece->SetStaticMesh(Cylinder.Object);
        Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);Piece->SetCastShadow(false);Piece->SetVisibility(false);
        Pieces.Add(Piece);
    }
}
FLinearColor AICAttackEffect::FamilyColor(EICWeaponFamily Family)
{
    if(Family==EICWeaponFamily::Bow)return FLinearColor(1.f,.6f,.12f);
    if(Family==EICWeaponFamily::Staff)return FLinearColor(.12f,.4f,1.f);
    if(Family==EICWeaponFamily::Wand)return FLinearColor(.15f,1.f,.52f);
    return FLinearColor(.5f,.8f,1.f);
}
void AICAttackEffect::SetColor(FLinearColor Value)
{
    Color=Value;
    if(auto* Material=GlowMaterial.LoadSynchronous())
    {Glow=UMaterialInstanceDynamic::Create(Material,this);Glow->SetVectorParameterValue(TEXT("Color"),Color*3);for(const auto& Piece:Pieces)Piece->SetMaterial(0,Glow);}
    SetLifeSpan(Duration+.05f);
}
void AICAttackEffect::Line(int32 Index,FVector Start,FVector End,float Width)
{
    auto* Piece=Pieces[Index].Get();Piece->SetVisibility(true);
    Piece->SetRelativeLocation((Start+End)*.5);
    Piece->SetRelativeRotation(FRotationMatrix::MakeFromZ(End-Start).Rotator());
    Piece->SetRelativeScale3D(FVector(Width/50,Width/50,FVector::Distance(Start,End)/100));
}
void AICAttackEffect::SpawnSlash(AICCharacterBase* Character,double Radius,double Angle,int32 Stage,const FICCombatPresentation& Presentation)
{
    if(!Character||!Character->GetWorld()||Character->GetWorld()->GetNetMode()==NM_DedicatedServer)return;
    auto* Effect=Character->GetWorld()->SpawnActor<AICAttackEffect>(Character->GetActorLocation(),Character->GetActorRotation());
    if(!Effect)return;
    if(const auto* Style=ICCombatVFX::Resolve(Presentation))
    {
        const float Floor=-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+5;
        const bool Slash=ICCombatVFX::Attach(Effect,Effect->RootComponent,Style->Slash,.24f,Radius,Angle,Floor,Style->SlashSlope)!=nullptr;
        const bool Range=ICCombatVFX::Attach(Effect,Effect->RootComponent,Style->Range,.24f,Radius,Angle,Floor,0)!=nullptr;
        if(Slash||Range||!Style->bLegacyMeshFallback||(!Style->Slash.bEnabled&&!Style->Range.bEnabled))
        {Effect->UseNiagaraLifetime(FMath::Max(Style->Slash.Duration>0?Style->Slash.Duration:.24f,Style->Range.Duration>0?Style->Range.Duration:.24f));return;}
    }
    Effect->SetColor(Stage==2?FLinearColor(1,.65f,.18f):FamilyColor(EICWeaponFamily::Sword));
    const float Half=FMath::DegreesToRadians(FMath::Clamp(Angle,1.,180.)*.5);
    const float Floor=-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+5;
    // The faint lower edge shows the actual damage cone. The brighter upper
    // arc follows alternating combo swings and fades out with recovery.
    for(int32 I=0;I<12;++I)
    {
        const float A=-Half+2*Half*I/12, B=-Half+2*Half*(I+1)/12;
        const FVector From(Radius*FMath::Cos(A),Radius*FMath::Sin(A),Floor);
        const FVector To(Radius*FMath::Cos(B),Radius*FMath::Sin(B),Floor);
        Effect->Line(I,From,To,.7f);
        const float Slope=Stage==2?0:(Stage==0?1:-1)*.22f;
        Effect->Line(I+12,FVector(From.X,From.Y,-12+From.Y*Slope),FVector(To.X,To.Y,-12+To.Y*Slope),Stage==2?3.f:2.f);
    }
    Effect->Line(24,FVector(0,0,Floor),FVector(Radius*FMath::Cos(Half),-Radius*FMath::Sin(Half),Floor),.5f);
    Effect->Line(25,FVector(0,0,Floor),FVector(Radius*FMath::Cos(Half),Radius*FMath::Sin(Half),Floor),.5f);
}
void AICAttackEffect::SpawnImpact(UWorld* World,const FVector& Location,EICWeaponFamily Family)
{
    if(!World||World->GetNetMode()==NM_DedicatedServer)return;
    auto* Effect=World->SpawnActor<AICAttackEffect>(Location,FRotator::ZeroRotator);if(!Effect)return;
    Effect->bImpact=true;Effect->Duration=.16f;Effect->SetColor(FamilyColor(Family));
    for(int32 I=0;I<12;++I)
    {
        const FVector Direction=FRotator(I%3*60.f,I*137.5f,0).Vector();
        Effect->Line(I,Direction*3,Direction*18,1.2f);
    }
}
void AICAttackEffect::Tick(float Seconds)
{
    Super::Tick(Seconds);if(bNiagara)return;Age+=Seconds;const float Alpha=FMath::Clamp(Age/Duration,0.f,1.f);
    if(Glow)Glow->SetVectorParameterValue(TEXT("Color"),Color*(3.f*(1-Alpha)));
    if(bImpact)SetActorScale3D(FVector(1+Alpha*1.6f));
    else for(int32 I=12;I<24;++I)Pieces[I]->SetVisibility(float(I-12)/12<=Alpha*1.8f);
    if(Alpha>=1)Destroy();
}
void AICAttackEffect::UseNiagaraLifetime(float Seconds)
{bNiagara=true;SetActorTickEnabled(false);SetLifeSpan(FMath::Max(.01f,Seconds)+.05f);}
