#include "Player/Character/ICPlayerCharacter.h"
#include "Player/Character/ICCameraSpringArm.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Core/Controller/ICPlayerController.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Debug/ICLog.h"
#include "Player/Character/ICPlayerAppearance.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Quest/ICQuestGuideComponent.h"
#include "World/Warp/ICPartyWarpComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"

void AICPlayerCharacter::AdjustCameraDistance(float Axis)
{
    if(!CameraArm)return;
    if(DesiredCameraDistance<0)DesiredCameraDistance=CameraArm->TargetArmLength;
    const float Minimum=FMath::Max(50.f,MinimumCameraDistance);
    DesiredCameraDistance=FMath::Clamp(DesiredCameraDistance-Axis*CameraZoomStep,Minimum,FMath::Max(Minimum,MaximumCameraDistance));
}
void AICPlayerCharacter::UpdateCameraDistance(float DeltaSeconds)
{
    if(!CameraArm)return;
    if(DesiredCameraDistance<0)DesiredCameraDistance=CameraArm->TargetArmLength;
    const float Minimum=FMath::Max(50.f,MinimumCameraDistance);
    DesiredCameraDistance=FMath::Clamp(DesiredCameraDistance,Minimum,FMath::Max(Minimum,MaximumCameraDistance));
    CameraArm->TargetArmLength=FMath::FInterpTo(CameraArm->TargetArmLength,DesiredCameraDistance,DeltaSeconds,10.f);
}

bool AICPlayerCharacter::IsSprinting() const
{
    return bSprintHeld && AbilitySystem && AbilitySystem->IsAlive()
        && !AbilitySystem->HasRecentCombat()
        && !IsSwitchingWeapon() && !AbilitySystem->GetActionID().IsValid()
        && !AbilitySystem->HasMatchingGameplayTag(ICTags::Staggered)
        && !AbilitySystem->HasMatchingGameplayTag(ICTags::Returning)
        && GetCharacterMovement()->IsMovingOnGround();
}

AICPlayerCharacter::AICPlayerCharacter()
{
    IC_TRACE_FUNCTION();
    AbilitySystem=CreateDefaultSubobject<UICAbilitySystemComponent>(TEXT("ICAbilitySystem"));
    CharacterID=UICWorldStateSubsystem::PlayerId();bUsePrimaryStats=true;bUsePlayerSP=true;
    Inventory=CreateDefaultSubobject<UICInventoryComponent>(TEXT("Inventory"));
    Crafting=CreateDefaultSubobject<UICCraftingComponent>(TEXT("Crafting"));
    LifeActions=CreateDefaultSubobject<UICPlayerLifeComponent>(TEXT("LifeActions"));
    QuestGuide=CreateDefaultSubobject<UICQuestGuideComponent>(TEXT("QuestGuide"));
    PartyWarp=CreateDefaultSubobject<UICPartyWarpComponent>(TEXT("PartyWarp"));
    CameraArm=CreateDefaultSubobject<UICCameraSpringArm>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength=420;
    CameraArm->bUsePawnControlRotation=true;
    CameraArm->bDoCollisionTest=true;
    CameraArm->SocketOffset=FVector(0,45,65);
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    Camera->AspectRatio=ICDisplayPolicy::AspectRatio;
    Camera->bConstrainAspectRatio=true;
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,540,0);
    // Small floor lips may be stepped over; the test platforms require a jump.
    GetCharacterMovement()->MaxStepHeight=20.f;
    GetCharacterMovement()->JumpZVelocity=600;
    GetCharacterMovement()->GravityScale=1.6f;
    GetCharacterMovement()->AirControl=.3f;
    JumpMaxCount=1;
    GetCapsuleComponent()->SetCapsuleSize(34,88);
}

void AICPlayerCharacter::HandleZeroHP()
{
    IC_TRACE_FUNCTION();
    if(bZeroHPHandled)return;
    CancelWeaponTransition();
    if(auto* PC=Cast<AICPlayerController>(GetController()))PC->PlayerDied();
    Super::HandleZeroHP();
}

void AICPlayerCharacter::ApplyPreset(int32 Preset)
{
    IC_TRACE_FUNCTION();
    CancelWeaponTransition();
    CurrentPreset=Preset;
    if (auto* D = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>() : nullptr)
    {
        FICDefinitionResult Result;
        const FName ID = Preset == 1 ? TEXT("PRESET_PLAYER_F") : TEXT("PRESET_PLAYER_M");
        if (const auto* Row = D->Find<FICPlayerPresetRow>(ID, Result))
        {
            AnimationProfile = Row->AnimationProfile;
            if (ICPlayerAppearance::Apply(GetMesh(), Row->Mesh.LoadSynchronous(), Row->AnimationClass.LoadSynchronous(), Row->MeshTransform))
            { RefreshWeaponAppearance(); return; }
        }
    }
    if(!AppearancePresets.IsValidIndex(Preset)){UE_LOG(LogICCore, Warning, TEXT("[Appearance.PresetMissing] Character=%s Preset=%d Available=%d: BP Appearance Presets 확인 필요"),*GetNameSafe(this),Preset,AppearancePresets.Num());return;}
    const FICPlayerAppearancePreset& Appearance=AppearancePresets[Preset];
    const bool bApplied=ICPlayerAppearance::Apply(GetMesh(),Appearance.Mesh,Appearance.AnimClass,Appearance.MeshTransform);
    if(bApplied)RefreshWeaponAppearance();
    UE_LOG(LogICCore, Verbose, TEXT("[Appearance.Preset] Character=%s Preset=%d Success=%d"),*GetNameSafe(this),Preset,bApplied);
}
