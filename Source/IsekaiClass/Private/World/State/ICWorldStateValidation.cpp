#include "World/State/ICWorldStateSubsystem.h"
#include "Debug/ICLog.h"
#include "Life/Survival/ICPlayerSurvival.h"

namespace
{
bool TimeValid(double Value) { IC_TRACE_FUNCTION(); return FMath::IsFinite(Value) && Value >= 0; }
bool NamesValid(const TSet<FName>& Values) { IC_TRACE_FUNCTION(); for (FName ID : Values) if (ID.IsNone()) return false; return true; }
bool GuidsValid(const TSet<FGuid>& Values) { IC_TRACE_FUNCTION(); for (const FGuid& ID : Values) if (!ID.IsValid()) return false; return true; }
bool CombatValid(const FICCombatState& Combat, const FString& Field, FString& Reason)
{
    IC_TRACE_FUNCTION();
    auto Fail = [&Reason](const FString& Path) { IC_TRACE_FUNCTION(); Reason = Path + TEXT(": invalid combat capture.");UE_LOG(LogICWorldState, Verbose, TEXT("[State.Validation.Rejected] %s"),*Reason); return false; };
    if (!TimeValid(Combat.LastCapturedPlaySecond)) return Fail(Field + TEXT(".Combat.LastCapturedPlaySecond"));
    for (const auto& Cooldown : Combat.SkillReadyAtPlaySecond)
        if (Cooldown.Key.IsNone() || !TimeValid(Cooldown.Value)) return Fail(Field + TEXT(".Combat.SkillReadyAtPlaySecond"));
    TSet<FGuid> EffectIDs;
    for (const auto& E : Combat.Effects)
    {
        if (!E.InstanceID.IsValid() || EffectIDs.Contains(E.InstanceID) || E.EffectID.IsNone() || E.Stacks <= 0
            || !TimeValid(E.ExpiresAtPlaySecond) || !TimeValid(E.NextTickPlaySecond) || !TimeValid(E.RemainingShield)) return Fail(Field + TEXT(".Combat.Effects"));
        for (const auto& Magnitude : E.CapturedMagnitudes)
            if (Magnitude.Key.IsNone() || !FMath::IsFinite(Magnitude.Value)) return Fail(Field + TEXT(".Combat.Effects.CapturedMagnitudes"));
        EffectIDs.Add(E.InstanceID);
    }
    return true;
}

}

bool UICWorldStateSubsystem::ValidateStateWithReason(const FICWorldState& Candidate, FString& FailureReason)
{
    IC_TRACE_FUNCTION();
    FailureReason.Reset();
    if (!ValidateAdditionalState(Candidate, FailureReason)) return false;
    if (!ValidateState(Candidate)) { FailureReason = TEXT("WorldState: invalid identity, inventory, resources, party, quest or world-progress fields.");UE_LOG(LogICWorldState, Verbose, TEXT("[State.Validation.Rejected] %s"),*FailureReason); return false; }
    return true;
}

bool UICWorldStateSubsystem::ValidateAdditionalState(const FICWorldState& S, FString& Reason)
{
    IC_TRACE_FUNCTION();
    Reason.Reset();
    auto Fail = [&Reason](const FString& Field) { IC_TRACE_FUNCTION(); Reason = Field + TEXT(": invalid value or inconsistent reference.");UE_LOG(LogICWorldState, Verbose, TEXT("[State.Validation.Rejected] %s"),*Reason); return false; };
    for (const auto& Pair : S.Characters)
    {
        const auto& C = Pair.Value;
        const FString Field = TEXT("Characters.") + Pair.Key.ToString();
        if (static_cast<uint8>(C.LifeState) > static_cast<uint8>(EICCharacterLifeState::Downed)
            || (C.IsDowned() && (C.Kind != EICCharacterKind::Recruitable || C.HP != 0))
            || static_cast<uint8>(C.AttackDisposition) > static_cast<uint8>(EICAttackDisposition::Wait)
            || static_cast<uint8>(C.LowHPDisposition) > static_cast<uint8>(EICLowHPDisposition::Wait)
            || static_cast<uint8>(C.TargetPriority) > static_cast<uint8>(EICTargetPriority::NearbyEnemyFirst)
            || !TimeValid(C.LastSimPlaySecond) || !TimeValid(C.SPDrainElapsedPlaySeconds)
            || C.SPDrainElapsedPlaySeconds >= FPlayerSurvivalState::SPDrainIntervalPlaySeconds
            || C.Position.ContainsNaN() || C.Rotation.ContainsNaN() || !NamesValid(C.UnlockedSkillIDs)) return Fail(Field);
        if (!C.HasStats() && (!C.Combat.Effects.IsEmpty() || !C.Combat.SkillReadyAtPlaySecond.IsEmpty() || !C.UnlockedSkillIDs.IsEmpty())) return Fail(Field + TEXT(".Combat"));
        if (!CombatValid(C.Combat, Field, Reason)) return false;
        for(const auto& Skill:C.EquippedWeaponSkills)
            if(Skill.Key==EICWeaponFamily::None||Skill.Key>EICWeaponFamily::Wand||(!Skill.Value.IsNone()&&!C.UnlockedSkillIDs.Contains(Skill.Value)))
                return Fail(Field+TEXT(".EquippedWeaponSkills"));
    }
    for (const auto& Pair : S.Quests)
    {
        const auto& Q = Pair.Value;
        if (Q.CompletionCount < 0 || !TimeValid(Q.AcceptedAtGameMinute) || !TimeValid(Q.CompletedAtGameMinute)
            || (Q.bRewardClaimed && (!Q.bCompleted || !Q.RewardTransactionID.IsValid() || !S.CompletedTransactionIDs.Contains(Q.RewardTransactionID)))) return Fail(TEXT("Quests.") + Pair.Key.ToString());
    }
    TMap<FGuid, const FPlayerItemInstance*> BagItems;
    for (const auto& Item : S.Inventory.Bag.Slots) if (!Item.IsEmpty()) BagItems.Add(Item.InstanceID, &Item);
    TMap<FGuid, int64> ReservedCounts;
    int64 ReservedGold = 0;
    TSet<FGuid> CompletionIDs;
    for (const auto& Pair : S.ProductionJobs)
    {
        const auto& J = Pair.Value;
        const FString Field = TEXT("ProductionJobs.") + Pair.Key.ToString();
        if (!Pair.Key.IsValid() || Pair.Key != J.JobID || !S.Characters.Contains(J.OwnerCharacterID)
            || J.ProductionID.IsNone() || J.WorkAnchorID.IsNone()
            || static_cast<uint8>(J.Status) > static_cast<uint8>(EICProductionJobStatus::Cancelled)
            || !TimeValid(J.StartedAtPlaySecond) || !TimeValid(J.CompletesAtPlaySecond) || J.CompletesAtPlaySecond < J.StartedAtPlaySecond
            || J.ReservedGold < 0 || !TimeValid(J.ReservedSP)) return Fail(Field);
        const bool Active = J.Status == EICProductionJobStatus::Running || J.Status == EICProductionJobStatus::Ready;
        if (J.bResultsCommitted != (J.Status == EICProductionJobStatus::Completed)
            || (J.bResultsCommitted && (!J.bCostsCommitted || !J.CompletionTransactionID.IsValid() || !S.CompletedTransactionIDs.Contains(J.CompletionTransactionID)))) return Fail(Field + TEXT(".Completion"));
        if (J.CompletionTransactionID.IsValid())
        {
            if (CompletionIDs.Contains(J.CompletionTransactionID)) return Fail(Field + TEXT(".CompletionTransactionID"));
            CompletionIDs.Add(J.CompletionTransactionID);
        }
        if (Active && !J.bCostsCommitted) ReservedGold += J.ReservedGold;
        TSet<FGuid> JobItems;
        for (const auto& R : J.Materials)
        {
            if (!R.ItemInstanceID.IsValid() || R.ItemID.IsNone() || R.Count <= 0 || JobItems.Contains(R.ItemInstanceID)
                || static_cast<uint8>(R.Timing) > static_cast<uint8>(EICProductionMaterialTiming::ConsumeAtStart)) return Fail(Field + TEXT(".Materials"));
            JobItems.Add(R.ItemInstanceID);
            if (Active && (R.Timing == EICProductionMaterialTiming::ConsumeOnSuccess || !J.bCostsCommitted))
            {
                const auto* Found = BagItems.Find(R.ItemInstanceID);
                if (!Found || (*Found)->ItemID != R.ItemID) return Fail(Field + TEXT(".Materials.ItemInstanceID"));
                int64& Total = ReservedCounts.FindOrAdd(R.ItemInstanceID); Total += R.Count;
                if (Total > (*Found)->Count) return Fail(Field + TEXT(".Materials.Count"));
            }
        }
    }
    if (ReservedGold > S.Gold) return Fail(TEXT("ProductionJobs.ReservedGold"));
    for (const auto& Pair : S.Crops)
    {
        const auto& Crop = Pair.Value;
        if (Pair.Key.IsNone() || Crop.CropID.IsNone() || !S.Characters.Contains(Crop.OwnerCharacterID)
            || !TimeValid(Crop.PlantedAtGameMinute) || !TimeValid(Crop.HarvestableAtGameMinute)
            || Crop.HarvestableAtGameMinute < Crop.PlantedAtGameMinute) return Fail(TEXT("Crops.") + Pair.Key.ToString());
    }
    TSet<FString> SpawnKeys;
    for (const auto& Pair : S.SpawnSlots)
    {
        const auto& Slot = Pair.Value;
        const FString Key = Slot.SpawnerID.ToString() + TEXT(":") + FString::FromInt(Slot.SlotIndex);
        if (Pair.Key.IsNone() || Slot.SpawnerID.IsNone() || Slot.MonsterID.IsNone() || Slot.SlotIndex < 0
            || SpawnKeys.Contains(Key) || !TimeValid(Slot.RespawnReadyAtPlaySecond) || !TimeValid(Slot.LastSimPlaySecond)
            || Slot.Position.ContainsNaN() || Slot.Rotation.ContainsNaN()
            || !FMath::IsFinite(Slot.HP) || !FMath::IsFinite(Slot.MP) || !FMath::IsFinite(Slot.MaxHP) || !FMath::IsFinite(Slot.MaxMP)
            || Slot.MaxHP <= 0 || Slot.MaxMP < 0 || Slot.HP < 0 || Slot.HP > Slot.MaxHP || Slot.MP < 0 || Slot.MP > Slot.MaxMP
            || (Slot.bDefeated && Slot.HP != 0)) return Fail(TEXT("SpawnSlots.") + Pair.Key.ToString());
        if (!CombatValid(Slot.Combat, TEXT("SpawnSlots.") + Pair.Key.ToString(), Reason)) return false;
        SpawnKeys.Add(Key);
    }
    for (const auto& Pair : S.ResourcePoints)
        if (Pair.Key.IsNone() || Pair.Value.DefinitionID.IsNone() || Pair.Value.RemainingUses < 0 || !TimeValid(Pair.Value.ReadyAtPlaySecond)) return Fail(TEXT("ResourcePoints.") + Pair.Key.ToString());
    for (const auto& Pair : S.ContainerOwnership)
        if (Pair.Key.IsNone() || !S.Inventory.Storage.Contains(Pair.Key) || Pair.Value.HomeID.IsNone()
            || !IsCharacterIdValid(Pair.Value.OwnerCharacterID)
            || (Pair.Value.bPlayerOwned != (Pair.Value.OwnerCharacterID == PlayerId()))) return Fail(TEXT("ContainerOwnership.") + Pair.Key.ToString());
    TMap<FName, int32> HomeCounts;
    TSet<FName> InstalledContainers;
    // Accept the old fourth installation on load to preserve its contents.
    // New home definitions and the upgrade API are limited to three stages.
    for (const auto& Pair : S.StorageInstallations)
    {
        const auto& Installation = Pair.Value;
        if (Pair.Key.IsNone() || Installation.HomeID.IsNone() || Installation.ProductionID.IsNone()
            || !S.Inventory.Storage.Contains(Installation.ContainerID) || InstalledContainers.Contains(Installation.ContainerID)
            || ++HomeCounts.FindOrAdd(Installation.HomeID) > 4) return Fail(TEXT("StorageInstallations.") + Pair.Key.ToString());
        InstalledContainers.Add(Installation.ContainerID);
    }
    if (!NamesValid(S.DiscoveredRegionIDs) || !NamesValid(S.DiscoveredAnchorIDs) || !NamesValid(S.CodexEntryIDs) || !NamesValid(S.UnlockedRecipeIDs)) return Fail(TEXT("WorldProgress.IDs"));
    if(!NamesValid(S.Codex.AcquiredItemIDs))return Fail(TEXT("Codex.AcquiredItemIDs"));
    for(const auto& Pair:S.Codex.PetEssence)if(Pair.Key<0||Pair.Key>=55||Pair.Value<0)return Fail(TEXT("Codex.PetEssence"));
    for(const auto& Pair:S.Codex.CharacterPets)
    {
        const auto* Character=S.Characters.Find(Pair.Key);
        if(!Character||!Character->HasStats()||Pair.Value<0||Pair.Value>=55||S.Codex.PetEssence.FindRef(Pair.Value)<100)return Fail(TEXT("Codex.CharacterPets"));
    }
    for (const auto& Pair : S.ProgressionStages) if (Pair.Key.IsNone() || Pair.Value < 0) return Fail(TEXT("ProgressionStages"));
    for (const auto& Pair : S.Flags) if (Pair.Key.IsNone()) return Fail(TEXT("Flags"));
    if (!GuidsValid(S.CompletedTransactionIDs) || !GuidsValid(S.ProcessedEventIDs)) return Fail(TEXT("TransactionLedger"));
    return true;
}

bool UICWorldStateSubsystem::ValidateStateClock(const FICWorldState& S, const FICClockState& Clock)
{
    IC_TRACE_FUNCTION();
    if (!Clock.IsValid()) return false;
    for (const auto& Pair : S.Characters)
        if (Pair.Value.LastSimGameMinute > Clock.GameMinute || Pair.Value.LastSimPlaySecond > Clock.PlaySecond || Pair.Value.Combat.LastCapturedPlaySecond > Clock.PlaySecond) return false;
    for (const auto& Pair : S.ProductionJobs) if (Pair.Value.StartedAtPlaySecond > Clock.PlaySecond) return false;
    for (const auto& Pair : S.SpawnSlots) if (Pair.Value.LastSimPlaySecond > Clock.PlaySecond || Pair.Value.Combat.LastCapturedPlaySecond > Clock.PlaySecond) return false;
    for (const auto& Pair : S.Crops) if (Pair.Value.PlantedAtGameMinute > Clock.GameMinute) return false;
    for (const auto& Drop : S.Drops) if (Drop.CreatedPlaySecond > Clock.PlaySecond) return false;
    for (const auto& Pair : S.Quests)
        if (Pair.Value.AcceptedAtGameMinute > Clock.GameMinute || Pair.Value.CompletedAtGameMinute > Clock.GameMinute) return false;
    return true;
}

double UICWorldStateSubsystem::GetProductionRemainingSeconds(FGuid JobID, double PlaySecond) const
{
    IC_TRACE_FUNCTION();
    const auto* Job = State.ProductionJobs.Find(JobID);
    if (!Job || !TimeValid(PlaySecond)) return -1;
    if (Job->Status == EICProductionJobStatus::Completed || Job->Status == EICProductionJobStatus::Cancelled) return 0;
    return FMath::Max(0.0, Job->CompletesAtPlaySecond - PlaySecond);
}
