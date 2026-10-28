#include "Data/Validation/ICDefinitionLibrary.h"
#include "Debug/ICLog.h"
#include "Data/Rows/Character/ICCharacterRows.h"
#include "Data/Rows/Combat/ICSkillRows.h"
#include "Data/Rows/Combat/ICStatusEffectRow.h"
#include "Data/Rows/Combat/ICSynergyRow.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "UObject/UnrealType.h"

namespace ICDefinitionValidation
{
namespace
{
struct FLocation
{
    FString Table;
    FName Row;
};

void Issue(TArray<FICDefinitionIssue>& Issues, const FLocation& At, const FString& Field,
    const FString& Reason, EICDefinitionSeverity Severity = EICDefinitionSeverity::Error)
{
    IC_TRACE_FUNCTION();
    FICDefinitionIssue& Result = Issues.AddDefaulted_GetRef();
    Result.Severity = Severity;
    Result.Table = At.Table;
    Result.Row = At.Row;
    Result.Field = Field;
    Result.Reason = Reason;
}

void Missing(TArray<FICDefinitionIssue>& Issues, const FLocation& At, const FString& Field,
    const FString& Reason, bool bComplete)
{
    IC_TRACE_FUNCTION();
    Issue(Issues, At, Field, Reason, bComplete ? EICDefinitionSeverity::Error : EICDefinitionSeverity::Warning);
}

template<typename TRow, typename TFunction>
void EachRow(const TArray<FICDefinitionTable>& Tables, TFunction Function)
{
    IC_TRACE_FUNCTION();
    for (const FICDefinitionTable& DefinitionTable : Tables)
    {
        const UDataTable* Table = DefinitionTable.Table;
        if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(TRow::StaticStruct()))
        {
            continue;
        }
        for (const TPair<FName, uint8*>& Entry : Table->GetRowMap())
        {
            const TRow* Row = reinterpret_cast<const TRow*>(Entry.Value);
            if (Row && !Row->bDeprecated && !Row->bTestOnly)
            {
                Function(*Row, FLocation{Table->GetPathName(), Entry.Key});
            }
        }
    }
}

bool IsPrimaryStat(EICStat Stat)
{
    IC_TRACE_FUNCTION();
    return Stat >= EICStat::STR && Stat <= EICStat::LUK;
}

void ValidateMagnitude(const FICCombatMagnitude& Magnitude, const FLocation& At,
    const FString& Field, bool bRequired, bool bComplete, TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    if (Magnitude.Coefficient > 0.0 && Magnitude.SourceStat == EICStat::None)
    {
        Issue(Issues, At, Field + TEXT(".SourceStat"), TEXT("A positive coefficient requires a source statistic."));
    }
    if (bRequired && Magnitude.Coefficient == 0.0 && Magnitude.FlatAmount == 0.0)
    {
        Missing(Issues, At, Field, TEXT("An executable magnitude requires a positive coefficient or flat amount."), bComplete);
    }
}

void ValidateModifiers(const TArray<FICStatModifier>& Modifiers, const FLocation& At,
    const FString& Field, bool bForbidSP, TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    TSet<EICStat> Seen;
    for (int32 Index = 0; Index < Modifiers.Num(); ++Index)
    {
        const FICStatModifier& Modifier = Modifiers[Index];
        const FString Element = FString::Printf(TEXT("%s[%d]"), *Field, Index);
        if (Modifier.Stat == EICStat::None)
        {
            Issue(Issues, At, Element + TEXT(".Stat"), TEXT("A modifier requires an explicit statistic."));
        }
        if (bForbidSP && Modifier.Stat == EICStat::MaxSP)
        {
            Issue(Issues, At, Element + TEXT(".Stat"), TEXT("Party synergy must not introduce SP modifiers for companions."));
        }
        if (Seen.Contains(Modifier.Stat))
        {
            Issue(Issues, At, Element + TEXT(".Stat"), TEXT("Prefer combining flat and percent changes for a statistic into one modifier."), EICDefinitionSeverity::Warning);
        }
        Seen.Add(Modifier.Stat);
    }
}

FString ExecutionSignature(const UStruct* Struct, const void* Data, const TSet<FName>& Skip)
{
    IC_TRACE_FUNCTION();
    FString Signature;
    for (TFieldIterator<FProperty> It(Struct); It; ++It)
    {
        const FProperty* Property = *It;
        if (Skip.Contains(Property->GetFName())) continue;
        FString Value;
        Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Data), nullptr, nullptr, PPF_None);
        Signature += FString::Printf(TEXT("%s:%d:%s;"), *Property->GetName(), Value.Len(), *Value);
    }
    return Signature;
}
}

void ValidateCombat(const TArray<FICDefinitionTable>& Tables, bool bComplete, TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    TMap<FName, const FICSkillRow*> Skills;
    EachRow<FICSkillRow>(Tables, [&Skills](const FICSkillRow& Row, const FLocation&) { IC_TRACE_FUNCTION(); Skills.Add(Row.ID, &Row); });
    TMap<FName, const FICClassRow*> Classes;
    EachRow<FICClassRow>(Tables, [&Classes](const FICClassRow& Row, const FLocation&) { IC_TRACE_FUNCTION(); Classes.Add(Row.ID, &Row); });
    TMap<FName, const FICMonsterRow*> Monsters;
    EachRow<FICMonsterRow>(Tables, [&Monsters](const FICMonsterRow& Row, const FLocation&) { IC_TRACE_FUNCTION(); Monsters.Add(Row.ID, &Row); });
    TMap<FName, const FICBossPatternRow*> Patterns;
    EachRow<FICBossPatternRow>(Tables, [&Patterns](const FICBossPatternRow& Row, const FLocation&) { IC_TRACE_FUNCTION(); Patterns.Add(Row.ID, &Row); });
    TMap<FName, const FICSkillEffectRow*> SkillEffects;
    EachRow<FICSkillEffectRow>(Tables, [&SkillEffects](const FICSkillEffectRow& Row, const FLocation&) { IC_TRACE_FUNCTION(); SkillEffects.Add(Row.ID, &Row); });
    TMap<FName, const FICStatusEffectRow*> StatusEffects;
    EachRow<FICStatusEffectRow>(Tables, [&StatusEffects](const FICStatusEffectRow& Row, const FLocation&) { IC_TRACE_FUNCTION(); StatusEffects.Add(Row.ID, &Row); });
    TMap<FString, FName> UniqueSignatures;
    const TSet<FName> DefinitionMetadata = {TEXT("ID"), TEXT("NameKey"), TEXT("DescriptionKey"), TEXT("Tags"), TEXT("DataVersion"), TEXT("bDeprecated"), TEXT("bTestOnly"), TEXT("Presentation"), TEXT("SourceDefinitions"), TEXT("ExpiryWarningPlaySeconds")};

    EachRow<FICClassRow>(Tables, [&](const FICClassRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        EICWeaponFamily ExpectedWeapon = EICWeaponFamily::None;
        EICDamageType ExpectedDamage = EICDamageType::Physical;
        switch (Row.CharacterClass)
        {
        case EICCharacterClass::Warrior: ExpectedWeapon = EICWeaponFamily::Sword; break;
        case EICCharacterClass::Archer: ExpectedWeapon = EICWeaponFamily::Bow; break;
        case EICCharacterClass::Mage: ExpectedWeapon = EICWeaponFamily::Staff; ExpectedDamage = EICDamageType::Magical; break;
        case EICCharacterClass::Healer: ExpectedWeapon = EICWeaponFamily::Wand; ExpectedDamage = EICDamageType::Magical; break;
        default: Missing(Issues, At, TEXT("CharacterClass"), TEXT("Choose one of the four fixed character classes."), bComplete); break;
        }
        if (ExpectedWeapon != EICWeaponFamily::None && Row.WeaponFamily != ExpectedWeapon)
        {
            Issue(Issues, At, TEXT("WeaponFamily"), TEXT("Weapon family must match the fixed class weapon."));
        }
        if (ExpectedWeapon != EICWeaponFamily::None && Row.NormalAttackDamageType != ExpectedDamage)
        {
            Issue(Issues, At, TEXT("NormalAttackDamageType"), TEXT("Sword/bow attacks are physical; staff/wand attacks are magical."));
        }
        const FName Slots[] = {Row.SkillSlot1ID, Row.SkillSlot2ID, Row.SkillSlot3ID, Row.SkillSlot4ID};
        TSet<FName> Seen;
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
        {
            const FString Field = FString::Printf(TEXT("SkillSlot%dID"), Index + 1);
            if (Slots[Index].IsNone())
            {
                Missing(Issues, At, Field, TEXT("Each class requires four authored skills."), bComplete);
                continue;
            }
            if (Seen.Contains(Slots[Index])) Issue(Issues, At, Field, TEXT("Class skills must be unique across four slots."));
            Seen.Add(Slots[Index]);
            if (const FICSkillRow* const* Skill = Skills.Find(Slots[Index]))
            {
                if ((*Skill)->OwnerType != EICSkillOwner::CharacterClass || (*Skill)->ClassID != Row.ID || (*Skill)->ClassSlotIndex != Index + 1)
                {
                    Issue(Issues, At, Field, TEXT("Skill ownership, class ID and slot must point back to this class slot."));
                }
                if (Index > 0 && !(*Skill)->PreviousSkillID.IsNone() && (*Skill)->PreviousSkillID != Slots[Index - 1])
                {
                    Issue(Issues, At, Field, TEXT("Sequential class unlocking must reference the preceding slot."));
                }
            }
        }
        if (Row.NormalAttackStages.IsEmpty())
        {
            Missing(Issues, At, TEXT("NormalAttackStages"), TEXT("Author exactly three normal attack stages."), bComplete);
        }
        else if (Row.NormalAttackStages.Num() != 3)
        {
            Issue(Issues, At, TEXT("NormalAttackStages"), TEXT("All four weapon families use exactly three attack stages."));
        }
        for (int32 Index = 0; Index < Row.NormalAttackStages.Num(); ++Index)
        {
            const FICNormalAttackStage& Stage = Row.NormalAttackStages[Index];
            const FString Field = FString::Printf(TEXT("NormalAttackStages[%d]"), Index);
            if (Stage.GuardDodgeCancelEndPlaySeconds < Stage.GuardDodgeCancelStartPlaySeconds)
                Issue(Issues, At, Field, TEXT("Attack cancel window end must not precede its start."));
            if (Stage.Hit.Shape == EICHitShape::Projectile && (Stage.Hit.RepeatCount != 1 || Stage.Hit.bPierceTargets || Stage.Hit.MaximumTargets != 1 || !Stage.Hit.bStopAtWorldCollision))
                Issue(Issues, At, Field + TEXT(".Hit"), TEXT("Normal ranged attacks emit one projectile ending on its first valid target or obstacle."));
            if (bComplete && Stage.Montage.IsNull())
                Issue(Issues, At, Field + TEXT(".Montage"), TEXT("A complete normal attack requires an authored montage."));
        }
    });

    EachRow<FICPlayerPresetRow>(Tables, [&](const FICPlayerPresetRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        if (bComplete && Row.Mesh.IsNull()) Issue(Issues, At, TEXT("Mesh"), TEXT("A complete human appearance preset requires its actual mesh."));
        if (bComplete && Row.AnimationClass.IsNull()) Issue(Issues, At, TEXT("AnimationClass"), TEXT("A complete preset requires an animation class."));
    });

    EachRow<FICRecruitCombatRow>(Tables, [&](const FICRecruitCombatRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        const int32 Stats[] = {Row.InitialBaseStats.STR, Row.InitialBaseStats.DEX, Row.InitialBaseStats.INT,
            Row.InitialBaseStats.CON, Row.InitialBaseStats.CHA, Row.InitialBaseStats.LUK};
        const TCHAR* Names[] = {TEXT("STR"), TEXT("DEX"), TEXT("INT"), TEXT("CON"), TEXT("CHA"), TEXT("LUK")};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Stats); ++Index)
        {
            if (Stats[Index] < 1 || Stats[Index] > 10000)
                Issue(Issues, At, FString(TEXT("InitialBaseStats.")) + Names[Index], TEXT("Initial primary stats must be integers in [1, 10000]."));
        }
        if (Row.NPCID.IsNone()) Missing(Issues, At, TEXT("NPCID"), TEXT("A recruit combat definition requires its NPC ID."), bComplete);
        if (Row.ClassID.IsNone()) Missing(Issues, At, TEXT("ClassID"), TEXT("A recruit requires a combat class definition."), bComplete);
        if (Row.WeaponFamily == EICWeaponFamily::None) Missing(Issues, At, TEXT("WeaponFamily"), TEXT("A recruit requires its fixed weapon family."), bComplete);
        if (const FICClassRow* const* Class = Classes.Find(Row.ClassID))
        {
            if (Row.WeaponFamily != (*Class)->WeaponFamily) Issue(Issues, At, TEXT("WeaponFamily"), TEXT("Recruit weapon family must match the referenced combat class."));
        }
        if (Row.UniqueSkillID.IsNone()) Missing(Issues, At, TEXT("UniqueSkillID"), TEXT("Each recruit requires one authored unique skill."), bComplete);
        if (const FICSkillRow* const* Skill = Skills.Find(Row.UniqueSkillID))
        {
            if ((*Skill)->OwnerType != EICSkillOwner::RecruitUnique || (*Skill)->NPCID != Row.NPCID)
                Issue(Issues, At, TEXT("UniqueSkillID"), TEXT("Unique skill ownership must point back to this recruit's NPC."));
        }
        const FName Equipment[] = {Row.InitialEquipment.WeaponID};
        const TCHAR* EquipmentFields[] = {TEXT("WeaponID")};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Equipment); ++Index)
        {
            if (Index==0 && Equipment[Index].IsNone()) Missing(Issues, At, FString(TEXT("InitialEquipment.")) + EquipmentFields[Index], TEXT("Author the recruit's initial weapon."), bComplete);
        }
    });

    EachRow<FICSkillRow>(Tables, [&](const FICSkillRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        if (Row.OwnerType == EICSkillOwner::CharacterClass)
        {
            if (Row.ClassID.IsNone()) Missing(Issues, At, TEXT("ClassID"), TEXT("A class skill requires its owning class."), bComplete);
            if (Row.ClassSlotIndex < 1 || Row.ClassSlotIndex > 4) Issue(Issues, At, TEXT("ClassSlotIndex"), TEXT("Class skills occupy fixed slots 1 through 4."));
            if (!Row.NPCID.IsNone() || !Row.MonsterID.IsNone()) Issue(Issues, At, TEXT("OwnerType"), TEXT("Class skill owner must not also be an NPC or monster."));
            if (Row.ClassSlotIndex == 1 && !Row.bInitiallyUnlocked) Issue(Issues, At, TEXT("bInitiallyUnlocked"), TEXT("The first class skill is available at new game start."));
            if (Row.ClassSlotIndex > 1 && Row.bInitiallyUnlocked) Issue(Issues, At, TEXT("bInitiallyUnlocked"), TEXT("Later class skills must unlock sequentially from base primary stats."));
            if (Row.ClassSlotIndex == 1 && !Row.PreviousSkillID.IsNone()) Issue(Issues, At, TEXT("PreviousSkillID"), TEXT("The first class slot cannot require a preceding skill."));
            if (Row.ClassSlotIndex > 1 && Row.PreviousSkillID.IsNone()) Missing(Issues, At, TEXT("PreviousSkillID"), TEXT("Later class slots require the preceding skill."), bComplete);
            if (Row.ClassSlotIndex > 1 && Row.UnlockConditions.Conditions.IsEmpty()) Missing(Issues, At, TEXT("UnlockConditions"), TEXT("Author the base-stat unlock thresholds."), bComplete);
            for (int32 Index = 0; Index < Row.UnlockConditions.Conditions.Num(); ++Index)
            {
                const FICCondition& Condition = Row.UnlockConditions.Conditions[Index];
                if (Condition.Type != EICConditionType::Stat || !IsPrimaryStat(Condition.Stat) || Condition.bUseEffectiveStats || Condition.bAllowCurrentState)
                    Issue(Issues, At, FString::Printf(TEXT("UnlockConditions.Conditions[%d]"), Index), TEXT("Class unlock conditions must use base primary stats without equipment, buffs or current-state values."));
            }
        }
        else
        {
            if (Row.ClassSlotIndex != 0) Issue(Issues, At, TEXT("ClassSlotIndex"), TEXT("Only player class skills occupy class slots."));
            if (Row.OwnerType == EICSkillOwner::RecruitUnique && Row.NPCID.IsNone()) Missing(Issues, At, TEXT("NPCID"), TEXT("A unique skill requires its recruit NPC owner."), bComplete);
            if (Row.OwnerType == EICSkillOwner::Monster && Row.MonsterID.IsNone()) Missing(Issues, At, TEXT("MonsterID"), TEXT("A monster skill requires its monster owner."), bComplete);
            if (Row.OwnerType == EICSkillOwner::RecruitUnique && !Row.MonsterID.IsNone()) Issue(Issues, At, TEXT("MonsterID"), TEXT("A recruit unique skill cannot also belong to a monster."));
            if (Row.OwnerType == EICSkillOwner::Monster && !Row.NPCID.IsNone()) Issue(Issues, At, TEXT("NPCID"), TEXT("A monster skill cannot also belong to a recruit NPC."));
        }
        TSet<FName> Chain;
        FName Cursor = Row.ID;
        while (const FICSkillRow* const* Current = Skills.Find(Cursor))
        {
            if (Chain.Contains(Cursor))
            {
                Issue(Issues, At, TEXT("PreviousSkillID"), TEXT("Previous-skill references form a cycle, including a possible self-reference."));
                break;
            }
            Chain.Add(Cursor);
            Cursor = (*Current)->PreviousSkillID;
            if (Cursor.IsNone()) break;
        }
        if (Row.TargetRules.MaximumRangeCm < Row.TargetRules.MinimumRangeCm) Issue(Issues, At, TEXT("TargetRules.MaximumRangeCm"), TEXT("Maximum target range must be at least the minimum range."));
        if (Row.Purpose == EICSkillPurpose::Revive && (Row.TargetRules.TargetState != EICCombatTargetState::Downed || !Row.TargetRules.bIgnoreRangeForDowned))
            Issue(Issues, At, TEXT("TargetRules"), TEXT("Revival must allow only Downed targets and bypass range checks."));
        if (Row.FirstEffectOffsetPlaySeconds > Row.CastPlaySeconds) Issue(Issues, At, TEXT("FirstEffectOffsetPlaySeconds"), TEXT("First effect commit must occur within the cast/preparation span."));
        if (Row.Hits.IsEmpty()) Missing(Issues, At, TEXT("Hits"), TEXT("A usable skill must reference at least one actual effect emission."), bComplete);
        for (int32 Index = 0; Index < Row.Hits.Num(); ++Index)
        {
            const FICSkillHitDefinition& Hit = Row.Hits[Index];
            const FString Field = FString::Printf(TEXT("Hits[%d]"), Index);
            if (Hit.EffectID.IsNone()) Missing(Issues, At, Field + TEXT(".EffectID"), TEXT("Each emission requires its actual effect definition."), bComplete);
            if (Hit.StartOffsetPlaySeconds < Row.FirstEffectOffsetPlaySeconds)
                Issue(Issues, At, Field + TEXT(".StartOffsetPlaySeconds"), TEXT("Hit emission cannot precede the first effect/cost commit."));
            const double LastEmission = Hit.StartOffsetPlaySeconds + FMath::Max(0, Hit.RepeatCount - 1) * Hit.RepeatIntervalPlaySeconds;
            if (LastEmission > Row.CastPlaySeconds + Row.RecoveryPlaySeconds)
                Issue(Issues, At, Field + TEXT(".StartOffsetPlaySeconds"), TEXT("All authored emissions must finish within CastPlaySeconds + RecoveryPlaySeconds; projectile flight may continue afterward."));
            if (Hit.RepeatCount > 1 && Hit.RepeatIntervalPlaySeconds <= 0.0)
                Issue(Issues, At, Field + TEXT(".RepeatIntervalPlaySeconds"), TEXT("Repeated emissions require a positive interval."));
            if(Hit.Shape==EICHitShape::WeaponTrace)
            {
                if(Hit.TraceStartSocket.IsNone()||Hit.TraceEndSocket.IsNone())Issue(Issues,At,Field+TEXT(".TraceStartSocket"),TEXT("Weapon trace requires both actual weapon socket names."));
                if(!FMath::IsFinite(Hit.TraceRadiusCm)||Hit.TraceRadiusCm<=0)Issue(Issues,At,Field+TEXT(".TraceRadiusCm"),TEXT("Weapon trace radius must be finite and positive in centimeters."));
                if(!FMath::IsFinite(Hit.TraceDurationPlaySeconds)||Hit.TraceDurationPlaySeconds<=0)Issue(Issues,At,Field+TEXT(".TraceDurationPlaySeconds"),TEXT("Weapon trace duration must be finite and positive in PlayClock seconds."));
            }
            if (Hit.Shape == EICHitShape::Projectile && bComplete && Hit.ProjectileClass.IsNull())
                Issue(Issues, At, Field + TEXT(".ProjectileClass"), TEXT("A complete projectile emission requires its actual actor class."));
        }
        if (bComplete && Row.Montage.IsNull()) Issue(Issues, At, TEXT("Montage"), TEXT("A complete skill requires the actual animation montage."));


        if (Row.OwnerType == EICSkillOwner::RecruitUnique && !Row.Hits.IsEmpty())
        {
            bool bResolved = true;
            FString Signature = FString::Printf(TEXT("Purpose=%d;"), static_cast<int32>(Row.Purpose));
            Signature += ExecutionSignature(FICSkillTargetRules::StaticStruct(), &Row.TargetRules, {});
            for (const FICSkillHitDefinition& Hit : Row.Hits)
            {
                const FICSkillEffectRow* const* Effect = SkillEffects.Find(Hit.EffectID);
                if (!Effect) { bResolved = false; break; }
                Signature += ExecutionSignature(FICSkillHitDefinition::StaticStruct(), &Hit, {TEXT("EffectID"), TEXT("ProjectileClass"), TEXT("MontageSection"), TEXT("NotifyName")});
                TSet<FName> EffectSkip = DefinitionMetadata;
                EffectSkip.Add(TEXT("AppliedStatusEffectIDs"));
                EffectSkip.Add(TEXT("DispellableStatusEffectIDs"));
                Signature += ExecutionSignature(FICSkillEffectRow::StaticStruct(), *Effect, EffectSkip);
                auto AppendStatuses = [&](const TArray<FName>& IDs, const TCHAR* Prefix)
                {
                    IC_TRACE_FUNCTION();
                    TArray<FString> Payloads;
                    for (FName StatusID : IDs)
                    {
                        if (const FICStatusEffectRow* const* Status = StatusEffects.Find(StatusID))
                            Payloads.Add(ExecutionSignature(FICStatusEffectRow::StaticStruct(), *Status, DefinitionMetadata));
                        else bResolved = false;
                    }
                    Payloads.Sort();
                    Signature += Prefix;
                    for (const FString& Payload : Payloads) Signature += FString::Printf(TEXT("%d:%s;"), Payload.Len(), *Payload);
                };
                AppendStatuses((*Effect)->AppliedStatusEffectIDs, TEXT("Applied="));
                AppendStatuses((*Effect)->DispellableStatusEffectIDs, TEXT("Dispel="));
            }
            if (bResolved)
            {
                if (const FName* Duplicate = UniqueSignatures.Find(Signature))
                    Issue(Issues, At, TEXT("Hits"), FString::Printf(TEXT("Unique skill has the same effect/target/hit payload as %s; names, owners, costs and presentation do not distinguish final effects."), *Duplicate->ToString()), bComplete ? EICDefinitionSeverity::Error : EICDefinitionSeverity::Warning);
                else UniqueSignatures.Add(Signature, Row.ID);
            }
        }
    });

    EachRow<FICSkillEffectRow>(Tables, [&](const FICSkillEffectRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        const bool bNeedsMagnitude = Row.Kind == EICSkillEffectKind::Damage || Row.Kind == EICSkillEffectKind::Heal || Row.Kind == EICSkillEffectKind::Shield;
        ValidateMagnitude(Row.Magnitude, At, TEXT("Magnitude"), bNeedsMagnitude, bComplete, Issues);
        if (Row.Kind == EICSkillEffectKind::ApplyStatus && Row.AppliedStatusEffectIDs.IsEmpty()) Missing(Issues, At, TEXT("AppliedStatusEffectIDs"), TEXT("ApplyStatus requires an authored status effect."), bComplete);
        if (Row.Kind == EICSkillEffectKind::Dispel && Row.DispellableStatusEffectIDs.IsEmpty() && Row.DispellableEffectTags.IsEmpty()) Missing(Issues, At, TEXT("DispellableStatusEffectIDs"), TEXT("Dispel requires effect IDs or a dispel tag filter."), bComplete);
        if (Row.Kind == EICSkillEffectKind::Revive && Row.ReviveHPRatio <= 0.0) Missing(Issues, At, TEXT("ReviveHPRatio"), TEXT("Revival requires a positive HP recovery ratio."), bComplete);
        if (Row.Kind == EICSkillEffectKind::Movement && (Row.MovementDistanceCm <= 0.0 || Row.MovementPlaySeconds <= 0.0)) Missing(Issues, At, TEXT("MovementDistanceCm"), TEXT("Movement effects require authored distance and duration."), bComplete);
        if (Row.Kind == EICSkillEffectKind::Custom && !Row.CustomExecutionTag.IsValid()) Missing(Issues, At, TEXT("CustomExecutionTag"), TEXT("Custom effects require an explicitly implemented execution key."), bComplete);
    });

    EachRow<FICStatusEffectRow>(Tables, [&](const FICStatusEffectRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        if (Row.bPermanent && Row.PermanentEndConditions.Conditions.IsEmpty()) Missing(Issues, At, TEXT("PermanentEndConditions"), TEXT("Permanent effects require an explicit end condition."), bComplete);
        if (!Row.bPermanent && Row.DurationPlaySeconds <= 0.0) Missing(Issues, At, TEXT("DurationPlaySeconds"), TEXT("Timed effects require a positive PlayClock duration."), bComplete);
        const bool bPeriodic = Row.Kind == EICStatusKind::DamageOverTime || Row.Kind == EICStatusKind::HealOverTime;
        if (bPeriodic && Row.TickIntervalPlaySeconds <= 0.0) Issue(Issues, At, TEXT("TickIntervalPlaySeconds"), TEXT("Periodic effects require a positive tick interval."));
        if (bPeriodic) ValidateMagnitude(Row.PeriodicMagnitude, At, TEXT("PeriodicMagnitude"), true, bComplete, Issues);
        if (Row.Kind == EICStatusKind::Shield) ValidateMagnitude(Row.InitialShieldMagnitude, At, TEXT("InitialShieldMagnitude"), true, bComplete, Issues);
        if (Row.Kind == EICStatusKind::StatModifier && Row.StatModifiers.IsEmpty()) Missing(Issues, At, TEXT("StatModifiers"), TEXT("Stat modifier effects require at least one authored modifier."), bComplete);
        ValidateModifiers(Row.StatModifiers, At, TEXT("StatModifiers"), false, Issues);
        if (Row.Kind == EICStatusKind::Control && Row.GrantedStateTags.IsEmpty()) Missing(Issues, At, TEXT("GrantedStateTags"), TEXT("Control effects require an explicit state tag."), bComplete);
        if (Row.bHarmful && !Row.bRemovedByLivingTargetSleep) Issue(Issues, At, TEXT("bRemovedByLivingTargetSleep"), TEXT("Harmful effects must be removable by sleep for living targets."));
        if (Row.Kind == EICStatusKind::Custom && !Row.CustomExecutionTag.IsValid()) Missing(Issues, At, TEXT("CustomExecutionTag"), TEXT("Custom status effects require an implemented execution key."), bComplete);
    });

    EachRow<FICSynergyRow>(Tables, [&](const FICSynergyRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.RequiredWeaponFamily==EICWeaponFamily::None&&Row.MinimumMembers==0)
        {
            if (Row.MemberNPCIDs.IsEmpty()) Missing(Issues, At, TEXT("MemberNPCIDs"), TEXT("Author distinct recruit NPC IDs."), bComplete);
            else if (Row.MemberNPCIDs.Num() < 2 || Row.MemberNPCIDs.Num() > 4) Issue(Issues, At, TEXT("MemberNPCIDs"), TEXT("Legacy synergy combinations contain two to four NPCs."));
        }
        if(Row.MinimumMembers<0||Row.MinimumMembers>4||(Row.RequiredWeaponFamily!=EICWeaponFamily::None&&Row.MinimumMembers<1)||(Row.RequiredWeaponFamily==EICWeaponFamily::None&&Row.MinimumMembers>Row.MemberNPCIDs.Num()))Issue(Issues,At,TEXT("MinimumMembers"),TEXT("Invalid party member threshold."));
        TSet<FName> Seen;
        for (int32 Index = 0; Index < Row.MemberNPCIDs.Num(); ++Index)
        {
            const FName Member = Row.MemberNPCIDs[Index];
            const FString Field = FString::Printf(TEXT("MemberNPCIDs[%d]"), Index);
            if (Member.IsNone()) Missing(Issues, At, Field, TEXT("A synergy member requires its NPC ID."), bComplete);
            else if (Seen.Contains(Member)) Issue(Issues, At, Field, TEXT("A synergy combination cannot include the same NPC twice."));
            Seen.Add(Member);
        }
        if (Row.StatModifiers.IsEmpty()) Missing(Issues, At, TEXT("StatModifiers"), TEXT("A synergy requires at least one actual modifier."), bComplete);
        ValidateModifiers(Row.StatModifiers, At, TEXT("StatModifiers"), true, Issues);
        if (bComplete && Row.Icon.IsNull()) Issue(Issues, At, TEXT("Icon"), TEXT("A complete synergy requires its HUD icon."));
    });

    EachRow<FICMonsterRow>(Tables, [&](const FICMonsterRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        if (Row.CombatStats.MaxHP < 1) Issue(Issues, At, TEXT("CombatStats.MaxHP"), TEXT("Monsters use positive direct HP instead of player primary stats."));
        if (Row.DetectionRangeCm <= 0.0 || Row.ChaseLimitCm <= 0.0
            || Row.InvestigationAdvanceDistanceCm < 0.0
            || Row.AttackRangeCm <= 0.0 || Row.AttackIntervalPlaySeconds <= 0.0)
            Missing(Issues, At, TEXT("DetectionRangeCm"), TEXT("Author valid detection, chase, investigation, and attack ranges plus attack interval."), bComplete);
        if (Row.Kind != EICMonsterKind::Normal && !Row.bImmuneToOrdinaryHitStagger) Issue(Issues, At, TEXT("bImmuneToOrdinaryHitStagger"), TEXT("Bosses are immune to ordinary hit stagger; casting interruption remains separate."));
        TSet<int32> PhaseIndices;
        double PreviousThreshold = 1.0;
        for (int32 Index = 0; Index < Row.BossPhases.Num(); ++Index)
        {
            const FICBossPhaseDefinition& Phase = Row.BossPhases[Index];
            const FString Field = FString::Printf(TEXT("BossPhases[%d]"), Index);
            if (PhaseIndices.Contains(Phase.PhaseIndex)) Issue(Issues, At, Field + TEXT(".PhaseIndex"), TEXT("Boss phase indices must be unique."));
            PhaseIndices.Add(Phase.PhaseIndex);
            if (Phase.bUseHPThreshold)
            {
                if (Phase.EnterAtOrBelowHPRatio > PreviousThreshold) Issue(Issues, At, Field + TEXT(".EnterAtOrBelowHPRatio"), TEXT("HP phase threshold increases; confirm that this is intentional for this boss."), EICDefinitionSeverity::Warning);
                PreviousThreshold = Phase.EnterAtOrBelowHPRatio;
            }
            if (Phase.Patterns.IsEmpty()) Missing(Issues, At, Field + TEXT(".Patterns"), TEXT("Author the boss phase's actual patterns."), bComplete);
            TSet<FName> SeenPatterns;
            double WeightSum = 0.0;
            for (int32 PatternIndex = 0; PatternIndex < Phase.Patterns.Num(); ++PatternIndex)
            {
                const FICBossPatternChoice& Choice = Phase.Patterns[PatternIndex];
                WeightSum += Choice.Weight;
                if (!Choice.PatternID.IsNone() && SeenPatterns.Contains(Choice.PatternID)) Issue(Issues, At, Field + TEXT(".Patterns"), TEXT("Do not duplicate a pattern within one weighted phase pool."));
                SeenPatterns.Add(Choice.PatternID);
                if (const FICBossPatternRow* const* Pattern = Patterns.Find(Choice.PatternID))
                {
                    if ((*Pattern)->MonsterID != Row.ID) Issue(Issues, At, Field + TEXT(".Patterns"), TEXT("Referenced boss pattern must belong to this monster."));
                }
            }
            if (!Phase.Patterns.IsEmpty() && WeightSum <= 0.0) Issue(Issues, At, Field + TEXT(".Patterns"), TEXT("A weighted pattern pool requires a positive total weight."));
        }
        if (Row.Kind != EICMonsterKind::Normal && Row.BossPhases.IsEmpty()) Missing(Issues, At, TEXT("BossPhases"), TEXT("Author the boss's phase and pattern definitions."), bComplete);
        if (bComplete && Row.PawnClass.IsNull() && Row.Mesh.IsNull()) Issue(Issues, At, TEXT("PawnClass"), TEXT("A complete monster requires an actual pawn class or mesh."));
    });

    EachRow<FICBossPatternRow>(Tables, [&](const FICBossPatternRow& Row, const FLocation& At)
    {
        IC_TRACE_FUNCTION();
        if (Row.MonsterID.IsNone()) Missing(Issues, At, TEXT("MonsterID"), TEXT("A boss pattern requires its owning monster."), bComplete);
        if (const FICMonsterRow* const* Monster = Monsters.Find(Row.MonsterID))
        {
            if ((*Monster)->Kind == EICMonsterKind::Normal) Issue(Issues, At, TEXT("MonsterID"), TEXT("Boss patterns must reference a regional or final boss."));
        }
        if (Row.MaximumRangeCm < Row.MinimumRangeCm) Issue(Issues, At, TEXT("MaximumRangeCm"), TEXT("Maximum pattern range must be at least its minimum."));
        if (Row.TelegraphPlaySeconds > Row.DurationPlaySeconds) Issue(Issues, At, TEXT("TelegraphPlaySeconds"), TEXT("Pattern telegraph must fit within the complete pattern span."));
        if (Row.Steps.IsEmpty()) Missing(Issues, At, TEXT("Steps"), TEXT("A boss pattern requires actual skill steps."), bComplete);
        for (int32 Index = 0; Index < Row.Steps.Num(); ++Index)
        {
            const FICBossPatternStep& Step = Row.Steps[Index];
            const FString Field = FString::Printf(TEXT("Steps[%d]"), Index);
            if (Step.SkillID.IsNone()) Missing(Issues, At, Field + TEXT(".SkillID"), TEXT("Pattern steps require an actual skill."), bComplete);
            const double LastStart = Step.StartOffsetPlaySeconds + FMath::Max(0, Step.RepeatCount - 1) * Step.RepeatIntervalPlaySeconds;
            if (LastStart > Row.DurationPlaySeconds)
                Issue(Issues, At, Field + TEXT(".StartOffsetPlaySeconds"), TEXT("Pattern skill starts must fit within the pattern duration."));
            if (Step.RepeatCount > 1 && Step.RepeatIntervalPlaySeconds <= 0.0) Issue(Issues, At, Field + TEXT(".RepeatIntervalPlaySeconds"), TEXT("Repeated pattern steps require a positive interval."));
            if (const FICSkillRow* const* Skill = Skills.Find(Step.SkillID))
            {
                if ((*Skill)->OwnerType != EICSkillOwner::Monster || (*Skill)->MonsterID != Row.MonsterID)
                    Issue(Issues, At, Field + TEXT(".SkillID"), TEXT("Pattern skill ownership must point back to the same boss."));
                if (Step.StartOffsetPlaySeconds + (*Skill)->FirstEffectOffsetPlaySeconds < Row.TelegraphPlaySeconds)
                    Issue(Issues, At, Field + TEXT(".StartOffsetPlaySeconds"), TEXT("The skill's first effect must follow the pattern telegraph; preparation may begin earlier."));
            }
        }
        for (int32 Index = 0; Index < Row.InvulnerabilityWindows.Num(); ++Index)
        {
            const FICPatternInvulnerabilityWindow& Window = Row.InvulnerabilityWindows[Index];
            if (Window.EndOffsetPlaySeconds <= Window.StartOffsetPlaySeconds || Window.EndOffsetPlaySeconds > Row.DurationPlaySeconds)
                Issue(Issues, At, FString::Printf(TEXT("InvulnerabilityWindows[%d]"), Index), TEXT("Invulnerability requires a positive window within the pattern duration."));
        }
        if (Row.bUnblockable && bComplete && Row.WarningIcon.IsNull() && Row.WarningKey.IsEmpty()) Issue(Issues, At, TEXT("WarningIcon"), TEXT("Unblockable patterns require an icon or text warning beyond color alone."));
        if (bComplete && Row.Montage.IsNull()) Issue(Issues, At, TEXT("Montage"), TEXT("A complete boss pattern requires its actual montage."));
    });
}
}
