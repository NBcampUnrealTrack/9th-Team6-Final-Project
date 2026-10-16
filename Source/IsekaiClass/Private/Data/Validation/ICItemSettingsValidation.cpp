#include "Data/Validation/ICDefinitionLibrary.h"
#include "Debug/ICLog.h"
#include "Data/Rows/Items/ICItemRows.h"
#include "Data/Rows/Rewards/ICRewardRows.h"
#include "Data/Rows/Settings/ICSettingsRows.h"
#include "Data/Rows/Character/ICCharacterRows.h"
#include "UObject/UnrealType.h"

namespace ICDefinitionValidation
{
namespace
{
struct FItemLocation { FString Table; FName Row; };

void ItemIssue(TArray<FICDefinitionIssue>& Issues,const FItemLocation& At,const FString& Field,const FString& Reason,
    bool bMissing=false,bool bComplete=true)
{
    IC_TRACE_FUNCTION();
    FICDefinitionIssue& Issue=Issues.AddDefaulted_GetRef();
    Issue.Table=At.Table; Issue.Row=At.Row; Issue.Field=Field; Issue.Reason=Reason;
    Issue.Severity=bMissing && !bComplete ? EICDefinitionSeverity::Warning : EICDefinitionSeverity::Error;
}

template<typename TRow,typename TCallback>
void ItemEach(const TArray<FICDefinitionTable>& Tables,TCallback Callback,bool bIncludeDeprecated=false)
{
    IC_TRACE_FUNCTION();
    for(const FICDefinitionTable& Binding:Tables)
    {
        const UDataTable* Table=Binding.Table;
        if(!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(TRow::StaticStruct())) continue;
        for(const auto& Entry:Table->GetRowMap())
        {
            const TRow* Row=reinterpret_cast<const TRow*>(Entry.Value);
            if(Row && !Row->bTestOnly && (bIncludeDeprecated || !Row->bDeprecated)) Callback(*Row,FItemLocation{Table->GetPathName(),Entry.Key});
        }
    }
}

void CheckCondition(const FICCondition& Condition,const FItemLocation& At,const FString& Field,
    bool bComplete,TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    if(Condition.Type==EICConditionType::None) return;
    EICDefinitionType Expected=EICDefinitionType::Settings;
    bool bNeedsDefinition=false;
    switch(Condition.Type)
    {
    case EICConditionType::QuestAccepted: case EICConditionType::QuestCompleted: Expected=EICDefinitionType::Quest; bNeedsDefinition=true; break;
    case EICConditionType::ItemInBag: Expected=EICDefinitionType::Item; bNeedsDefinition=true; break;
    case EICConditionType::Affinity: Expected=EICDefinitionType::NPC; bNeedsDefinition=true; break;
    case EICConditionType::VisitedAnchor: Expected=EICDefinitionType::Anchor; bNeedsDefinition=true; break;
    case EICConditionType::BossDefeated: Expected=EICDefinitionType::Monster; bNeedsDefinition=true; break;
    case EICConditionType::WarpUnlocked: Expected=EICDefinitionType::Warp; bNeedsDefinition=true; break;
    case EICConditionType::RecipeUnlocked: Expected=EICDefinitionType::Recipe; bNeedsDefinition=true; break;
    case EICConditionType::HasStatusEffect: Expected=EICDefinitionType::StatusEffect; bNeedsDefinition=true; break;
    default: break;
    }
    if(bNeedsDefinition)
    {
        if(Condition.Target.Type!=Expected) ItemIssue(Issues,At,Field+TEXT(".Target.Type"),TEXT("Condition target definition type does not match its semantic kind."));
        if(Condition.Target.ID.IsNone()) ItemIssue(Issues,At,Field+TEXT(".Target.ID"),TEXT("This condition requires its actual referenced definition."),true,bComplete);
    }
    if(Condition.Type==EICConditionType::Stat)
    {
        if(Condition.Stat==EICStat::None) ItemIssue(Issues,At,Field+TEXT(".Stat"),TEXT("Stat conditions require an explicit stat."));
        const bool bPrimary=Condition.Stat>=EICStat::STR && Condition.Stat<=EICStat::LUK;
        if(!bPrimary && !Condition.bUseEffectiveStats) ItemIssue(Issues,At,Field+TEXT(".bUseEffectiveStats"),TEXT("The default base-stat condition must use one of six primary stats; derived/effective exceptions must be explicit."));
        if(bPrimary && (Condition.Value<1 || Condition.Value>10000 || Condition.Value!=FMath::RoundToDouble(Condition.Value)))
            ItemIssue(Issues,At,Field+TEXT(".Value"),TEXT("Primary-stat thresholds must be integers in [1,10000]."));
    }
    if(Condition.Type==EICConditionType::ItemInBag && (Condition.Value<1 || Condition.Value!=FMath::RoundToDouble(Condition.Value)))
        ItemIssue(Issues,At,Field+TEXT(".Value"),TEXT("Inventory count thresholds must be positive integers."));
    if(Condition.Type==EICConditionType::Affinity && (Condition.Value<0 || Condition.Value>100 || Condition.Value!=FMath::RoundToDouble(Condition.Value)))
        ItemIssue(Issues,At,Field+TEXT(".Value"),TEXT("Affinity thresholds must be integers in [0,100]."));
    if((Condition.Type==EICConditionType::CurrentHPRatio || Condition.Type==EICConditionType::CurrentMPRatio) && (Condition.Value<0 || Condition.Value>1))
        ItemIssue(Issues,At,Field+TEXT(".Value"),TEXT("Resource ratio thresholds use [0,1]."));
    if(Condition.Type==EICConditionType::Flag && Condition.Flag.IsNone()) ItemIssue(Issues,At,Field+TEXT(".Flag"),TEXT("Flag conditions require their persistent flag key."),true,bComplete);
    if((Condition.Type==EICConditionType::HasGameplayTag || Condition.Type==EICConditionType::CharacterMode) && Condition.RequiredTags.IsEmpty())
        ItemIssue(Issues,At,Field+TEXT(".RequiredTags"),TEXT("Tag/mode conditions require explicit gameplay tags."),true,bComplete);
    // Runtime condition kinds inherently inspect current state; bAllowCurrentState is a caller policy.
}

void InspectConditions(const UStruct* Struct,const void* Data,const FItemLocation& At,const FString& Path,
    bool bComplete,TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    if(Struct==FICCondition::StaticStruct())
    {
        CheckCondition(*static_cast<const FICCondition*>(Data),At,Path,bComplete,Issues);
        return;
    }
    for(TFieldIterator<FProperty> It(Struct);It;++It)
    {
        const FProperty* Property=*It;
        const FString Field=Path.IsEmpty()?Property->GetName():Path+TEXT(".")+Property->GetName();
        const void* Value=Property->ContainerPtrToValuePtr<void>(Data);
        if(const FStructProperty* Nested=CastField<FStructProperty>(Property)) InspectConditions(Nested->Struct,Value,At,Field,bComplete,Issues);
        else if(const FArrayProperty* Array=CastField<FArrayProperty>(Property))
        {
            if(const FStructProperty* Inner=CastField<FStructProperty>(Array->Inner))
            {
                FScriptArrayHelper Helper(Array,Value);
                for(int32 Index=0;Index<Helper.Num();++Index) InspectConditions(Inner->Struct,Helper.GetRawPtr(Index),At,FString::Printf(TEXT("%s[%d]"),*Field,Index),bComplete,Issues);
            }
        }
    }
}
}

void ValidateItemSettings(const TArray<FICDefinitionTable>& Tables,bool bComplete,TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    TMap<FName,const FICItemRow*> Items;
    ItemEach<FICItemRow>(Tables,[&](const FICItemRow& Row,const FItemLocation&){ IC_TRACE_FUNCTION();Items.Add(Row.ID,&Row);});
    TMap<FName,const FICEquipmentRow*> Equipment;
    ItemEach<FICEquipmentRow>(Tables,[&](const FICEquipmentRow& Row,const FItemLocation&){ IC_TRACE_FUNCTION();Equipment.Add(Row.ID,&Row);});
    TMap<FName,const FICIDRegistryRow*> Registry;
    ItemEach<FICIDRegistryRow>(Tables,[&](const FICIDRegistryRow& Row,const FItemLocation&){ IC_TRACE_FUNCTION();Registry.Add(Row.ID,&Row);},true);

    for(const FICDefinitionTable& Binding:Tables)
    {
        const UDataTable* Table=Binding.Table;
        if(!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FICDefinitionRow::StaticStruct())) continue;
        for(const auto& Entry:Table->GetRowMap())
        {
            const FICDefinitionRow* Row=reinterpret_cast<const FICDefinitionRow*>(Entry.Value);
            if(Row && !Row->bDeprecated && !Row->bTestOnly) InspectConditions(Table->GetRowStruct(),Entry.Value,FItemLocation{Table->GetPathName(),Entry.Key},TEXT(""),bComplete,Issues);
        }
    }

    ItemEach<FICEquipmentRow>(Tables,[&](const FICEquipmentRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.ItemID.IsNone()) ItemIssue(Issues,At,TEXT("ItemID"),TEXT("Equipment requires its inventory item definition."),true,bComplete);
        if(Row.Slot==EPlayerEquipSlot::None) ItemIssue(Issues,At,TEXT("Slot"),TEXT("Equipment requires one of the four equipment slots."),true,bComplete);
        if(Row.Slot==EPlayerEquipSlot::Weapon && Row.WeaponFamily==EICWeaponFamily::None) ItemIssue(Issues,At,TEXT("WeaponFamily"),TEXT("A weapon requires its fixed family."),true,bComplete);
        if(Row.Slot!=EPlayerEquipSlot::Weapon && Row.WeaponFamily!=EICWeaponFamily::None) ItemIssue(Issues,At,TEXT("WeaponFamily"),TEXT("Clothes, rings and necklaces are not weapon families."));
        EICWeaponFamily Expected=EICWeaponFamily::None;
        switch(Row.RequiredClass)
        {
        case EICCharacterClass::Warrior:Expected=EICWeaponFamily::Sword;break;
        case EICCharacterClass::Archer:Expected=EICWeaponFamily::Bow;break;
        case EICCharacterClass::Mage:Expected=EICWeaponFamily::Staff;break;
        case EICCharacterClass::Healer:Expected=EICWeaponFamily::Wand;break;
        default:break;
        }
        if(Row.Slot==EPlayerEquipSlot::Weapon && Expected!=EICWeaponFamily::None && Row.WeaponFamily!=Expected) ItemIssue(Issues,At,TEXT("WeaponFamily"),TEXT("Required class and fixed weapon family disagree."));
        if(const FICItemRow* const* Item=Items.Find(Row.ItemID))
        {
            if((*Item)->Kind!=EPlayerItemKind::Equipment || (*Item)->EquipSlot!=Row.Slot) ItemIssue(Issues,At,TEXT("ItemID"),TEXT("Linked inventory item must be equipment in the same slot."));
            if(!(*Item)->EquipmentID.IsNone() && (*Item)->EquipmentID!=Row.ID) ItemIssue(Issues,At,TEXT("ItemID"),TEXT("Inventory item's equipment back-reference points elsewhere."));
        }
        for(int32 Index=0;Index<Row.BaseModifiers.Num();++Index) if(Row.BaseModifiers[Index].Stat==EICStat::None) ItemIssue(Issues,At,FString::Printf(TEXT("BaseModifiers[%d].Stat"),Index),TEXT("Equipment modifiers require an explicit stat."));
    });
    auto CheckInitialEquipment=[&](FName ID,EPlayerEquipSlot Slot,EICWeaponFamily Family,const FItemLocation& At,const FString& Field)
    {
        IC_TRACE_FUNCTION();
        if(const FICEquipmentRow* const* Definition=Equipment.Find(ID))
        {
            if((*Definition)->Slot!=Slot) ItemIssue(Issues,At,Field,TEXT("Initial equipment definition occupies a different equipment slot."));
            if(Slot==EPlayerEquipSlot::Weapon && (*Definition)->WeaponFamily!=Family) ItemIssue(Issues,At,Field,TEXT("Initial weapon definition must match the character's fixed weapon family."));
        }
    };
    ItemEach<FICClassRow>(Tables,[&](const FICClassRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        CheckInitialEquipment(Row.StartingWeaponID,EPlayerEquipSlot::Weapon,Row.WeaponFamily,At,TEXT("StartingWeaponID"));
        CheckInitialEquipment(Row.StartingClothesID,EPlayerEquipSlot::Clothes,Row.WeaponFamily,At,TEXT("StartingClothesID"));
    });
    ItemEach<FICRecruitCombatRow>(Tables,[&](const FICRecruitCombatRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        CheckInitialEquipment(Row.InitialEquipment.WeaponID,EPlayerEquipSlot::Weapon,Row.WeaponFamily,At,TEXT("InitialEquipment.WeaponID"));
        CheckInitialEquipment(Row.InitialEquipment.ClothesID,EPlayerEquipSlot::Clothes,Row.WeaponFamily,At,TEXT("InitialEquipment.ClothesID"));
        CheckInitialEquipment(Row.InitialEquipment.RingID,EPlayerEquipSlot::Ring,Row.WeaponFamily,At,TEXT("InitialEquipment.RingID"));
        CheckInitialEquipment(Row.InitialEquipment.NecklaceID,EPlayerEquipSlot::Necklace,Row.WeaponFamily,At,TEXT("InitialEquipment.NecklaceID"));
    });
    ItemEach<FICEnhancementRow>(Tables,[&](const FICEnhancementRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.MaximumLevel<1 || Row.MaximumLevel>10) ItemIssue(Issues,At,TEXT("MaximumLevel"),TEXT("Enhancement targets are restricted to +1 through +10."));
        if(Row.EnhancedStats.IsEmpty()) ItemIssue(Issues,At,TEXT("EnhancedStats"),TEXT("Specify the original fixed stats increased by enhancement."),true,bComplete);
        TSet<EICStat> SeenStats;
        for(EICStat Stat:Row.EnhancedStats)
        {
            if(Stat==EICStat::None || SeenStats.Contains(Stat)) ItemIssue(Issues,At,TEXT("EnhancedStats"),TEXT("Enhancement stat list must be explicit and unique."));
            SeenStats.Add(Stat);
        }
        TSet<int32> SeenLevels;
        for(int32 Index=0;Index<Row.Steps.Num();++Index)
        {
            const FICEnhancementStep& Step=Row.Steps[Index];
            if(Step.TargetLevel<1 || Step.TargetLevel>Row.MaximumLevel || SeenLevels.Contains(Step.TargetLevel)) ItemIssue(Issues,At,FString::Printf(TEXT("Steps[%d].TargetLevel"),Index),TEXT("Enhancement steps must have unique target levels within the configured maximum."));
            SeenLevels.Add(Step.TargetLevel);
        }
        for(int32 Level=1;Level<=FMath::Min(Row.MaximumLevel,10);++Level) if(!SeenLevels.Contains(Level)) ItemIssue(Issues,At,TEXT("Steps"),FString::Printf(TEXT("Missing authored enhancement step +%d."),Level),true,bComplete);
    });
    ItemEach<FICOptionRow>(Tables,[&](const FICOptionRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.Stat==EICStat::None) ItemIssue(Issues,At,TEXT("Stat"),TEXT("Random options require an explicit stat."),true,bComplete);
        if(Row.MinimumValue>Row.MaximumValue) ItemIssue(Issues,At,TEXT("MaximumValue"),TEXT("Option maximum must be at least its minimum."));
        if(Row.AllowedSlots.Contains(EPlayerEquipSlot::None)) ItemIssue(Issues,At,TEXT("AllowedSlots"),TEXT("None is not an equipment slot; use an empty list for unrestricted slots."));
        if(Row.AllowedWeaponFamilies.Contains(EICWeaponFamily::None)) ItemIssue(Issues,At,TEXT("AllowedWeaponFamilies"),TEXT("Use an empty family list for unrestricted non-weapon options."));
    });
    ItemEach<FICOptionPoolRow>(Tables,[&](const FICOptionPoolRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.bAllowDuplicateOptions) ItemIssue(Issues,At,TEXT("bAllowDuplicateOptions"),TEXT("Final equipment rules forbid duplicate options on an instance."));
        if(Row.Grades.IsEmpty()) ItemIssue(Issues,At,TEXT("Grades"),TEXT("Author rarity-specific option counts and weighted pools."),true,bComplete);
        TSet<EPlayerItemGrade> Grades;
        for(int32 Index=0;Index<Row.Grades.Num();++Index)
        {
            const FICGradeOptionPool& Grade=Row.Grades[Index];
            const FString Field=FString::Printf(TEXT("Grades[%d]"),Index);
            if(Grades.Contains(Grade.Grade)) ItemIssue(Issues,At,Field+TEXT(".Grade"),TEXT("Each rarity must appear once."));
            Grades.Add(Grade.Grade);
            TSet<FName> PositiveOptions;
            for(int32 OptionIndex=0;OptionIndex<Grade.Options.Num();++OptionIndex)
            {
                const FICWeightedOption& Entry=Grade.Options[OptionIndex];
                if(Entry.OptionID.IsNone()) ItemIssue(Issues,At,FString::Printf(TEXT("%s.Options[%d].OptionID"),*Field,OptionIndex),TEXT("Weighted option requires its actual definition."),true,bComplete);
                if(Entry.Weight>0 && !Entry.OptionID.IsNone())
                {
                    if(PositiveOptions.Contains(Entry.OptionID)) ItemIssue(Issues,At,Field+TEXT(".Options"),TEXT("Combine duplicate option weights into one entry."));
                    PositiveOptions.Add(Entry.OptionID);
                }
            }
            if(Grade.OptionCount>PositiveOptions.Num()) ItemIssue(Issues,At,Field+TEXT(".Options"),TEXT("A non-duplicate roll needs at least OptionCount distinct positive-weight options."),Grade.Options.IsEmpty(),bComplete);
        }
    });
    ItemEach<FICRewardRow>(Tables,[&](const FICRewardRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        TSet<FName> ItemIDs,NPCIDs,Recipes;
        for(int32 Index=0;Index<Row.Items.Num();++Index)
        {
            const FICItemAmount& Entry=Row.Items[Index];
            if(Entry.ItemID.IsNone()) ItemIssue(Issues,At,FString::Printf(TEXT("Items[%d].ItemID"),Index),TEXT("Reward item requires its actual definition."),true,bComplete);
            else if(ItemIDs.Contains(Entry.ItemID)) ItemIssue(Issues,At,TEXT("Items"),TEXT("Combine duplicate reward item quantities to prevent duplicate grants."));
            ItemIDs.Add(Entry.ItemID);
        }
        for(const FICAffinityReward& Entry:Row.Affinity)
        {
            if(Entry.NPCID.IsNone()) ItemIssue(Issues,At,TEXT("Affinity.NPCID"),TEXT("Affinity rewards require their NPC definition."),true,bComplete);
            else if(NPCIDs.Contains(Entry.NPCID)) ItemIssue(Issues,At,TEXT("Affinity"),TEXT("Combine duplicate NPC affinity grants."));
            NPCIDs.Add(Entry.NPCID);
        }
        for(FName Recipe:Row.UnlockRecipeIDs)
        {
            if(Recipe.IsNone()) ItemIssue(Issues,At,TEXT("UnlockRecipeIDs"),TEXT("Recipe unlock requires its definition."),true,bComplete);
            else if(Recipes.Contains(Recipe)) ItemIssue(Issues,At,TEXT("UnlockRecipeIDs"),TEXT("Recipe reward unlocks must be unique."));
            Recipes.Add(Recipe);
        }
    });
    ItemEach<FICDropTableRow>(Tables,[&](const FICDropTableRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.MinimumGold>Row.MaximumGold) ItemIssue(Issues,At,TEXT("MaximumGold"),TEXT("Maximum gold must be at least minimum gold."));
        double Sum=0; bool bNeedsGrades=false;
        for(int32 Index=0;Index<Row.Entries.Num();++Index)
        {
            const FICDropEntry& Entry=Row.Entries[Index]; const FString Field=FString::Printf(TEXT("Entries[%d]"),Index);
            Sum+=Entry.Weight; bNeedsGrades|=Entry.bEquipmentGradeRoll;
            if(Entry.ItemID.IsNone()) ItemIssue(Issues,At,Field+TEXT(".ItemID"),TEXT("Drop entry requires its actual item definition."),true,bComplete);
            if(Entry.MinimumCount>Entry.MaximumCount) ItemIssue(Issues,At,Field+TEXT(".MaximumCount"),TEXT("Drop maximum count must be at least its minimum."));
            if(const FICItemRow* const* Item=Items.Find(Entry.ItemID))
            {
                if((*Item)->Kind==EPlayerItemKind::Quest) ItemIssue(Issues,At,Field+TEXT(".ItemID"),TEXT("Quest items cannot be ordinary world drops."));
                if(Entry.bEquipmentGradeRoll && (*Item)->Kind!=EPlayerItemKind::Equipment) ItemIssue(Issues,At,Field+TEXT(".bEquipmentGradeRoll"),TEXT("Equipment rarity rolls require equipment items."));
            }
        }
        if(Row.Selection==EICDropSelection::WeightedChoice && !Row.Entries.IsEmpty() && Sum<=0) ItemIssue(Issues,At,TEXT("Entries"),TEXT("Weighted drop pools require a positive total weight."));
        if(bNeedsGrades && Row.EquipmentGrades.IsEmpty()) ItemIssue(Issues,At,TEXT("EquipmentGrades"),TEXT("Equipment rarity rolls require an authored rarity-weight table."),true,bComplete);
        TSet<EPlayerItemGrade> Grades; double GradeWeight=0;
        for(int32 Index=0;Index<Row.EquipmentGrades.Num();++Index)
        {
            const FICEquipmentGradeWeight& Grade=Row.EquipmentGrades[Index]; GradeWeight+=Grade.Weight;
            const FString Field=FString::Printf(TEXT("EquipmentGrades[%d]"),Index);
            if(Grades.Contains(Grade.Grade)) ItemIssue(Issues,At,Field+TEXT(".Grade"),TEXT("Rarity weights must be unique."));
            Grades.Add(Grade.Grade);
            if(Grade.Grade<EPlayerItemGrade::Rare && Grade.bApplyPlayerLUK) ItemIssue(Issues,At,Field+TEXT(".bApplyPlayerLUK"),TEXT("LUK rarity weighting applies only to Rare and above."));
            if(Grade.Grade>=EPlayerItemGrade::Rare && !Grade.bApplyPlayerLUK) ItemIssue(Issues,At,Field+TEXT(".bApplyPlayerLUK"),TEXT("Rare and higher equipment weights must receive player LUK."));
        }
        if(!Row.EquipmentGrades.IsEmpty() && GradeWeight<=0) ItemIssue(Issues,At,TEXT("EquipmentGrades"),TEXT("Rarity pool requires a positive total weight."));
    });

    ItemEach<FICNewGameRow>(Tables,[&](const FICNewGameRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.StartAnchorID.IsNone()) ItemIssue(Issues,At,TEXT("StartAnchorID"),TEXT("New game requires its player-house starting anchor."),true,bComplete);
        if(Row.RegionID.IsNone()) ItemIssue(Issues,At,TEXT("RegionID"),TEXT("New game requires its starting region."),true,bComplete);
        if(Row.MinimumNameLength>Row.MaximumNameLength) ItemIssue(Issues,At,TEXT("MaximumNameLength"),TEXT("Maximum name length must be at least minimum length."));
        if(Row.InitialGameMinute>=1440) ItemIssue(Issues,At,TEXT("InitialGameMinute"),TEXT("Day-one starting time must lie within [0,1440) game minutes."));
        if(Row.InitialHP<0 || Row.InitialHP>Row.InitialMaxHP || Row.InitialMP<0 || Row.InitialMP>Row.InitialMaxMP || Row.InitialSP<0 || Row.InitialSP>Row.InitialMaxSP) ItemIssue(Issues,At,TEXT("InitialResources"),TEXT("Initial resources must be within the configured maxima."));
    });
    ItemEach<FICTimeSettingsRow>(Tables,[&](const FICTimeSettingsRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.SPDrainIntervalPlaySeconds!=10) ItemIssue(Issues,At,TEXT("SPDrainIntervalPlaySeconds"),TEXT("Natural SP drains by one per ten unpaused PlayClock seconds."));
        if(Row.RealSecondsPerGameMinute<=0 || Row.SleepRecoveryPerGameMinute<=0) ItemIssue(Issues,At,TEXT("RealSecondsPerGameMinute"),TEXT("Game-clock scale and sleep recovery must be positive."));
    });
    ItemEach<FICStatSettingsRow>(Tables,[&](const FICStatSettingsRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.MainStatCap>10000) ItemIssue(Issues,At,TEXT("MainStatCap"),TEXT("Final primary-stat cap must not exceed 10000."));
        if(Row.BaseMaxHP<1) ItemIssue(Issues,At,TEXT("BaseMaxHP"),TEXT("At primary stats of one, maximum HP must remain at least one."));
        if(Row.BaseCriticalChance>Row.CriticalChanceCap) ItemIssue(Issues,At,TEXT("CriticalChanceCap"),TEXT("Critical cap cannot be below the baseline critical chance."));
        if(Row.BaseCriticalMultiplier>Row.CriticalMultiplierCap) ItemIssue(Issues,At,TEXT("CriticalMultiplierCap"),TEXT("Critical multiplier cap cannot be below its baseline."));
        const TSet<EICStat> Allowed={EICStat::MaxHP,EICStat::MaxMP,EICStat::PhysicalAttack,EICStat::MagicAttack,EICStat::Defense};
        TSet<EICStat> Seen;
        for(EICStat Stat:Row.CompanionCHAAffectedStats)
        {
            if(!Allowed.Contains(Stat) || Seen.Contains(Stat)) ItemIssue(Issues,At,TEXT("CompanionCHAAffectedStats"),TEXT("Player CHA affects only companion maximum HP/MP, physical/magic attack and defense, once each."));
            Seen.Add(Stat);
        }
        if(Seen.Num()!=5) ItemIssue(Issues,At,TEXT("CompanionCHAAffectedStats"),TEXT("All five designated companion derived stats must receive player CHA."),Row.CompanionCHAAffectedStats.IsEmpty(),bComplete);
    });
    ItemEach<FICCombatSettingsRow>(Tables,[&](const FICCombatSettingsRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.RunSpeedCmPerSecond<Row.WalkSpeedCmPerSecond) ItemIssue(Issues,At,TEXT("RunSpeedCmPerSecond"),TEXT("Running cannot be slower than walking."));
        if(Row.InputBufferPlaySeconds>Row.ComboWindowPlaySeconds) ItemIssue(Issues,At,TEXT("ComboWindowPlaySeconds"),TEXT("Combo connection window must cover its input buffer."));
        if(Row.DodgeInvulnerabilityPlaySeconds>Row.DodgeCooldownPlaySeconds) ItemIssue(Issues,At,TEXT("DodgeCooldownPlaySeconds"),TEXT("Dodge cooldown must cover its invulnerability span."));
        if(Row.HitStaggerPlaySeconds>Row.HitStaggerIntervalPlaySeconds) ItemIssue(Issues,At,TEXT("HitStaggerIntervalPlaySeconds"),TEXT("Repeat stagger interval must cover the stagger span."));
    });
    ItemEach<FICPartySettingsRow>(Tables,[&](const FICPartySettingsRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.SearchDistanceCm>Row.ChaseDistanceCm || Row.ChaseDistanceCm>Row.RecoveryDistanceCm) ItemIssue(Issues,At,TEXT("RecoveryDistanceCm"),TEXT("Party search, chase and safe recovery ranges must be non-decreasing."));
        if(!Row.FormationOffsetsCm.IsEmpty() && Row.FormationOffsetsCm.Num()<Row.MaximumCompanions) ItemIssue(Issues,At,TEXT("FormationOffsetsCm"),TEXT("Formation must supply a position for every companion slot."));
        if(bComplete && Row.FormationOffsetsCm.IsEmpty()) ItemIssue(Issues,At,TEXT("FormationOffsetsCm"),TEXT("Complete party settings require authored formation positions."));
    });
    ItemEach<FICInventorySettingsRow>(Tables,[&](const FICInventorySettingsRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.InitialSlots>Row.MaximumSlots) ItemIssue(Issues,At,TEXT("MaximumSlots"),TEXT("Maximum inventory capacity cannot be below initial capacity."));
        if(Row.ExpansionSlots>0 && Row.MaximumSlots>=Row.InitialSlots && (Row.MaximumSlots-Row.InitialSlots)%Row.ExpansionSlots!=0) ItemIssue(Issues,At,TEXT("ExpansionSlots"),TEXT("Fixed expansion steps must reach the configured maximum without overshoot."));
        if(static_cast<int64>(Row.MaximumStorageInstallations)*Row.SlotsPerStorage>MAX_int32) ItemIssue(Issues,At,TEXT("SlotsPerStorage"),TEXT("Combined storage capacity exceeds the supported integer range."));
    });
    ItemEach<FICSaveSettingsRow>(Tables,[&](const FICSaveSettingsRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.ManualSlotCount!=4 || Row.ClearRecordSlot!=5) ItemIssue(Issues,At,TEXT("ClearRecordSlot"),TEXT("Manual world slots are 1-4 and the separate clear record is slot 5."));
        if(Row.AutoSaveIntervalPlaySeconds<=0) ItemIssue(Issues,At,TEXT("AutoSaveIntervalPlaySeconds"),TEXT("Periodic autosave requires a positive PlayClock interval."));
    });

    ItemEach<FICIDRegistryRow>(Tables,[&](const FICIDRegistryRow& Row,const FItemLocation& At)
    {
        IC_TRACE_FUNCTION();
        if(Row.DefinitionType==EICDefinitionType::IDRegistry) ItemIssue(Issues,At,TEXT("DefinitionType"),TEXT("Registry entries classify actual definitions, not registry entries themselves."));
        if(Row.DefinitionTable.IsNull()) ItemIssue(Issues,At,TEXT("DefinitionTable"),TEXT("Registry requires the owning definition table."),true,bComplete);
        else
        {
            const FICDefinitionTable* Owner=Tables.FindByPredicate([&](const FICDefinitionTable& Binding){ IC_TRACE_FUNCTION();return Binding.Table && Binding.Table->GetPathName()==Row.DefinitionTable.ToSoftObjectPath().ToString();});
            if(Owner)
            {
                if(Owner->Type!=Row.DefinitionType) ItemIssue(Issues,At,TEXT("DefinitionType"),TEXT("Registry type disagrees with its definition table."));
                if(!Owner->Table->GetRowMap().Contains(Row.ID)) ItemIssue(Issues,At,TEXT("ID"),TEXT("Registry ID is missing from the selected definition table."),true,bComplete);
            }
            else ItemIssue(Issues,At,TEXT("DefinitionTable"),TEXT("Owning table was not supplied for registry validation."),true,bComplete);
        }
        TSet<FName> Chain; FName Cursor=Row.ID;
        while(const FICIDRegistryRow* const* Current=Registry.Find(Cursor))
        {
            if(Chain.Contains(Cursor)){ItemIssue(Issues,At,TEXT("ReplacementID"),TEXT("Registry replacement references form a cycle."));break;}
            Chain.Add(Cursor);
            const FName Next=(*Current)->ReplacementID;
            if(Next.IsNone()) break;
            const FICIDRegistryRow* const* Replacement=Registry.Find(Next);
            if(!Replacement){ItemIssue(Issues,At,TEXT("ReplacementID"),TEXT("Replacement ID is missing from the registry."),true,bComplete);break;}
            if((*Replacement)->DefinitionType!=Row.DefinitionType) ItemIssue(Issues,At,TEXT("ReplacementID"),TEXT("Replacements must preserve the original definition type."));
            Cursor=Next;
        }
    },true);
}
}
