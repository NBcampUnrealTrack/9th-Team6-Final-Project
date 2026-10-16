#include "Character/Base/ICCharacterBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"
AICCharacterBase::AICCharacterBase() { IC_TRACE_FUNCTION(); PrimaryActorTick.bCanEverTick = true; }
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
    // The existing player controller delegates its persistence interface to this pawn.
    if (Snapshot.Kind != EICCharacterKind::Player && !State->RegisterStateParticipant(ID, this)) return false;
    CharacterID = ID; bRegisteredState = Snapshot.Kind != EICCharacterKind::Player;
    State->OnCharacterChanged.AddUniqueDynamic(this, &ThisClass::StoredCharacterChanged);
    if (AbilitySystem) AbilitySystem->Restore(Snapshot);
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
    IC_TRACE_FUNCTION(); if (const auto* Snapshot = Candidate.Characters.Find(CharacterID))
    { if (AbilitySystem) { AbilitySystem->ResetExecutionForLoad(); AbilitySystem->Restore(*Snapshot); } bZeroHPHandled = Snapshot->IsDowned(); }
}
void AICCharacterBase::StoredCharacterChanged(FName ID)
{
    IC_TRACE_FUNCTION(); if (ID != CharacterID || !AbilitySystem || !WorldState()) return;
    if (const auto* State = WorldState()->GetStoredCharacter(ID)) {AbilitySystem->Restore(*State);bZeroHPHandled=State->IsDowned();}
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
    IC_TRACE_FUNCTION();
    if (bRegisteredState && WorldState())
    { FlushToWorldState_Implementation(WorldState()); WorldState()->UnregisterStateParticipant(CharacterID, this); WorldState()->OnCharacterChanged.RemoveDynamic(this, &ThisClass::StoredCharacterChanged); }
    if(WorldState())WorldState()->OnCharacterChanged.RemoveDynamic(this,&ThisClass::StoredCharacterChanged);
    if (AbilitySystem) AbilitySystem->Shutdown();
    Super::EndPlay(Reason);
}
void AICCharacterBase::ReleaseForLoad()
{
    IC_TRACE_FUNCTION();bSuppressPersistence=true;
    if(WorldState()){WorldState()->UnregisterStateParticipant(CharacterID,this);WorldState()->OnCharacterChanged.RemoveDynamic(this,&ThisClass::StoredCharacterChanged);}bRegisteredState=false;
    if(AbilitySystem)AbilitySystem->Shutdown();
}
