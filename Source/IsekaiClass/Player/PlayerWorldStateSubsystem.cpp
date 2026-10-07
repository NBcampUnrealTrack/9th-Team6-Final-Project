#include "PlayerWorldStateSubsystem.h"
#include "PlayerStateConversions.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
UICWorldStateSubsystem* CoreState(const UPlayerWorldStateSubsystem* Owner)
{ auto* Instance=Owner->GetGameInstance();return Instance?Instance->GetSubsystem<UICWorldStateSubsystem>():nullptr; }
}

void UPlayerWorldStateSubsystem::SetSnapshot(const FPlayerPersistentSnapshot& Snapshot)
{
    if(Snapshot.CharacterID.IsNone()||!Snapshot.MainStats.IsValid()||!Snapshot.Survival.IsValid()||Snapshot.Gold<0)return;
    if(auto* Core=CoreState(this))
    {
        FICCharacterState State;
        if(const auto* Stored=Core->GetStoredCharacter(Snapshot.CharacterID))State=*Stored;
        else {State.CharacterId=Snapshot.CharacterID;State.Kind=Snapshot.CharacterID==UICWorldStateSubsystem::PlayerId()?EICCharacterKind::Player:EICCharacterKind::Recruitable;}
        PlayerStateConversions::Apply(Snapshot.MainStats,Snapshot.Survival,State);
        if(State.Kind==EICCharacterKind::Player)
        {
            FICInventoryState Inventory;Inventory.Bag=Snapshot.Bag;Inventory.QuestItems=Snapshot.QuestItems;
            Inventory.Equipment=Snapshot.Equipment;Inventory.QuickSlots=Snapshot.QuickSlots;Inventory.Storage=Snapshot.Storage;
            if(!Core->CommitCharacterAndInventory(State,Inventory))return;
            Core->SetPlayerGold(Snapshot.Gold);
        }
        else {State.Equipment=Snapshot.Equipment;if(!Core->SetCharacterState(State))return;}
    }
    Characters.Add(Snapshot.CharacterID,Snapshot);
    OnCharacterStateChanged.Broadcast(Snapshot.CharacterID);
}
bool UPlayerWorldStateSubsystem::GetSnapshot(FName CharacterID,FPlayerPersistentSnapshot& OutSnapshot) const
{
    if(auto* Core=CoreState(this))
    {
        if(!Core->GetStoredCharacter(CharacterID))return false;
        const auto State=Core->GetSnapshot(CharacterID);OutSnapshot=FPlayerPersistentSnapshot();OutSnapshot.CharacterID=CharacterID;
        OutSnapshot.MainStats=PlayerStateConversions::Main(State.MainStats);OutSnapshot.Survival=PlayerStateConversions::Survival(State);
        OutSnapshot.Equipment=State.Equipment;
        if(CharacterID==UICWorldStateSubsystem::PlayerId())
        {
            const auto World=Core->ExportState();OutSnapshot.Gold=World.Gold;OutSnapshot.Bag=World.Inventory.Bag;
            OutSnapshot.QuestItems=World.Inventory.QuestItems;OutSnapshot.Equipment=World.Inventory.Equipment;
            OutSnapshot.QuickSlots=World.Inventory.QuickSlots;OutSnapshot.Storage=World.Inventory.Storage;
        }
        return true;
    }
    if(const auto* Snapshot=Characters.Find(CharacterID)){OutSnapshot=*Snapshot;return true;}return false;
}
int32 UPlayerWorldStateSubsystem::AddMainStat(FName CharacterID,EPlayerMainStat Stat,int32 Amount)
{
    FPlayerPersistentSnapshot Snapshot;if(Amount<=0||!GetSnapshot(CharacterID,Snapshot))return 0;
    const int32 Applied=Snapshot.MainStats.Add(Stat,Amount);
    if(Applied>0)SetSnapshot(Snapshot);return Applied;
}
FName UPlayerWorldStateSubsystem::GetPartyTarget(int32 PartyIndex) const
{
    if(PartyIndex<0||PartyIndex>3)return NAME_None;
    if(auto* Core=CoreState(this))for(const auto& Entry:Core->ExportState().Characters)
        if(Entry.Value.PartySlot==PartyIndex)return Entry.Key;
    return PartySlots.IsValidIndex(PartyIndex)?PartySlots[PartyIndex]:NAME_None;
}
