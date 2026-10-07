#include "Core/ICPlayerState.h"
#include "Debug/ICLog.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"

UICWorldStateSubsystem* AICPlayerState::GetWorldState() const
{
    IC_TRACE_FUNCTION();
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
}

void AICPlayerState::BeginPlay()
{
    IC_TRACE_FUNCTION();
    Super::BeginPlay();
    UE_LOG(LogICCore, Log, TEXT("[PlayerState.BeginPlay] Object=%s State=%s"),*GetNameSafe(this),*GetNameSafe(GetWorldState()));
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
    IC_TRACE_FUNCTION();
    if (UICWorldStateSubsystem* State = GetWorldState())
    {
        State->OnCharacterChanged.RemoveDynamic(this, &ThisClass::HandleCharacterChanged);
        State->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleSnapshotChanged);
        State->OnWorldRestored.RemoveDynamic(this, &ThisClass::HandleSnapshotChanged);
        State->OnPlayerGoldChanged.RemoveDynamic(this, &ThisClass::HandleSnapshotChanged);
    }
    Super::EndPlay(Reason);
    UE_LOG(LogICCore, Log, TEXT("[PlayerState.EndPlay] Object=%s Reason=%d: 상태 이벤트 구독 정리"),*GetNameSafe(this),static_cast<int32>(Reason));
}

FICCharacterState AICPlayerState::GetPlayerSnapshot() const
{
    IC_TRACE_FUNCTION();
    const UICWorldStateSubsystem* State = GetWorldState();
    return State ? State->GetSnapshot(UICWorldStateSubsystem::PlayerId()) : FICCharacterState();
}

FICInventoryState AICPlayerState::GetInventorySnapshot() const
{
    IC_TRACE_FUNCTION();
    const UICWorldStateSubsystem* State = GetWorldState();
    return State ? State->GetInventorySnapshot() : FICInventoryState();
}

int32 AICPlayerState::GetGold() const
{
    IC_TRACE_FUNCTION();
    const UICWorldStateSubsystem* State = GetWorldState();
    return State ? State->ExportState().Gold : 0;
}

bool AICPlayerState::IsPlayerCreated() const
{
    IC_TRACE_FUNCTION();
    const UICWorldStateSubsystem* State = GetWorldState();
    return State && State->ExportState().bPlayerCreated;
}

void AICPlayerState::HandleCharacterChanged(FName CharacterId)
{
    IC_TRACE_FUNCTION();
    if (CharacterId == UICWorldStateSubsystem::PlayerId()) HandleSnapshotChanged();
}

void AICPlayerState::HandleSnapshotChanged() { IC_TRACE_FUNCTION(); OnSnapshotChanged.Broadcast(); }
