#include "World/State/ICWorldStateSubsystem.h"
#include "Combat/GAS/Simulation/ICCombatSimulation.h"
#include "Time/ICTimeSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Engine/World.h"
#include "Debug/ICLog.h"
#include "World/State/ICStateParticipant.h"
#include "Internationalization/Regex.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/Character/ICCharacterRows.h"
#include "Data/Rows/World/ICAnchorRow.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Engine/GameInstance.h"

namespace
{
bool ValidItems(const TArray<FPlayerItemInstance>& Items, TSet<FGuid>& IDs)
{
    IC_TRACE_FUNCTION();
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
    IC_TRACE_FUNCTION();
    if (!UICWorldStateSubsystem::IsCharacterIdValid(C.CharacterId) || C.Affinity < 0 || C.Affinity > 500
        || static_cast<uint8>(C.Kind) > static_cast<uint8>(EICCharacterKind::Background)
        || static_cast<uint8>(C.Mode) > static_cast<uint8>(EICCharacterMode::Companion)
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
    IC_TRACE_FUNCTION();
    return STR >= 1 && STR <= 10000 && DEX >= 1 && DEX <= 10000 && INT >= 1 && INT <= 10000
        && CON >= 1 && CON <= 10000 && CHA >= 1 && CHA <= 10000 && LUK >= 1 && LUK <= 10000;
}

bool FICMainStats::Set(EICMainStat Stat, int32 Value)
{
    IC_TRACE_FUNCTION();
    if (Value < 1 || Value > 10000) return false;
    switch (Stat)
    {
        case EICMainStat::STR: STR = Value; break;
        case EICMainStat::DEX: DEX = Value; break;
        case EICMainStat::INT: INT = Value; break;
        case EICMainStat::CON: CON = Value; break;
        case EICMainStat::CHA: return false; // Keep the saved field, disallow new growth.
        case EICMainStat::LUK: LUK = Value; break;
        default: return false;
    }
    return true;
}

bool UICWorldStateSubsystem::IsCharacterIdValid(FName ID)
{
    IC_TRACE_FUNCTION();
    if (ID == PlayerId()) return true;
    const FString Text = ID.ToString();
    FRegexMatcher Match(FRegexPattern(TEXT("^NPC_R0[0-8]_[0-9]{3}$")), Text);
    return Match.FindNext() && !Text.EndsWith(TEXT("_000"));
}

FICCharacterState UICWorldStateSubsystem::GetSnapshot(FName ID) const
{
    IC_TRACE_FUNCTION();
    const_cast<UICWorldStateSubsystem*>(this)->AdvanceOfflineCharacter(ID);
    const auto* Stored = State.Characters.Find(ID);
    if (!Stored) return FICCharacterState();
    FICCharacterState Result = *Stored;
    if (const auto* Entry = Participants.Find(ID); Entry && Entry->IsValid())
    {
        FICCharacterState Live = Result;
        if (IICStateParticipant::Execute_ReadLiveState(Entry->Get(), ID, Live) && ValidCharacter(Live) && Live.CharacterId == ID)
        {
            Result.Position=Live.Position;Result.Rotation=Live.Rotation;Result.LastSimGameMinute=Live.LastSimGameMinute;
            Result.HP = Live.HP; Result.MP = Live.MP; Result.SP = Live.SP;
            Result.MaxHP = Live.MaxHP; Result.MaxMP = Live.MaxMP; Result.MaxSP = Live.MaxSP;
            Result.Debuffs = Live.Debuffs; Result.MainStats=Live.MainStats;
            Result.Combat = Live.Combat; Result.LifeState = Live.LifeState;
            Result.SPDrainElapsedPlaySeconds = Live.SPDrainElapsedPlaySeconds;
            Result.LastSimPlaySecond = Live.LastSimPlaySecond;
        }
    }
    if (ID == PlayerId()) Result.Equipment = State.Inventory.Equipment;
    return Result;
}

bool UICWorldStateSubsystem::EnsureCharacter(FName ID, EICCharacterKind Kind)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.Character.Ensure.Request] ID=%s Kind=%d"),*ID.ToString(),static_cast<int32>(Kind));
    if (const auto* Existing = State.Characters.Find(ID)) return Existing->Kind == Kind;
    if(ID==PlayerId()||!IsCharacterIdValid(ID))return false;
    auto* Definitions=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    const auto* NPC=Definitions?Definitions->Find<FICNPCRow>(ID,R):nullptr;
    if(!NPC||static_cast<int32>(NPC->Type)+1!=static_cast<int32>(Kind))return false;
    const auto* Anchor=Definitions->Find<FICAnchorRow>(NPC->InitialAnchorID,R);if(!Anchor)return false;
    FICCharacterState C;C.CharacterId=ID;C.Kind=Kind;C.Affinity=NPC->InitialAffinity;C.CurrentAnchorID=NPC->InitialAnchorID;
    C.Position=Anchor->WorldTransform.GetLocation();C.Rotation=Anchor->WorldTransform.Rotator();
    if(Kind==EICCharacterKind::Recruitable)
    {
        const auto* Base=Definitions->Find<FICRecruitCombatRow>(NPC->RecruitCombatID,R);
        const auto* Class=Base?Definitions->Find<FICClassRow>(Base->ClassID,R):nullptr;
        const auto* Rules=Definitions->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),R);
        if(!Base||!Class||!Rules||Base->NPCID!=ID||NPC->ClassID!=Base->ClassID||!Definitions->Find<FICSkillRow>(Base->UniqueSkillID,R))return false;
        C.MainStats.STR=Base->InitialBaseStats.STR;C.MainStats.DEX=Base->InitialBaseStats.DEX;C.MainStats.INT=Base->InitialBaseStats.INT;
        C.MainStats.CON=Base->InitialBaseStats.CON;C.MainStats.CHA=Base->InitialBaseStats.CHA;C.MainStats.LUK=Base->InitialBaseStats.LUK;
        C.HP=C.MaxHP=Rules->BaseMaxHP+Rules->HPPerCON*(C.MainStats.CON-1);
        C.MP=C.MaxMP=Rules->BaseMaxMP+Rules->MPPerINT*(C.MainStats.INT-1);C.SP=0;C.MaxSP=1;
        C.UnlockedSkillIDs.Add(Base->UniqueSkillID);C.AttackDisposition=Base->InitialAttackDisposition;
        C.LowHPDisposition=Base->InitialLowHPDisposition;C.TargetPriority=Base->InitialTargetPriority;C.bAutoSkillsEnabled=Base->bInitiallyAutoSkillEnabled;
        const FName GearIDs[]={Base->InitialEquipment.WeaponID,Base->InitialEquipment.ClothesID,Base->InitialEquipment.RingID,Base->InitialEquipment.NecklaceID};
        UICInventoryComponent* Staging=NewObject<UICInventoryComponent>(GetTransientPackage());
        Staging->BeginStaging(Definitions,static_cast<int32>(Class->CharacterClass)-1);
        for(FName GearID:GearIDs)if(!GearID.IsNone())
        {
            const auto* Gear=Definitions->Find<FICEquipmentRow>(GearID,R);
            if(!Gear||!Staging->AddItem(Gear->ItemID,1,false)||!Staging->Equip(0))return false;
        }
        C.Equipment=Staging->GetSnapshot().Equipment;
    }
    else {C.HP=0;C.MP=0;C.SP=0;C.MaxHP=1;C.MaxMP=0;C.MaxSP=1;}
    return SetCharacterState(C);
}

bool UICWorldStateSubsystem::SetCharacterState(const FICCharacterState& Character)
{
    IC_TRACE_FUNCTION();
    if (!ValidCharacter(Character)){UE_LOG(LogICWorldState, Verbose, TEXT("[State.Character.Rejected] ID=%s Kind=%d Mode=%d Slot=%d HP=%.2f MP=%.2f SP=%.2f: 캐릭터 규격 위반"),*Character.CharacterId.ToString(),static_cast<int32>(Character.Kind),static_cast<int32>(Character.Mode),Character.PartySlot,Character.HP,Character.MP,Character.SP);return false;}
    FICWorldState Next = State;
    Next.Characters.Add(Character.CharacterId, Character);
    if (!ValidateState(Next)) return false;
    State = MoveTemp(Next);
    UE_LOG(LogICWorldState, VeryVerbose, TEXT("[State.Character.Committed] ID=%s HP=%.2f MP=%.2f SP=%.2f Affinity=%d"),*Character.CharacterId.ToString(),Character.HP,Character.MP,Character.SP,Character.Affinity);
    OnCharacterChanged.Broadcast(Character.CharacterId);
    return true;
}

bool UICWorldStateSubsystem::SetAffinity(FName ID, int32 Value)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.Affinity.Request] ID=%s Value=%d"),*ID.ToString(),Value);
    const auto* C = State.Characters.Find(ID);
    if (!C || C->Kind == EICCharacterKind::Player || C->Kind == EICCharacterKind::Background) return false;
    FICCharacterState Next = GetSnapshot(ID); Next.Affinity = Value;
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetMainStat(FName ID, EICMainStat Stat, int32 Value)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.MainStat.Request] ID=%s Stat=%d Value=%d"),*ID.ToString(),static_cast<int32>(Stat),Value);
    const auto* C = State.Characters.Find(ID);
    if (!C || !C->HasStats()) return false;
    FICCharacterState Next = GetSnapshot(ID);
    if (ID == PlayerId()) Next.Equipment.Reset();
    return Next.MainStats.Set(Stat, Value) && SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetVitals(FName ID, float HP, float MP, float SP)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.Vitals.Request] ID=%s HP=%.2f MP=%.2f SP=%.2f"),*ID.ToString(),HP,MP,SP);
    const auto* C = State.Characters.Find(ID);
    if (!C || !C->HasStats()) return false;
    FICCharacterState Next = *C; Next.HP = HP; Next.MP = MP; Next.SP = SP;
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetCharacterMode(FName ID, EICCharacterMode Mode)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.Mode.Request] ID=%s Mode=%d"),*ID.ToString(),static_cast<int32>(Mode));
    const auto* C = State.Characters.Find(ID);
    if (!C || C->Kind != EICCharacterKind::Recruitable || (C->IsDowned() && Mode == EICCharacterMode::Scheduled)) return false;
    FICCharacterState Next = *C; Next.Mode = Mode;
    if (Mode == EICCharacterMode::Scheduled) Next.PartySlot = INDEX_NONE;
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::SetPartySlot(FName ID, int32 Slot)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.PartySlot.Request] ID=%s Slot=%d"),*ID.ToString(),Slot);
    const auto* C = State.Characters.Find(ID);
    if (!C || C->Kind != EICCharacterKind::Recruitable || (C->IsDowned() && Slot == INDEX_NONE)
        || (Slot >= 0 && C->PartySlot == INDEX_NONE && C->Affinity < 300)) return false;
    FICCharacterState Next = *C; Next.PartySlot = Slot;
    Next.Mode = Slot == INDEX_NONE ? EICCharacterMode::Scheduled : EICCharacterMode::Companion;
    return SetCharacterState(Next);
}

bool UICWorldStateSubsystem::ValidateInventoryShape(const FICInventoryState& I, TSet<FGuid>* ExistingIDs)
{
    IC_TRACE_FUNCTION();
    if (I.Bag.Slots.Num() < 20 || I.Bag.Slots.Num() > 100 || (I.Bag.Slots.Num() - 20) % 10 != 0
        || I.Equipment.Num() != 4 || I.WeaponSlots.Num() != 4 || I.QuickSlots.Num() != 4) return false;
    TSet<FGuid> Local;
    TSet<FGuid>& IDs = ExistingIDs ? *ExistingIDs : Local;
    if (!ValidItems(I.Bag.Slots, IDs) || !ValidItems(I.QuestItems, IDs) || !ValidItems(I.Equipment, IDs) || !ValidItems(I.WeaponSlots, IDs)) return false;
    for (const auto& Container : I.Storage)
    {
        const int32 Slots=Container.Value.Slots.Num();
        const bool Shape=Container.Key==TEXT("PlayerHome")?(Slots>=40&&Slots<=160&&Slots%40==0):Slots==40;
        if (Container.Key.IsNone() || !Shape || !ValidItems(Container.Value.Slots, IDs)) return false;
    }
    return true;
}

bool UICWorldStateSubsystem::ValidateState(const FICWorldState& Candidate)
{
    IC_TRACE_FUNCTION();
    if(Candidate.Gold<0)return false;
    if (Candidate.Preset < 0 || Candidate.Preset > 1 || Candidate.Job < 0 || Candidate.Job > 3
        || Candidate.PlayerPosition.ContainsNaN() || Candidate.PlayerRotation.ContainsNaN()) return false;
    if (Candidate.bPlayerCreated)
    {
        if (Candidate.PlayerName.TrimStartAndEnd().IsEmpty() || Candidate.PlayerName.Len() > 12 || !Candidate.Characters.Contains(PlayerId()) || Candidate.FirstSkill.IsNone()) return false;
        for (TCHAR Ch : Candidate.PlayerName) if (Ch < TEXT(' ') || Ch == 0x2028 || Ch == 0x2029) return false;
        if(!Candidate.CreationAnswers.Name.IsEmpty()&&(!Candidate.CreationAnswers.IsComplete()
            ||Candidate.CreationAnswers.Name.TrimStartAndEnd()!=Candidate.PlayerName||Candidate.CreationAnswers.Gender!=Candidate.Preset))return false;
    }
    TSet<int32> Party;
    TSet<FGuid> IDs;
    if (!ValidateInventoryShape(Candidate.Inventory, &IDs)) return false;
    for (const auto& Entry : Candidate.Characters)
    {
        const auto& C = Entry.Value;
        if (Entry.Key != C.CharacterId || !ValidCharacter(C)) return false;
        if (C.CharacterId == PlayerId() && !C.Equipment.IsEmpty()) return false;
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
    if (Candidate.bEndingCleared && Candidate.EndingKind != TEXT("Ending.Normal") && Candidate.EndingKind != TEXT("Ending.Hidden")) return false;
    FString Reason;
    return ValidateAdditionalState(Candidate, Reason);
}

bool UICWorldStateSubsystem::CommitInventoryState(const FICInventoryState& Inventory)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.Inventory.Request] BagSlots=%d Containers=%d"),Inventory.Bag.Slots.Num(),Inventory.Storage.Num());
    FICWorldState Next = State; Next.Inventory = Inventory;
    if (!ValidateState(Next)) return false;
    State.Inventory = Inventory;
    OnInventoryChanged.Broadcast();
    return true;
}

bool UICWorldStateSubsystem::SetPlayerTransform(FVector Position, FRotator Rotation)
{
    IC_TRACE_FUNCTION();
    if (Position.ContainsNaN() || Rotation.ContainsNaN()) return false;
    State.PlayerPosition = Position; State.PlayerRotation = Rotation;
    return true;
}

bool UICWorldStateSubsystem::CommitCharacterAndInventory(const FICCharacterState& Character, const FICInventoryState& Inventory)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.CharacterInventory.Request] ID=%s"),*Character.CharacterId.ToString());
    FICWorldState Next = State; Next.Characters.Add(Character.CharacterId, Character); Next.Inventory = Inventory;
    if (!ValidateState(Next)) return false;
    State = MoveTemp(Next);
    OnCharacterChanged.Broadcast(Character.CharacterId); OnInventoryChanged.Broadcast();
    return true;
}

bool UICWorldStateSubsystem::CommitGameplayTransaction(const FICWorldState& Candidate, FGuid TransactionID)
{
    IC_TRACE_FUNCTION();
    if(!TransactionID.IsValid() || State.CompletedTransactionIDs.Contains(TransactionID)){UE_LOG(LogICWorldState, Verbose, TEXT("[State.Transaction.Rejected] Transaction=%s: 무효 또는 이미 처리된 거래"),*TransactionID.ToString());return false;}
    FICWorldState Next=Candidate;Next.CompletedTransactionIDs.Add(TransactionID);
    if(!ValidateState(Next)){UE_LOG(LogICWorldState, Verbose, TEXT("[State.Transaction.Rejected] Transaction=%s: 후보 상태 검증 실패, 원본 유지"),*TransactionID.ToString());return false;}
    State=MoveTemp(Next);
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.Transaction.Committed] Transaction=%s Gold=%d Characters=%d Jobs=%d Drops=%d"),*TransactionID.ToString(),State.Gold,State.Characters.Num(),State.ProductionJobs.Num(),State.Drops.Num());

    TArray<FName> CharacterIDs;State.Characters.GetKeys(CharacterIDs);
    for(FName ID:CharacterIDs)OnCharacterChanged.Broadcast(ID);
    OnInventoryChanged.Broadcast();OnPlayerGoldChanged.Broadcast();OnDropsChanged.Broadcast();
    return true;
}

bool UICWorldStateSubsystem::RegisterStateParticipant(FName ID, UObject* Participant)
{
    IC_TRACE_FUNCTION();
    if (!IsCharacterIdValid(ID) || !IsValid(Participant) || !Participant->GetClass()->ImplementsInterface(UICStateParticipant::StaticClass())){UE_LOG(LogICWorldState, Verbose, TEXT("[State.Participant.Rejected] ID=%s Object=%s: ID/객체/인터페이스 확인 필요"),*ID.ToString(),*GetNameSafe(Participant));return false;}
    if (const auto* Existing = Participants.Find(ID); Existing && Existing->IsValid() && Existing->Get() != Participant){UE_LOG(LogICWorldState, Verbose, TEXT("[State.Participant.Rejected] ID=%s Existing=%s Requested=%s: 중복 참여자"),*ID.ToString(),*GetNameSafe(Existing->Get()),*GetNameSafe(Participant));return false;}
    Participants.Add(ID, Participant);
    UE_LOG(LogICWorldState, Log, TEXT("[State.Participant.Registered] ID=%s Object=%s Count=%d"),*ID.ToString(),*GetNameSafe(Participant),Participants.Num());
    return true;
}

void UICWorldStateSubsystem::UnregisterStateParticipant(FName ID, UObject* Participant)
{
    IC_TRACE_FUNCTION();
    if (const auto* Existing = Participants.Find(ID); Existing && Existing->Get() == Participant){Participants.Remove(ID);UE_LOG(LogICWorldState, Log, TEXT("[State.Participant.Unregistered] ID=%s Object=%s Remaining=%d"),*ID.ToString(),*GetNameSafe(Participant),Participants.Num());}
}

bool UICWorldStateSubsystem::FlushParticipants()
{
    IC_TRACE_FUNCTION();
    const FICWorldState Before = State;
    TArray<FName> OfflineIDs;State.Characters.GetKeys(OfflineIDs);for(FName ID:OfflineIDs)if(!AdvanceOfflineCharacter(ID)){State=Before;return false;}
    for (const auto& Entry : Participants)
    {
        if (Entry.Value.IsValid() && !IICStateParticipant::Execute_FlushToWorldState(Entry.Value.Get(), this))
        {
            UE_LOG(LogICWorldState, Warning, TEXT("[State.Flush.Rollback] ID=%s Object=%s: 활성 상태 반영 실패, 이전 원본 복원"),*Entry.Key.ToString(),*GetNameSafe(Entry.Value.Get()));
            State = Before;
            OnWorldRestored.Broadcast();
            return false;
        }
    }
    if(GetWorld())if(auto* Spawn=GetWorld()->GetSubsystem<UICSpawnSubsystem>())if(!Spawn->FlushActors()){State=Before;return false;}
    if (ValidateState(State)) return true;
    UE_LOG(LogICWorldState, Warning, TEXT("[State.Flush.Rollback] 반영 후 WorldState 검증 실패, 이전 원본 복원"));
    State = Before;
    OnWorldRestored.Broadcast();
    return false;
}

bool UICWorldStateSubsystem::CanRestoreState(const FICWorldState& Candidate) const
{
    IC_TRACE_FUNCTION();
    if (!ValidateState(Candidate)) return false;
    if(GetWorld())if(auto* Spawn=GetWorld()->GetSubsystem<UICSpawnSubsystem>())if(!Spawn->CanRestoreActors(Candidate))return false;
    for (const auto& Entry : Participants)
        if (Entry.Value.IsValid() && !IICStateParticipant::Execute_CanRestoreWorldState(Entry.Value.Get(), Candidate)) return false;
    return true;
}

bool UICWorldStateSubsystem::RestoreState(const FICWorldState& Candidate)
{
    IC_TRACE_FUNCTION();
    if (!CanRestoreState(Candidate)){UE_LOG(LogICWorldState, Verbose, TEXT("[State.Restore.Rejected] Characters=%d Jobs=%d Gold=%d: 규격/활성 참여자 검사 실패"),Candidate.Characters.Num(),Candidate.ProductionJobs.Num(),Candidate.Gold);return false;}
    State = Candidate;
    UE_LOG(LogICWorldState, Log, TEXT("[State.Restored] PlayerCreated=%d Characters=%d Jobs=%d Quests=%d Gold=%d"),State.bPlayerCreated,State.Characters.Num(),State.ProductionJobs.Num(),State.Quests.Num(),State.Gold);
    for (const auto& Entry : Participants)
        if (Entry.Value.IsValid()) IICStateParticipant::Execute_RestoreFromWorldState(Entry.Value.Get(), State);
    OnWorldRestored.Broadcast();
    OnInventoryChanged.Broadcast();
    return true;
}

bool UICWorldStateSubsystem::ApplyWorldProgress(const FICWorldState& Candidate)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICWorldState, Verbose, TEXT("[State.WorldProgress.Request] Quests=%d Jobs=%d Drops=%d"),Candidate.Quests.Num(),Candidate.ProductionJobs.Num(),Candidate.Drops.Num());
    FICWorldState Next = State;
    Next.Quests = Candidate.Quests; Next.UnlockedWarps = Candidate.UnlockedWarps;
    Next.MonsterKilledAt = Candidate.MonsterKilledAt; Next.DefeatedBosses = Candidate.DefeatedBosses;
    Next.ResourceReadyAt = Candidate.ResourceReadyAt; Next.Drops = Candidate.Drops;
    Next.bEndingCleared = Candidate.bEndingCleared; Next.EndingKind = Candidate.EndingKind;
    Next.ProductionJobs = Candidate.ProductionJobs; Next.Crops = Candidate.Crops;
    Next.SpawnSlots = Candidate.SpawnSlots; Next.ResourcePoints = Candidate.ResourcePoints;
    Next.ContainerOwnership = Candidate.ContainerOwnership; Next.StorageInstallations = Candidate.StorageInstallations;
    Next.DiscoveredRegionIDs = Candidate.DiscoveredRegionIDs; Next.DiscoveredAnchorIDs = Candidate.DiscoveredAnchorIDs;
    Next.CodexEntryIDs = Candidate.CodexEntryIDs; Next.UnlockedRecipeIDs = Candidate.UnlockedRecipeIDs;
    Next.ProgressionStages = Candidate.ProgressionStages; Next.Flags = Candidate.Flags;
    Next.CompletedTransactionIDs = Candidate.CompletedTransactionIDs; Next.ProcessedEventIDs = Candidate.ProcessedEventIDs;
    if (!ValidateState(Next)) return false;
    State = MoveTemp(Next);
    return true;
}

bool UICWorldStateSubsystem::SetPlayerGold(int32 Value)
{
    IC_TRACE_FUNCTION();
    FICWorldState Next=State; Next.Gold=Value;
    if(!ValidateState(Next))return false;
    if(State.Gold!=Value){UE_LOG(LogICWorldState, Verbose, TEXT("[State.Gold] Before=%d After=%d"),State.Gold,Value);State.Gold=Value;OnPlayerGoldChanged.Broadcast();}
    return true;
}

bool UICWorldStateSubsystem::AdvanceOfflineCharacter(FName ID)
{
    IC_TRACE_FUNCTION();if(ID==PlayerId())return true;const auto* Participant=Participants.Find(ID);if(Participant&&Participant->IsValid())return true;
    const auto* Stored=State.Characters.Find(ID);if(!Stored)return true;auto* T=GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;if(!T||!D)return true;
    const auto Clock=T->GetClock();if(Clock.PlaySecond<=Stored->LastSimPlaySecond&&Clock.GameMinute<=Stored->LastSimGameMinute)return true;
    auto C=*Stored;if(!ICCombatSimulation::Advance(C,State,D,Clock.PlaySecond,Clock.GameMinute))return false;
    auto Next=State;Next.Characters.Add(ID,C);if(!ValidateState(Next))return false;State=MoveTemp(Next);return true;
}
