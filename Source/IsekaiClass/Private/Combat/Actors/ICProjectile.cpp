#include "Combat/Actors/ICProjectile.h"
#include "Character/Base/ICCharacterBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Combat/Actors/ICProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"
#include "Combat/Actors/ICAttackEffect.h"
#include "Combat/Actors/ICWeaponActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Combat/VFX/ICCombatVFX.h"
AICProjectile::AICProjectile()
{
    IC_TRACE_FUNCTION();PrimaryActorTick.bCanEverTick=true;
    Collision=CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));SetRootComponent(Collision);Collision->InitSphereRadius(8);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));Collision->SetNotifyRigidBodyCollision(true);Collision->OnComponentHit.AddDynamic(this,&ThisClass::Hit);
    Movement=CreateDefaultSubobject<UICProjectileMovementComponent>(TEXT("ProjectileMovement"));Movement->ProjectileGravityScale=0;Movement->bRotationFollowsVelocity=true;
    // Clamp range before movement; disable the default reverse owner dependency.
    Movement->bTickBeforeOwner=false;Movement->AddTickPrerequisiteActor(this);
    GlowMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/IsekaiClass/Combat/Effects/M_AttackGlow.M_AttackGlow")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));Body->SetupAttachment(Collision);Body->SetStaticMesh(Sphere.Object);Body->SetRelativeScale3D(FVector(.14));
    Trail=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trail"));Trail->SetupAttachment(Collision);Trail->SetStaticMesh(Cylinder.Object);Trail->SetRelativeRotation(FRotator(90,0,0));Trail->SetRelativeLocation(FVector(-26,0,0));Trail->SetRelativeScale3D(FVector(.035,.035,.55));
    ArrowTip=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowTip"));ArrowTip->SetupAttachment(Collision);ArrowTip->SetStaticMesh(Cone.Object);ArrowTip->SetRelativeRotation(FRotator(90,0,0));ArrowTip->SetRelativeLocation(FVector(6,0,0));ArrowTip->SetRelativeScale3D(FVector(.09,.09,.16));ArrowTip->SetVisibility(false);
    for(auto* Part:{Body.Get(),Trail.Get(),ArrowTip.Get()}){Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCastShadow(false);}
}
void AICProjectile::InitializeShot(UICAbilitySystemComponent* InSource,const FICSkillEffectRow& InEffect,const FICSkillHitDefinition& Rule,const FICAttackCapture& InCapture,FVector Direction,const FICCombatPresentation& InPresentation)
{
    IC_TRACE_FUNCTION();Source=InSource;Effect=InEffect;Captured=InCapture;Presentation=InPresentation;
    Family=InSource?InSource->GetEquippedWeaponFamily():EICWeaponFamily::None;
    if(auto* Material=GlowMaterial.LoadSynchronous())
    {
        auto* Glow=UMaterialInstanceDynamic::Create(Material,this);Glow->SetVectorParameterValue(TEXT("Color"),AICAttackEffect::FamilyColor(Family)*4);
        for(auto* Part:{Body.Get(),Trail.Get(),ArrowTip.Get()})Part->SetMaterial(0,Glow);
    }
    if(Family==EICWeaponFamily::Bow){Body->SetRelativeScale3D(FVector(.28,.035,.035));ArrowTip->SetVisibility(true);}
    Collision->SetSphereRadius(FMath::Max(1.,Rule.TraceRadiusCm));
    ShotOrigin=GetActorLocation();ShotSpeed=Rule.ProjectileSpeedCmPerSecond;
    MaximumTravel=Rule.ProjectileMaxDistanceCm>0?Rule.ProjectileMaxDistanceCm:ShotSpeed*Rule.ProjectileLifetimePlaySeconds;
    if (!Effect.Presentation.ImpactVFX.IsNull()) Presentation.ImpactVFX=Effect.Presentation.ImpactVFX;
    if (!Effect.Presentation.ImpactSound.IsNull()) Presentation.ImpactSound=Effect.Presentation.ImpactSound;
    if(!Effect.Presentation.Visuals.IsNull())Presentation.Visuals=Effect.Presentation.Visuals;
    if(const auto* Visuals=ICCombatVFX::Resolve(InPresentation))
    {
        const bool Main=ICCombatVFX::Attach(this,Collision,Visuals->Projectile,Rule.ProjectileLifetimePlaySeconds)!=nullptr;
        const bool Tail=ICCombatVFX::Attach(this,Collision,Visuals->Trail,Rule.ProjectileLifetimePlaySeconds)!=nullptr;
        if(Main||!Visuals->Projectile.bEnabled||!Visuals->bLegacyMeshFallback){Body->SetVisibility(false);ArrowTip->SetVisibility(false);}
        if(Tail||!Visuals->Trail.bEnabled||!Visuals->bLegacyMeshFallback)Trail->SetVisibility(false);
    }
    bPierce=Rule.bPierceTargets;bStopAtWorld=Rule.bStopAtWorldCollision;
    RemainingTargets=FMath::Max(1,Rule.MaximumTargets);bEnemySource=InSource&&InSource->GetAvatarActor()&&InSource->GetAvatarActor()->ActorHasTag(TEXT("IC.Enemy"));
    if(InSource&&InSource->GetAvatarActor())Collision->IgnoreActorWhenMoving(InSource->GetAvatarActor(),true);
    if(const auto* Character=Cast<AICCharacterBase>(InSource?InSource->GetAvatarActor():nullptr);Character&&Character->EquippedWeaponActor)Collision->IgnoreActorWhenMoving(Character->EquippedWeaponActor,true);
    for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("IC.Enemy"))==bEnemySource)Collision->IgnoreActorWhenMoving(*It,true);
    // Direction is already in world space. Deferred spawning initializes the
    // movement component later; rotating it as local velocity would aim twice.
    Movement->bInitialVelocityInLocalSpace=false;
    Movement->Velocity=Direction.GetSafeNormal()*Rule.ProjectileSpeedCmPerSecond;Movement->InitialSpeed=Movement->MaxSpeed=Rule.ProjectileSpeedCmPerSecond;
    ExpiresAt=(InSource?InSource->PlaySecond():0)+Rule.ProjectileLifetimePlaySeconds;
}
void AICProjectile::Tick(float Seconds)
{
    IC_TRACE_FUNCTION();Super::Tick(Seconds);auto* Time=GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;
    const double Remaining=MaximumTravel-FVector::Distance(ShotOrigin,GetActorLocation());
    if(Remaining<=.1||(Time&&Time->GetClock().PlaySecond>=ExpiresAt)){Destroy();return;}
    // Clamp this frame's swept movement to the exact range, even at low FPS.
    Movement->Velocity=Movement->Velocity.GetSafeNormal()*FMath::Min(ShotSpeed,Remaining/FMath::Max(.0001,double(Seconds)));
}
void AICProjectile::Hit(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Result)
{
    IC_TRACE_FUNCTION();if(!Other||Other==GetOwner()||HitActors.Contains(Other))return;
    auto* Character=Cast<AICCharacterBase>(Other);
    if(Character&&Character->ActorHasTag(TEXT("IC.Enemy"))!=bEnemySource&&Character->GetICAbilitySystem())
    {
        HitActors.Add(Other);auto* Target=Character->GetICAbilitySystem();
        if(!ICCombatVFX::Resolve(Presentation))AICAttackEffect::SpawnImpact(GetWorld(),Result.ImpactPoint,Family);
        if(Source.IsValid()) {if(Source->ExecuteEffect(Effect,Target,&Captured))Source->PlayCombatCue(true,Presentation,Character,Result.ImpactPoint);}
        else if(Effect.Kind==EICSkillEffectKind::Damage)
        {
            Target->ApplyDamage(Captured,nullptr,Effect.bCanCritical,Effect.bCanBeGuarded,Effect.bCanBeAutoDodged,Effect.bCancelsCastingOnEffectiveHit);
            if(const auto* Config=ICCombatVFX::Resolve(Presentation))ICCombatVFX::Spawn(GetWorld(),FTransform(Result.ImpactPoint),Config->Impact,.16f);
        }
        if(!bPierce||--RemainingTargets<=0)Destroy();else {Collision->IgnoreActorWhenMoving(Other,true);Movement->Activate();}
    }
    else if(!Character&&bStopAtWorld)
    {
        if(const auto* Config=ICCombatVFX::Resolve(Presentation))ICCombatVFX::Spawn(GetWorld(),FTransform(Result.ImpactPoint),Config->Impact,.16f);
        else AICAttackEffect::SpawnImpact(GetWorld(),Result.ImpactPoint,Family);
        Destroy();
    }
    else Collision->IgnoreActorWhenMoving(Other,true);
}
