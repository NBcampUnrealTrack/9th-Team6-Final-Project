#include "PlayerCharacter.h"
#include "PlayerAppearance.h"
#include "PlayerInventory.h"
#include "PlayerCrafting.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

APlayerPrototypeCharacter::APlayerPrototypeCharacter()
{
    Inventory=CreateDefaultSubobject<UPlayerInventory>(TEXT("Inventory"));
    Crafting=CreateDefaultSubobject<UPlayerCrafting>(TEXT("Crafting"));
    CameraArm=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength=420;
    CameraArm->bUsePawnControlRotation=true;
    CameraArm->bDoCollisionTest=true;
    CameraArm->SocketOffset=FVector(0,45,65);
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,540,0);
    GetCharacterMovement()->JumpZVelocity=600;
    GetCharacterMovement()->AirControl=.3f;
    JumpMaxCount=1;
    GetCapsuleComponent()->SetCapsuleSize(34,88);
    PlaceholderBody=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    PlaceholderBody->SetStaticMesh(BodyMesh.Object);
    PlaceholderBody->SetRelativeScale3D(FVector(.6,.6,1.6));
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APlayerPrototypeCharacter::BeginPlay()
{
    Super::BeginPlay();
    ApplyPreset(0);
}

void APlayerPrototypeCharacter::ApplyPreset(int32 Preset)
{
    const bool bLoaded=PlayerAppearance::Apply(GetMesh(),Preset,-GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
    PlaceholderBody->SetVisibility(!bLoaded);
    PlaceholderBody->SetHiddenInGame(bLoaded);
}
