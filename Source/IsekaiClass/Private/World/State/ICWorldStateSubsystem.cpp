#include "World/State/ICWorldStateSubsystem.h"
#include "World/State/ICStateParticipant.h"
#include "Internationalization/Regex.h"

namespace
{
bool ValidItems(const TArray<FPlayerItemInstance>& Items, TSet<FGuid>& IDs)
{
    for (const auto& Item : Items)
    {
        if (Item.IsEmpty())
        {
            if (!Item.ItemID.IsNone() || Item.Count != 0) return false;
            continue;
        }
        if (!Item.InstanceID.IsValid() || IDs.Contains(Item.InstanceID) || Item.Count > 99 || Item.Grade < 0 || Item.Upgrade < 0) return false;
        for (const auto& Option : Item.Options) if (Option.Key.IsNone() || !FMath::IsFinite(Option.Value)) return false;
        IDs.Add(Item.InstanceID);
    }
    return true;
}

bool ValidCharacter(const FICCharacterState& C)
{
    if (!UICWorldStateSubsystem::IsCharacterIdValid(C.CharacterId) || C.Affinity < 0 || C.Affinity > 100
        || static_cast<uint8>(C.Kind) > static_cast<uint8>(EICCharacterKind::Background)
        || static_cast<uint8>(C.Mode) > static_cast<uint8>(EICCharacterMode::Downed)
        || C.PartySlot < INDEX_NONE || C.PartySlot > 3 || !C.MainStats.IsValid()
        || !FMath::IsFinite(C.LastSimGameMinute) || C.LastSimGameMinute < 0) return false;
    if ((C.CharacterId == UICWorldStateSubsystem::PlayerId()) != (C.Kind == EICCharacterKind::Player)) return false;
    if (C.Kind != EICCharacterKind::Player)
    {
        const int32 Serial = FCString::Atoi(*C.CharacterId.ToString().Right(3));
        if ((C.Kind == EICCharacterKind::Recruitable && (Serial < 1 || Serial > 99))
            || (C.Kind == EICCharacterKind::Resident && (Serial < 101 || Serial > 199))
            || (C.Kind == EICCharacterKind::Background && (Serial < 201 || Serial > 299))) return false;
    }
    if (C.Kind != EICCharacterKind::Recruitable && (C.PartySlot != INDEX_NONE || C.Mode != EICCharacterMode::Scheduled)) return false;
    if (C.Kind == EICCharacterKind::Recruitable)
    {
        if ((C.PartySlot >= 0) != (C.Mode != EICCharacterMode::Scheduled)) return false;
    }
    if (C.Kind == EICCharacterKind::Background && C.Affinity != 0) return false;
    if (!FMath::IsFinite(C.HP) || !FMath::IsFinite(C.MP) || !FMath::IsFinite(C.SP)
        || !FMath::IsFinite(C.MaxHP) || !FMath::IsFinite(C.MaxMP) || !FMath::IsFinite(C.MaxSP)
        || C.MaxHP <= 0 || C.MaxMP < 0 || C.MaxSP <= 0
        || C.HP < 0 || C.HP > C.MaxHP || C.MP < 0 || C.MP > C.MaxMP || C.SP < 0 || C.SP > C.MaxSP) return false;
    for (const auto& D : C.Debuffs) if (D.ID.IsNone() || !FMath::IsFinite(D.RemainingSeconds) || D.RemainingSeconds < 0) return false;
    return true;
}
}

bool FICMainStats::IsValid() const
{
    return STR >= 1 && STR <= 10000 && DEX >= 1 && DEX <= 10000 && INT >= 1 && INT <= 10000
        && CON >= 1 && CON <= 10000 && CHA >= 1 && CHA <= 10000 && LUK >= 1 && LUK <= 10000;
}

bool FICMainStats::Set(EICMainStat Stat, int32 Value)
{
    if (Value < 1 || Value > 10000) return false;
    switch (Stat)
    {
        case EICMainStat::STR: STR = Value; break;
        case EICMainStat::DEX: DEX = Value; break;
        case EICMainStat::INT: INT = Value; break;
        case EICMainStat::CON: CON = Value; break;
        case EICMainStat::CHA: CHA = Value; break;
        case EICMainStat::LUK: LUK = Value; break;
        default: return false;
    }
    return true;
}

bool UICWorldStateSubsystem::IsCharacterIdValid(FName ID)
{
    if (ID == PlayerId()) return true;
    const FString Text = ID.ToString();
    FRegexMatcher Match(FRegexPattern(TEXT("^NPC_R0[0-4]_[0-9]{3}$")), Text);
    return Match.FindNext() && !Text.EndsWith(TEXT("_000"));
}

FICCharacterState UICWorldStateSubsystem::GetSnapshot(FName ID) const
{
    if (ID == TEXT("Player")) ID = PlayerId();
    const auto* Stored = State.Characters.Find(ID);
    if (!Stored) return FICCharacterState();
    FICCharacterState Result = *Stored;
    if (const auto* Entry = Participants.Find(ID); Entry && Entry->IsValid())
    {
        FICCharacterState Live = Result;
        if (IICStateParticipant::Execute_ReadLiveState(Entry->Get(), ID, Live) && ValidCharacter(Live) && Live.CharacterId == ID)
        {
            Result.HP = Live.HP; Result.MP = Live.MP; Result.SP = Live.SP;
            Result.MaxHP = Live.MaxHP; Result.MaxMP = Live.MaxMP; Result.MaxSP = Live.MaxSP;
            Result.Debuffs = Live.Debuffs; Result.MainStats=Live.MainStats;
        }
    }
    if (ID == PlayerId()) Result.Equipment = State.Inventory.Equipment;
    return Result;
}

bool UICWorldStateSubsystem::EnsureCharacter(FName ID, EICCharacterKind Kind)
{
    if (const auto* Existing = State.Characters.Find(ID)) return Existing->Kind == Kind;
    FICCharacterState C; C.CharacterId = ID; C.Kind = Kind;
    return SetCharacterState(C);
}

bool UICWorldStateSubsystem::SetCharacterState(const FICCharacterState& Character)
{
    if (!ValidCharacter(Character)) return false;
    FICWorldState Next = State;
    Next.Characters.Add(Character.CharacterId, Character);
    if (!ValidateState(Next)) return false;
    State = MoveTemp(Next);
    OnCharacterChanged.Broadcast(Character.CharacterId);
    return true;
}

bool UICWorldStateSubsystem::SetAffinity(FName ID, int32 Value)
{
    const auto* C = State.Characters.Find(ID);
    if (!C || C->Kind == EICCharacterKind::Player || C->Kind == EICCharacterKind::Background) return false;
    FICCharacterState Next = *C; Next.Affinity = Value;
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetMainStat(FName ID, EICMainStat Stat, int32 Value)
{
    const auto* C = State.Characters.Find(ID);
    if (!C || !C->HasStats()) return false;
    FICCharacterState Next = *C;
    return Next.MainStats.Set(Stat, Value) && SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetVitals(FName ID, float HP, float MP, float SP)
{
    const auto* C = State.Characters.Find(ID);
    if (!C || !C->HasStats()) return false;
    FICCharacterState Next = *C; Next.HP = HP; Next.MP = MP; Next.SP = SP;
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetCharacterMode(FName ID, EICCharacterMode Mode)
{
    const auto* C = State.Characters.Find(ID);
    if (!C || C->Kind != EICCharacterKind::Recruitable || (C->Mode == EICCharacterMode::Downed && Mode == EICCharacterMode::Scheduled)) return false;
    FICCharacterState Next = *C; Next.Mode = Mode;
    if (Mode == EICCharacterMode::Scheduled) Next.PartySlot = INDEX_NONE;
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetPartySlot(FName ID, int32 Slot)
{
    const auto* C = State.Characters.Find(ID);
    if (!C || C->Kind != EICCharacterKind::Recruitable || (C->Mode == EICCharacterMode::Downed && Slot == INDEX_NONE)
        || (Slot >= 0 && C->PartySlot == INDEX_NONE && C->Affinity != 100)) return false;
    FICCharacterState Next = *C; Next.PartySlot = Slot;
    Next.Mode = Slot == INDEX_NONE ? EICCharacterMode::Scheduled : (C->Mode == EICCharacterMode::Downed ? EICCharacterMode::Downed : EICCharacterMode::Companion);
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::ValidateInventoryShape(const FICInventoryState& I, TSet<FGuid>* ExistingIDs)
{
    if (I.Bag.Slots.Num() < 20 || I.Bag.Slots.Num() > 100 || (I.Bag.Slots.Num() - 20) % 10 != 0
        || I.Equipment.Num() != 4 || I.QuickSlots.Num() != 4) return false;
    TSet<FGuid> Local;
    TSet<FGuid>& IDs = ExistingIDs ? *ExistingIDs : Local;
    if (!ValidItems(I.Bag.Slots, IDs) || !ValidItems(I.QuestItems, IDs) || !ValidItems(I.Equipment, IDs)) return false;
    for (const auto& Container : I.Storage)
    {
        if (Container.Key.IsNone() || Container.Value.Slots.Num() != 40 || !ValidItems(Container.Value.Slots, IDs)) return false;
    }
    return true;
}

bool UICWorldStateSubsystem::ValidateState(const FICWorldState& Candidate)
{
    if(Candidate.Gold<0)return false;
    if (Candidate.Preset < 0 || Candidate.Preset > 1 || Candidate.Job < 0 || Candidate.Job > 3
        || Candidate.PlayerPosition.ContainsNaN() || Candidate.PlayerRotation.ContainsNaN()) return false;
    if (Candidate.bPlayerCreated)
    {
        if (Candidate.PlayerName.TrimStartAndEnd().IsEmpty() || Candidate.PlayerName.Len() > 12 || !Candidate.Characters.Contains(PlayerId()) || Candidate.FirstSkill.IsNone()) return false;
        for (TCHAR Ch : Candidate.PlayerName) if (Ch < TEXT(' ') || Ch == 0x2028 || Ch == 0x2029) return false;
    }
    TSet<int32> Party;
    TSet<FGuid> IDs;
    if (!ValidateInventoryShape(Candidate.Inventory, &IDs)) return false;
    for (const auto& Entry : Candidate.Characters)
    {
        const auto& C = Entry.Value;
        if (Entry.Key != C.CharacterId || !ValidCharacter(C)) return false;
        if (C.CharacterId == PlayerId() && !C.Equipment.IsEmpty()) return false; // Player equipment lives in Inventory.
        if (!C.HasStats() && (!C.Equipment.IsEmpty() || !C.Debuffs.IsEmpty())) return false;
        if (!ValidItems(C.Equipment, IDs)) return false;
        if (C.PartySlot >= 0)
        {
            if (Party.Contains(C.PartySlot)) return false;
            Party.Add(C.PartySlot);
        }
    }
    for (const auto& Q : Candidate.Quests)
    {
        if (Q.Key.IsNone() || (Q.Value.bCompleted && !Q.Value.bAccepted)) return false;
        for (const auto& O : Q.Value.Objectives) if (O.Key.IsNone() || O.Value < 0) return false;
    }
    for (const auto& Entry : Candidate.MonsterKilledAt) if (Entry.Key.IsNone() || !FMath::IsFinite(Entry.Value) || Entry.Value < 0) return false;
    for (const auto& Entry : Candidate.ResourceReadyAt) if (Entry.Key.IsNone() || !FMath::IsFinite(Entry.Value) || Entry.Value < 0) return false;
    for (FName ID : Candidate.UnlockedWarps) if (ID.IsNone()) return false;
    for (FName ID : Candidate.DefeatedBosses) if (ID.IsNone()) return false;
    TSet<FGuid> Drops;
    for (const auto& Drop : Candidate.Drops)
    {
        if (!Drop.ID.IsValid() || Drops.Contains(Drop.ID) || Drop.Position.ContainsNaN() || !FMath::IsFinite(Drop.CreatedPlaySecond)
            || Drop.CreatedPlaySecond < 0 || Drop.Contents.IsEmpty() || !ValidItems(Drop.Contents, IDs)) return false;
        Drops.Add(Drop.ID);
    }
    return !Candidate.bEndingCleared || Candidate.EndingKind == TEXT("Ending.Normal") || Candidate.EndingKind == TEXT("Ending.Hidden");
}

bool UICWorldStateSubsystem::CommitInventoryState(const FICInventoryState& Inventory)
{
    FICWorldState Next = State; Next.Inventory = Inventory;
    if (!ValidateState(Next)) return false;
    State.Inventory = Inventory;
    OnInventoryChanged.Broadcast();
    return true;
}

bool UICWorldStateSubsystem::SetPlayerTransform(FVector Position, FRotator Rotation)
{
    if (Position.ContainsNaN() || Rotation.ContainsNaN()) return false;
    State.PlayerPosition = Position; State.PlayerRotation = Rotation;
    return true;
}

bool UICWorldStateSubsystem::CommitCharacterAndInventory(const FICCharacterState& Character, const FICInventoryState& Inventory)
{
    FICWorldState Next = State; Next.Characters.Add(Character.CharacterId, Character); Next.Inventory = Inventory;
    if (!ValidateState(Next)) return false;
    State = MoveTemp(Next);
    OnCharacterChanged.Broadcast(Character.CharacterId); OnInventoryChanged.Broadcast();
    return true;
}

bool UICWorldStateSubsystem::RegisterStateParticipant(FName ID, UObject* Participant)
{
    if (!IsCharacterIdValid(ID) || !IsValid(Participant) || !Participant->GetClass()->ImplementsInterface(UICStateParticipant::StaticClass())) return false;
    if (const auto* Existing = Participants.Find(ID); Existing && Existing->IsValid() && Existing->Get() != Participant) return false;
    Participants.Add(ID, Participant);
    return true;
}

void UICWorldStateSubsystem::UnregisterStateParticipant(FName ID, UObject* Participant)
{
    if (const auto* Existing = Participants.Find(ID); Existing && Existing->Get() == Participant) Participants.Remove(ID);
}

bool UICWorldStateSubsystem::FlushParticipants()
{
    const FICWorldState Before = State;
    for (const auto& Entry : Participants)
    {
        if (Entry.Value.IsValid() && !IICStateParticipant::Execute_FlushToWorldState(Entry.Value.Get(), this))
        {
            State = Before;
            OnWorldRestored.Broadcast();
            return false;
        }
    }
    return ValidateState(State);
}

bool UICWorldStateSubsystem::CanRestoreState(const FICWorldState& Candidate) const
{
    if (!ValidateState(Candidate)) return false;
    for (const auto& Entry : Participants)
        if (Entry.Value.IsValid() && !IICStateParticipant::Execute_CanRestoreWorldState(Entry.Value.Get(), Candidate)) return false;
    return true;
}

bool UICWorldStateSubsystem::RestoreState(const FICWorldState& Candidate)
{
    if (!CanRestoreState(Candidate)) return false;
    State = Candidate;
    for (const auto& Entry : Participants)
        if (Entry.Value.IsValid()) IICStateParticipant::Execute_RestoreFromWorldState(Entry.Value.Get(), State);
    OnWorldRestored.Broadcast();
    OnInventoryChanged.Broadcast();
    return true;
}

bool UICWorldStateSubsystem::ApplyWorldProgress(const FICWorldState& Candidate)
{
    FICWorldState Next = State;
    Next.Quests = Candidate.Quests; Next.UnlockedWarps = Candidate.UnlockedWarps;
    Next.MonsterKilledAt = Candidate.MonsterKilledAt; Next.DefeatedBosses = Candidate.DefeatedBosses;
    Next.ResourceReadyAt = Candidate.ResourceReadyAt; Next.Drops = Candidate.Drops;
    Next.bEndingCleared = Candidate.bEndingCleared; Next.EndingKind = Candidate.EndingKind;
    if (!ValidateState(Next)) return false;
    State = MoveTemp(Next);
    return true;
}

bool UICWorldStateSubsystem::SetPlayerGold(int32 Value)
{
    if(Value<0)return false;
    if(State.Gold!=Value){State.Gold=Value;OnPlayerGoldChanged.Broadcast();}
    return true;
}
