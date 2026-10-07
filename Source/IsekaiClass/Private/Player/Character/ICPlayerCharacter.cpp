#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Debug/ICLog.h"
#include "Player/Character/ICPlayerAppearance.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AICPlayerCharacter::AICPlayerCharacter()
{
    IC_TRACE_FUNCTION();
    AbilitySystem=CreateDefaultSubobject<UICAbilitySystemComponent>(TEXT("ICAbilitySystem"));
    CharacterID=UICWorldStateSubsystem::PlayerId();bUsePrimaryStats=true;bUsePlayerSP=true;
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

void AICPlayerCharacter::HandleZeroHP()
{
    IC_TRACE_FUNCTION();
    if(bZeroHPHandled)return;
    if(auto* PC=Cast<AICPlayerController>(GetController()))PC->PlayerDied();
    Super::HandleZeroHP();
}

void AICPlayerCharacter::ApplyPreset(int32 Preset)
{
    IC_TRACE_FUNCTION();
    CurrentPreset=Preset;
    if(!AppearancePresets.IsValidIndex(Preset)){UE_LOG(LogICCore, Warning, TEXT("[Appearance.PresetMissing] Character=%s Preset=%d Available=%d: BP Appearance Presets 확인 필요"),*GetNameSafe(this),Preset,AppearancePresets.Num());return;}
    const FICPlayerAppearancePreset& Appearance=AppearancePresets[Preset];
    const bool bApplied=ICPlayerAppearance::Apply(GetMesh(),Appearance.Mesh,Appearance.AnimClass,Appearance.MeshTransform);
    UE_LOG(LogICCore, Verbose, TEXT("[Appearance.Preset] Character=%s Preset=%d Success=%d"),*GetNameSafe(this),Preset,bApplied);
}
