#include "Character/Base/ICCharacterBase.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/Actors/ICWeaponActor.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Animation/AnimMontage.h"
#include "Combat/Party/ICPartyStatus.h"
void AICCharacterBase::UpdatePresentationTransform(float GroundOffsetCm, const FTransform* BedAnchor)
{
    auto* CharacterMesh=GetMesh();auto* Profile=GetAnimationProfile();
    if(!CharacterMesh||!Profile||!CharacterMesh->GetSkeletalMeshAsset())return;
    if(PresentationMesh.Get()!=CharacterMesh->GetSkeletalMeshAsset())
    {PresentationMesh=CharacterMesh->GetSkeletalMeshAsset();StandingMeshTransform=CharacterMesh->GetRelativeTransform();bSleepPresentation=false;}
    const bool Sleeping=BedAnchor!=nullptr;
    // SkeletalMesh MoveComponent resets animation dynamics for every teleport,
    // even when the requested transform is unchanged. Routine presentation
    // updates must preserve spring state; the transition below resets it once.
    if(Sleeping)CharacterMesh->SetWorldTransform(Profile->SleepMeshTransform * *BedAnchor, false, nullptr, ETeleportType::None);
    else
    {
        auto Transform=StandingMeshTransform;Transform.AddToTranslation(FVector(0,0,GroundOffsetCm));
        CharacterMesh->SetRelativeTransform(Transform,false,nullptr,ETeleportType::None);
    }
    if(Sleeping!=bSleepPresentation)
    {
        // A large bed/standing pose change must not leave spring bones carrying the previous pose's inertia.
        CharacterMesh->ResetAnimInstanceDynamics(ETeleportType::ResetPhysics);
        if(EquippedWeaponActor)EquippedWeaponActor->SetActorHiddenInGame(Sleeping||bWeaponSheathed);
        bSleepPresentation=Sleeping;
    }
    const auto* Life=FindComponentByClass<UICPlayerLifeComponent>();
    const bool HideWeapon=Sleeping||bWeaponSheathed||(Life&&Life->IsWorking());
    if(EquippedWeaponActor&&EquippedWeaponActor->IsHidden()!=HideWeapon)EquippedWeaponActor->SetActorHiddenInGame(HideWeapon);
}
AICCharacterBase::AICCharacterBase()
{
    IC_TRACE_FUNCTION(); PrimaryActorTick.bCanEverTick = true;
}
void AICCharacterBase::RefreshWeaponAppearance()
{
    if (!AbilitySystem || !GetGameInstance() || !GetMesh()->GetSkeletalMeshAsset()) return;
    auto* D = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    const auto State = AbilitySystem->Capture();
    const auto* Inventory = FindComponentByClass<UICInventoryComponent>();
    const auto InventoryState = Inventory ? Inventory->GetSnapshot() : FICInventoryState();
    bWeaponSheathed = Inventory && InventoryState.bWeaponSheathed;
    const auto* Working=FindComponentByClass<UICPlayerLifeComponent>();const bool HideForLife=Working&&(Working->IsWorking()||Working->IsSleeping());
    const auto Items = Inventory ? InventoryState.Equipment : State.Equipment;
    const FName ItemID = Items.IsValidIndex(0) ? Items[0].ItemID : NAME_None;
    if (ItemID == VisualEquipmentID)
    {
        if (EquippedWeaponActor) EquippedWeaponActor->SetActorHiddenInGame(bSleepPresentation||bWeaponSheathed||HideForLife);
        return;
    }
    VisualEquipmentID = ItemID;
    FICDefinitionResult Result;
    const auto* Item = D ? D->Find<FICItemRow>(ItemID, Result) : nullptr;
    const auto* Gear = Item ? D->Find<FICEquipmentRow>(Item->EquipmentID, Result) : nullptr;
    if (EquippedWeaponActor) { EquippedWeaponActor->Destroy(); EquippedWeaponActor = nullptr; }
    if (Gear)
    {
        UClass* Type = Gear->WeaponActorClass.IsNull() ? AICWeaponActor::StaticClass() : Gear->WeaponActorClass.LoadSynchronous();
        if (!Type) return;
        FActorSpawnParameters Params; Params.Owner = this; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        EquippedWeaponActor = GetWorld()->SpawnActor<AICWeaponActor>(Type, GetActorTransform(), Params);
        if (!EquippedWeaponActor) return;
        EquippedWeaponActor->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Gear->AttachSocket);
        EquippedWeaponActor->SetActorRelativeTransform(Gear->AttachmentTransform);
        EquippedWeaponActor->Configure(*Gear);
        EquippedWeaponActor->SetActorHiddenInGame(bSleepPresentation||bWeaponSheathed||HideForLife);
    }
}
UICCharacterAnimationProfile* AICCharacterBase::GetAnimationProfile()
{
    if(!LoadedAnimationProfile || LoadedAnimationProfile->GetPathName()!=AnimationProfile.ToSoftObjectPath().ToString())
        LoadedAnimationProfile=AnimationProfile.LoadSynchronous();
    return LoadedAnimationProfile;
}
UAnimMontage* AICCharacterBase::ResolveActionMontage(FName Action)
{
    const auto* Profile = GetAnimationProfile();
    const auto* Set = Profile ? Profile->FindSet(AbilitySystem ? AbilitySystem->GetEquippedWeaponFamily() : Profile->DefaultFamily) : nullptr;
    if (!Set) return nullptr;
    if (Action == TEXT("Attack")) return Set->Attack;
    if (Action == TEXT("Skill")) return Set->Skill;
    if (Action == TEXT("Dodge")) return Set->Dodge;
    return nullptr;
}
UICWorldStateSubsystem* AICCharacterBase::WorldState() const { IC_TRACE_FUNCTION(); return GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr; }
UAbilitySystemComponent* AICCharacterBase::GetAbilitySystemComponent() const { IC_TRACE_FUNCTION(); return AbilitySystem; }
UICAbilitySystemComponent* AICCharacterBase::GetICAbilitySystem() const { IC_TRACE_FUNCTION(); return AbilitySystem; }
FName AICCharacterBase::GetCharacterID() const { IC_TRACE_FUNCTION(); return CharacterID; }
void AICCharacterBase::EnableCombat(bool PrimaryStats, bool PlayerSP)
{
    IC_TRACE_FUNCTION();
    bUsePrimaryStats = PrimaryStats; bUsePlayerSP = PlayerSP;
    if (!AbilitySystem)
    { AbilitySystem = NewObject<UICAbilitySystemComponent>(this, TEXT("ICAbilitySystem")); AddInstanceComponent(AbilitySystem); AbilitySystem->RegisterComponent(); }
    AbilitySystem->InitializeForCharacter(this, PrimaryStats, PlayerSP);
}
void AICCharacterBase::BeginPlay()
{
    IC_TRACE_FUNCTION(); Super::BeginPlay();
    if (AbilitySystem) AbilitySystem->InitializeForCharacter(this, bUsePrimaryStats, bUsePlayerSP);
}
bool AICCharacterBase::InitializeState(FName ID)
{
    IC_TRACE_FUNCTION(); auto* State = WorldState();
    if (!State || !State->GetStoredCharacter(ID) || (bRegisteredState && CharacterID != ID)) return false;
    const FICCharacterState Snapshot = State->GetStoredCharacter(ID)->Kind == EICCharacterKind::Player
        ? *State->GetStoredCharacter(ID) : State->GetSnapshot(ID);
    FString Reason;
    if (AbilitySystem && !AbilitySystem->CanRestore(Snapshot, Reason)) return false;

    if (Snapshot.Kind != EICCharacterKind::Player && !State->RegisterStateParticipant(ID, this)) return false;
    CharacterID = ID; bRegisteredState = Snapshot.Kind != EICCharacterKind::Player;
    State->OnCharacterChanged.AddUniqueDynamic(this, &ThisClass::StoredCharacterChanged);
    State->OnInventoryChanged.AddUniqueDynamic(this,&ThisClass::PartyBuffsChanged);
    if (AbilitySystem) AbilitySystem->Restore(Snapshot);
    RefreshWeaponAppearance();
    bZeroHPHandled = Snapshot.IsDowned();
    UE_LOG(LogICCore, Log, TEXT("[Character.Ready] Actor=%s ID=%s GAS=%d"), *GetName(), *ID.ToString(), AbilitySystem != nullptr);
    return true;
}
bool AICCharacterBase::ReadLiveState_Implementation(FName ID, FICCharacterState& Snapshot) const
{
    IC_TRACE_FUNCTION(); if (ID != CharacterID || !WorldState() || !WorldState()->GetStoredCharacter(ID)) return false;
    Snapshot = AbilitySystem ? AbilitySystem->Capture() : *WorldState()->GetStoredCharacter(ID);
    Snapshot.Position = GetActorLocation(); Snapshot.Rotation = GetActorRotation(); return true;
}
bool AICCharacterBase::FlushToWorldState_Implementation(UICWorldStateSubsystem* State)
{
    IC_TRACE_FUNCTION(); if (bSuppressPersistence || !bRegisteredState || !State || !State->GetStoredCharacter(CharacterID)) return true;
    FICCharacterState Snapshot; if (!ReadLiveState_Implementation(CharacterID, Snapshot)) return false;
    return State->SetCharacterState(Snapshot);
}
bool AICCharacterBase::CanRestoreWorldState_Implementation(const FICWorldState& Candidate) const
{
    IC_TRACE_FUNCTION(); const auto* Snapshot = Candidate.Characters.Find(CharacterID);
    FString Reason; return !Snapshot || !AbilitySystem || AbilitySystem->CanRestore(*Snapshot, Reason);
}
void AICCharacterBase::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{
    if(auto* Anim=Cast<UICCharacterAnimInstance>(GetMesh()->GetAnimInstance()))Anim->ResetForLoad();
    IC_TRACE_FUNCTION(); if (const auto* Snapshot = Candidate.Characters.Find(CharacterID))
    { if (AbilitySystem) { AbilitySystem->ResetExecutionForLoad(); AbilitySystem->Restore(*Snapshot); } bZeroHPHandled = Snapshot->IsDowned(); }
}
void AICCharacterBase::StoredCharacterChanged(FName ID)
{
    IC_TRACE_FUNCTION(); if (!AbilitySystem || !WorldState()) return;
    if(ID!=CharacterID){PartyBuffsChanged();return;}
    if (const auto* State = WorldState()->GetStoredCharacter(ID)) {AbilitySystem->Restore(*State);bZeroHPHandled=State->IsDowned();RefreshWeaponAppearance();}
}
void AICCharacterBase::PartyBuffsChanged()
{
    if(!WorldState()||!AbilitySystem)return;
    const uint32 Key=ICPartyStatus::CompositionKey(WorldState()->ExportState());
    if(Key!=LastPartyBuffKey){LastPartyBuffKey=Key;AbilitySystem->RebuildDerivedStats();}
}
void AICCharacterBase::HandleZeroHP()
{
    IC_TRACE_FUNCTION(); if (bZeroHPHandled) return; bZeroHPHandled = true;
    FlushToWorldState_Implementation(WorldState()); OnDefeated();
}
void AICCharacterBase::OnDefeated_Implementation() { IC_TRACE_FUNCTION(); }
bool AICCharacterBase::IsHostileTo(const AICCharacterBase* Other) const { IC_TRACE_FUNCTION(); return Other && Other != this && GetClass()->IsChildOf(AICCharacterBase::StaticClass()) && (ActorHasTag(TEXT("IC.Enemy")) != Other->ActorHasTag(TEXT("IC.Enemy"))); }
void AICCharacterBase::EndPlay(const EEndPlayReason::Type Reason)
{
    if (EquippedWeaponActor) EquippedWeaponActor->Destroy();
    IC_TRACE_FUNCTION();
    if (bRegisteredState && WorldState())
    { FlushToWorldState_Implementation(WorldState()); WorldState()->UnregisterStateParticipant(CharacterID, this); WorldState()->OnCharacterChanged.RemoveDynamic(this, &ThisClass::StoredCharacterChanged); }
    if(WorldState()){WorldState()->OnCharacterChanged.RemoveDynamic(this,&ThisClass::StoredCharacterChanged);WorldState()->OnInventoryChanged.RemoveDynamic(this,&ThisClass::PartyBuffsChanged);}
    if (AbilitySystem) AbilitySystem->Shutdown();
    Super::EndPlay(Reason);
}
void AICCharacterBase::ReleaseForLoad()
{
    IC_TRACE_FUNCTION();bSuppressPersistence=true;
    if(WorldState()){WorldState()->UnregisterStateParticipant(CharacterID,this);WorldState()->OnCharacterChanged.RemoveDynamic(this,&ThisClass::StoredCharacterChanged);WorldState()->OnInventoryChanged.RemoveDynamic(this,&ThisClass::PartyBuffsChanged);}bRegisteredState=false;
    if(AbilitySystem)AbilitySystem->Shutdown();
}
