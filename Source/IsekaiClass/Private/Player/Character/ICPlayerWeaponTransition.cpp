#include "Player/Character/ICPlayerCharacter.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

void AICPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    Inventory->OnChanged.AddUniqueDynamic(this, &ThisClass::WeaponInventoryChanged);
}

void AICPlayerCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelWeaponTransition();
    Inventory->OnChanged.RemoveDynamic(this, &ThisClass::WeaponInventoryChanged);
    Super::EndPlay(Reason);
}

bool AICPlayerCharacter::InitializeState(FName ID)
{
    CancelWeaponTransition();
    return Super::InitializeState(ID);
}

void AICPlayerCharacter::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{
    CancelWeaponTransition();
    Super::RestoreFromWorldState_Implementation(Candidate);
    RefreshWeaponAppearance();
}

bool AICPlayerCharacter::IsSwitchingWeapon() const
{
    return WeaponTransition != EWeaponTransition::Idle;
}

bool AICPlayerCharacter::RequestCycleWeapon()
{
    auto* ASC = GetICAbilitySystem();
    if (!Inventory || !ASC || !ASC->IsAlive() || IsSwitchingWeapon() || ASC->GetActionID().IsValid()
        || ASC->HasMatchingGameplayTag(ICTags::Staggered) || ASC->HasMatchingGameplayTag(ICTags::Returning)) return false;
    FICInventoryState Next;
    if (!Inventory->BuildNextWeaponState(Next)) return false;
    const int32 Slot = static_cast<int32>(EPlayerEquipSlot::Weapon);
    PendingWeaponID = Next.bWeaponSheathed ? FGuid() : Next.Equipment[Slot].InstanceID;
    TransitionWeaponID = Inventory->GetSnapshot().Equipment[Slot].InstanceID;
    ++WeaponTransitionSerial;
    // Moving to another occupied slot plays only that weapon's draw montage.
    // Putting away is a separate step only after the last equipped family.
    StartWeaponTransition(PendingWeaponID.IsValid());
    return true;
}

void AICPlayerCharacter::StartWeaponTransition(bool bDraw)
{
    WeaponTransition = bDraw ? EWeaponTransition::Drawing : EWeaponTransition::Sheathing;
    WeaponMontageInstanceID = INDEX_NONE;
    auto* Profile = GetAnimationProfile();
    auto Family = Inventory->GetSelectedWeaponFamily();
    if (bDraw)
    {
        const auto Loadout = Inventory->GetWeaponLoadout();
        const int32 Index = Loadout.IndexOfByPredicate([&](const auto& Item) { return Item.InstanceID == PendingWeaponID; });
        Family = Index == INDEX_NONE ? EICWeaponFamily::None : static_cast<EICWeaponFamily>(Index + 1);
    }
    const auto* Set = Profile ? Profile->WeaponSets.Find(Family) : nullptr;
    WeaponTransitionMontage = Set ? (bDraw ? Set->Draw : Set->Sheathe) : nullptr;
    auto* Anim = GetMesh()->GetAnimInstance();
    if (Anim && WeaponTransitionMontage && Anim->Montage_Play(WeaponTransitionMontage) > 0)
    {
        if (auto* Instance = Anim->GetActiveInstanceForMontage(WeaponTransitionMontage)) WeaponMontageInstanceID = Instance->GetInstanceID();
        const uint32 Serial = WeaponTransitionSerial;
        const EWeaponTransition Phase = WeaponTransition;
        FOnMontageEnded Ended;
        Ended.BindWeakLambda(this, [this, Serial, Phase](UAnimMontage*, bool bInterrupted)
        {
            if (Serial == WeaponTransitionSerial && Phase == WeaponTransition) FinishWeaponTransitionPhase(bInterrupted);
        });
        Anim->Montage_SetEndDelegate(Ended, WeaponTransitionMontage);
    }
    else
    {
        // Asset-free characters (including staged tests) still have a complete
        // state transition. Authored player profiles provide both montages.
        FinishWeaponTransitionPhase(false);
    }
}

bool AICPlayerCharacter::ApplyWeaponVisibility(bool bDraw)
{
    const auto* ASC = GetICAbilitySystem();
    const int32 Slot = static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (!Inventory || !ASC || !ASC->IsAlive()
        || Inventory->GetSnapshot().Equipment[Slot].InstanceID != TransitionWeaponID) return false;
    if (bDraw)
    {
        const FGuid PreviousWeaponID = TransitionWeaponID;
        // OnChanged fires during the commit, so accept the incoming instance
        // before publishing it to the inventory's live listeners.
        TransitionWeaponID = PendingWeaponID;
        if (!Inventory->DrawWeaponForTransition(PendingWeaponID))
        { TransitionWeaponID = PreviousWeaponID; return false; }
    }
    else if (!Inventory->SetWeaponSheathed(true)) return false;
    RefreshWeaponAppearance();
    return true;
}

bool AICPlayerCharacter::NotifyWeaponVisibility(int32 MontageInstanceID, bool bDraw)
{
    if (MontageInstanceID == INDEX_NONE || MontageInstanceID != WeaponMontageInstanceID
        || WeaponTransition != (bDraw ? EWeaponTransition::Drawing : EWeaponTransition::Sheathing)) return false;
    return ApplyWeaponVisibility(bDraw);
}

void AICPlayerCharacter::FinishWeaponTransitionPhase(bool bInterrupted)
{
    if (!IsSwitchingWeapon()) return;
    const bool bDrawing = WeaponTransition == EWeaponTransition::Drawing;
    if (bInterrupted || !ApplyWeaponVisibility(bDrawing)) { CancelWeaponTransition(); return; }
    WeaponTransitionMontage = nullptr;
    WeaponMontageInstanceID = INDEX_NONE;
    CancelWeaponTransition();
}

void AICPlayerCharacter::CancelWeaponTransition()
{
    ++WeaponTransitionSerial;
    auto* Montage = WeaponTransitionMontage.Get();
    WeaponTransition = EWeaponTransition::Idle;
    WeaponTransitionMontage = nullptr;
    WeaponMontageInstanceID = INDEX_NONE;
    PendingWeaponID.Invalidate(); TransitionWeaponID.Invalidate();
    if (Montage)
        if (auto* Anim = GetMesh()->GetAnimInstance(); Anim && Anim->Montage_IsPlaying(Montage)) Anim->Montage_Stop(.1f, Montage);
    RefreshWeaponAppearance();
}

void AICPlayerCharacter::WeaponInventoryChanged()
{
    const int32 Slot = static_cast<int32>(EPlayerEquipSlot::Weapon);
    if (IsSwitchingWeapon() && Inventory->GetSnapshot().Equipment[Slot].InstanceID != TransitionWeaponID) CancelWeaponTransition();
    RefreshWeaponAppearance();
}
