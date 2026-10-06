#include "Core/ICPlayerState.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"

UICWorldStateSubsystem* AICPlayerState::GetWorldState() const
{
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
}

void AICPlayerState::BeginPlay()
{
    Super::BeginPlay();
    if (UICWorldStateSubsystem* State = GetWorldState())
    {
        State->OnCharacterChanged.AddUniqueDynamic(this, &ThisClass::HandleCharacterChanged);
        State->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::HandleSnapshotChanged);
        State->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::HandleSnapshotChanged);
        State->OnPlayerGoldChanged.AddUniqueDynamic(this, &ThisClass::HandleSnapshotChanged);
    }
}

void AICPlayerState::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UICWorldStateSubsystem* State = GetWorldState())
    {
        State->OnCharacterChanged.RemoveDynamic(this, &ThisClass::HandleCharacterChanged);
        State->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleSnapshotChanged);
        State->OnWorldRestored.RemoveDynamic(this, &ThisClass::HandleSnapshotChanged);
        State->OnPlayerGoldChanged.RemoveDynamic(this, &ThisClass::HandleSnapshotChanged);
    }
    Super::EndPlay(Reason);
}

FICCharacterState AICPlayerState::GetPlayerSnapshot() const
{
    const UICWorldStateSubsystem* State = GetWorldState();
    return State ? State->GetSnapshot(UICWorldStateSubsystem::PlayerId()) : FICCharacterState();
}

FICInventoryState AICPlayerState::GetInventorySnapshot() const
{
    const UICWorldStateSubsystem* State = GetWorldState();
    return State ? State->GetInventorySnapshot() : FICInventoryState();
}

int32 AICPlayerState::GetGold() const
{
    const UICWorldStateSubsystem* State = GetWorldState();
    return State ? State->ExportState().Gold : 0;
}

bool AICPlayerState::IsPlayerCreated() const
{
    const UICWorldStateSubsystem* State = GetWorldState();
    return State && State->ExportState().bPlayerCreated;
}

void AICPlayerState::HandleCharacterChanged(FName CharacterId)
{
    if (CharacterId == UICWorldStateSubsystem::PlayerId()) HandleSnapshotChanged();
}

void AICPlayerState::HandleSnapshotChanged() { OnSnapshotChanged.Broadcast(); }
