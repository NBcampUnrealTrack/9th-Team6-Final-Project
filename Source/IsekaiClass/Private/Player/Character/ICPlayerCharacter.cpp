#include "Player/Character/ICPlayerCharacter.h"
#include "Player/Character/ICPlayerAppearance.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AICPlayerCharacter::AICPlayerCharacter()
{
    Inventory=CreateDefaultSubobject<UICInventoryComponent>(TEXT("Inventory"));
    Crafting=CreateDefaultSubobject<UICCraftingComponent>(TEXT("Crafting"));
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
}

void AICPlayerCharacter::ApplyPreset(int32 Preset)
{
    CurrentPreset=Preset;
    if(!AppearancePresets.IsValidIndex(Preset))return;
    const FICPlayerAppearancePreset& Appearance=AppearancePresets[Preset];
    ICPlayerAppearance::Apply(GetMesh(),Appearance.Mesh,Appearance.AnimClass,Appearance.MeshTransform);
}
