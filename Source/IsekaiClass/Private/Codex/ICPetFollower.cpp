#include "Codex/ICPetFollower.h"
#include "Enemy/ICMonsterPresentation.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AICPetFollower::AICPetFollower()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("FollowAnchor"));
    Visual=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PetVisual"));Visual->SetupAttachment(RootComponent);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);Visual->SetGenerateOverlapEvents(false);
    Visual->SetCanEverAffectNavigation(false);Visual->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    SetActorEnableCollision(false);Tags.Add(TEXT("IC.Pet"));
}
bool AICPetFollower::InitializePet(AICCharacterBase* Character,const FICMonsterRow& Definition)
{
    if(!Character||Definition.PetCodexIndex<0)return false;
    auto* Type=Definition.PawnClass.LoadSynchronous();
    const auto* Template=Type?Cast<ACharacter>(Type->GetDefaultObject()):nullptr;
    const auto* Source=Template?Template->GetMesh():nullptr;
    auto* Mesh=Definition.Mesh.LoadSynchronous();if(!Mesh&&Source)Mesh=Source->GetSkeletalMeshAsset();
    if(!Mesh)return false;
    FollowedCharacter=Character;CharacterID=Character->GetCharacterID();PetIndex=Definition.PetCodexIndex;SetOwner(Character);
    Visual->SetSkeletalMesh(Mesh);
    Visual->SetRelativeLocation(FVector::ZeroVector);
    Visual->SetRelativeScale3D(FVector::OneVector);
    if(Source)
    {
        Visual->SetRelativeRotation(Source->GetRelativeRotation());
        for(int32 Index=0;Index<Source->GetNumMaterials();++Index)Visual->SetMaterial(Index,Source->GetMaterial(Index));
    }
    // Pets use the shared field reference so the boss keeps a compact companion size.
    if(!ICMonsterPresentation::FitHeight(Visual,ICMonsterPresentation::PetHeight(Definition)))return false;
    ICMonsterPresentation::SetFeetHeight(Visual,GetActorLocation().Z);
    IdleAnimation=Definition.IdleAnimation.LoadSynchronous();MoveAnimation=Definition.MoveAnimation.LoadSynchronous();
    if(!MoveAnimation)MoveAnimation=IdleAnimation;
    PlayAnimation(false);SetActorLocation(Destination());SetActorRotation(FRotator(0,Character->GetActorRotation().Yaw,0));
    return true;
}
FVector AICPetFollower::Destination() const
{
    const auto* Character=FollowedCharacter.Get();if(!Character)return GetActorLocation();
    FVector Goal=Character->GetActorLocation()-Character->GetActorForwardVector()*115.f+Character->GetActorRightVector()*65.f;
    const float Feet=Character->GetActorLocation().Z-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Goal.Z=Feet+2.f;
    FHitResult Hit;FCollisionQueryParams Query;Query.AddIgnoredActor(this);Query.AddIgnoredActor(Character);
    if(GetWorld()->LineTraceSingleByObjectType(Hit,Goal+FVector(0,0,160),Goal-FVector(0,0,350),FCollisionObjectQueryParams(ECC_WorldStatic),Query))Goal.Z=Hit.ImpactPoint.Z+2.f;
    return Goal;
}
void AICPetFollower::PlayAnimation(bool Moving)
{
    auto* Animation=Moving&&MoveAnimation?MoveAnimation.Get():IdleAnimation.Get();
    if(Animation&&Animation!=CurrentAnimation)
    {
        CurrentAnimation=Animation;++AnimationSwitchCount;
        Visual->PlayAnimation(Animation,true);Visual->SetPlayRate(1.f);
    }
}
void AICPetFollower::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);auto* Character=FollowedCharacter.Get();
    if(!IsValid(Character)){Destroy();return;}
    if(DeltaSeconds<=0)return;
    SetActorHiddenInGame(Character->IsHidden());
    const FVector Goal=Destination(),Before=GetActorLocation();
    const float OwnerSpeed=Character->GetVelocity().Size2D();
    const bool OwnerMoving=OwnerSpeed>10.f;
    if(FVector::DistSquared(Before,Goal)>FMath::Square(1500.f))
    {
        SetActorLocation(Goal,false,nullptr,ETeleportType::TeleportPhysics);
        Visual->ResetAnimInstanceDynamics(ETeleportType::ResetPhysics);
        SetActorRotation(FRotator(0,Character->GetActorRotation().Yaw,0));
        bMoving=OwnerMoving;IdleSeconds=0;PlayAnimation(bMoving);return;
    }
    // Exponential arrival follows continuously instead of alternating between
    // overspeed pursuit and a hard stop inside the old 12 cm threshold.
    const float Alpha=1.f-FMath::Exp(-8.f*DeltaSeconds);
    FVector Step=(Goal-Before)*Alpha;
    const float MaxStep=FMath::Max(240.f,OwnerSpeed+200.f)*DeltaSeconds;
    const auto Horizontal=FVector(Step.X,Step.Y,0).GetClampedToMaxSize(MaxStep);
    Step.X=Horizontal.X;Step.Y=Horizontal.Y;
    SetActorLocation(Before+Step);
    const float ActualSpeed=Step.Size2D()/DeltaSeconds;
    if(OwnerMoving||ActualSpeed>20.f){bMoving=true;IdleSeconds=0;}
    else if(bMoving)
    {
        IdleSeconds=ActualSpeed<8.f?IdleSeconds+DeltaSeconds:0.f;
        if(IdleSeconds>=.2f)bMoving=false;
    }
    PlayAnimation(bMoving);
    const auto Rotation=ActualSpeed>5.f?Step.Rotation():Character->GetActorRotation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(),FRotator(0,Rotation.Yaw,0),DeltaSeconds,8.f));
}
