#include "PlayerWorldStateSubsystem.h"

void UPlayerWorldStateSubsystem::SetSnapshot(const FPlayerPersistentSnapshot& Snapshot)
{
    if(Snapshot.CharacterID.IsNone())return;
    Characters.Add(Snapshot.CharacterID,Snapshot);
    OnCharacterStateChanged.Broadcast(Snapshot.CharacterID);
}

bool UPlayerWorldStateSubsystem::GetSnapshot(FName CharacterID,FPlayerPersistentSnapshot& OutSnapshot) const
{
    if(const auto* Snapshot=Characters.Find(CharacterID)){OutSnapshot=*Snapshot;return true;}
    return false;
}

int32 UPlayerWorldStateSubsystem::AddMainStat(FName CharacterID,EPlayerMainStat Stat,int32 Amount)
{
    auto* Snapshot=Characters.Find(CharacterID);
    if(!Snapshot||Amount<=0)return 0;
    const int32 Applied=Snapshot->MainStats.Add(Stat,Amount);
    if(Applied>0)OnCharacterStateChanged.Broadcast(CharacterID);
    return Applied;
}

FName UPlayerWorldStateSubsystem::GetPartyTarget(int32 PartyIndex) const
{
    return PartySlots.IsValidIndex(PartyIndex)?PartySlots[PartyIndex]:NAME_None;
}
