#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Debug/ICLog.h"
#include "Data/Validation/ICDefinitionLibrary.h"

namespace
{
void Failure(FICDefinitionResult& Result, EICDefinitionFailure Code, const TCHAR* Field, const TCHAR* Reason)
{
    IC_TRACE_FUNCTION();
    Result.bSuccess=false; Result.Code=Code; Result.Field=Field; Result.Reason=Reason;
    UE_LOG(LogICData, Verbose, TEXT("[Definition.Rejected] ID=%s Type=%d Table=%s Field=%s Code=%d Reason=%s"),
        *Result.ID.ToString(), static_cast<int32>(Result.Type), *Result.Table, *Result.Field, static_cast<int32>(Code), *Result.Reason);
}
}

void UICDefinitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    IC_TRACE_FUNCTION();
    Super::Initialize(Collection);
    ConfigureCatalog(GetDefault<UICDefinitionSettings>()->Catalog.LoadSynchronous());
    if (!Catalog) UE_LOG(LogICData, Error, TEXT("[Definition.Initialize] 필수 카탈로그를 로드하지 못했습니다. Project Settings의 Definition Catalog 경로=%s"), *GetDefault<UICDefinitionSettings>()->Catalog.ToSoftObjectPath().ToString());
}
void UICDefinitionSubsystem::Deinitialize()
{ IC_TRACE_FUNCTION(); UE_LOG(LogICData, Log, TEXT("[Definition.Deinitialize] 캐시 테이블=%d"),LoadedTables.Num()); LoadedTables.Reset(); CachedSchemaTypes.Reset(); Catalog=nullptr; Super::Deinitialize(); }
void UICDefinitionSubsystem::ConfigureCatalog(UICDefinitionCatalog* InCatalog)
{
    IC_TRACE_FUNCTION(); LoadedTables.Reset(); CachedSchemaTypes.Reset(); Catalog=InCatalog;
    for(const auto& Schema:UICDefinitionLibrary::GetDefinitionSchemas())
        CachedSchemaTypes.Add(Schema.RowStruct.Get(),Schema.Type);
    UE_LOG(LogICData, Verbose, TEXT("[Definition.Configure] Catalog=%s Tables=%d StringTables=%d"),*GetPathNameSafe(Catalog),Catalog?Catalog->Tables.Num():0,Catalog?Catalog->StringTables.Num():0);
}

UDataTable* UICDefinitionSubsystem::LoadTable(const FICDefinitionBinding& Binding, FICDefinitionResult& Result) const
{
    IC_TRACE_FUNCTION();
    Result.Table=Binding.Table.ToSoftObjectPath().ToString();
    const auto* SchemaType=CachedSchemaTypes.Find(Binding.RowStruct.Get());
    if(!Binding.RowStruct || !SchemaType || *SchemaType!=Binding.Type)
    { Failure(Result,EICDefinitionFailure::WrongRowStruct,TEXT("RowStruct"),TEXT("Catalog type and native row schema do not agree."));return nullptr; }
    UDataTable* Table=Binding.Table.LoadSynchronous();
    if(!Table) {Failure(Result,EICDefinitionFailure::TableLoadFailed,TEXT("Table"),TEXT("Registered DataTable could not be loaded."));return nullptr;}
    if(Table->GetRowStruct()!=Binding.RowStruct)
    { Failure(Result,EICDefinitionFailure::WrongRowStruct,TEXT("RowStruct"),TEXT("DataTable has a different row struct."));return nullptr; }
    const_cast<UICDefinitionSubsystem*>(this)->LoadedTables.AddUnique(Table);
    return Table;
}

const FICDefinitionRow* UICDefinitionSubsystem::FindRow(FName ID,const UScriptStruct* ExpectedStruct,FICDefinitionResult& Result) const
{
    IC_TRACE_FUNCTION();
    Result=FICDefinitionResult(); Result.ID=Result.Row=ID;
    const auto* SchemaType=CachedSchemaTypes.Find(ExpectedStruct);
    if(!SchemaType) {Failure(Result,EICDefinitionFailure::WrongRowStruct,TEXT("RowStruct"),TEXT("Expected a registered IC native row struct."));return nullptr;}
    Result.Type=*SchemaType;
    if(!UICDefinitionLibrary::IsDefinitionIDValid(ID)) {Failure(Result,EICDefinitionFailure::InvalidID,TEXT("ID"),TEXT("ID must contain uppercase letters, digits or underscores (maximum 40)."));return nullptr;}
    if(!Catalog) {Failure(Result,EICDefinitionFailure::CatalogUnavailable,TEXT("Catalog"),TEXT("Definition catalog is not configured or could not be loaded."));return nullptr;}
    const FICDefinitionRow* Found=nullptr; FString FoundTable; bool bRegistered=false;

    for(const auto& Binding:Catalog->Tables)
    {
        if(Binding.Type!=*SchemaType)continue;
        bRegistered |= Binding.RowStruct==ExpectedStruct;
        UDataTable* Table=LoadTable(Binding,Result);if(!Table)return nullptr;
        const uint8* const* Bytes=Table->GetRowMap().Find(ID);if(!Bytes)continue;
        const auto* Candidate=reinterpret_cast<const FICDefinitionRow*>(*Bytes);
        if(!Candidate || Candidate->ID!=ID) {Failure(Result,EICDefinitionFailure::IDMismatch,TEXT("ID"),TEXT("RowName and definition ID must match."));return nullptr;}
        if(Found) {Failure(Result,EICDefinitionFailure::DuplicateID,TEXT("ID"),TEXT("ID exists more than once in this definition type."));return nullptr;}
        Found=Candidate;FoundTable=Table->GetPathName();
        if(Binding.RowStruct!=ExpectedStruct) {Failure(Result,EICDefinitionFailure::WrongRowStruct,TEXT("RowStruct"),TEXT("ID belongs to another row schema."));return nullptr;}
    }
    if(Found)Result.Table=FoundTable;
    if(!bRegistered) {Failure(Result,EICDefinitionFailure::TableUnregistered,TEXT("Table"),TEXT("No table is registered for this native row schema."));return nullptr;}
    if(!Found) {Failure(Result,EICDefinitionFailure::RowMissing,TEXT("Row"),TEXT("Definition row does not exist."));return nullptr;}
    if(Found->bDeprecated) {Failure(Result,EICDefinitionFailure::Deprecated,TEXT("bDeprecated"),TEXT("Deprecated definitions cannot be used for gameplay."));return nullptr;}
    if(Found->bTestOnly) {Failure(Result,EICDefinitionFailure::TestDefinition,TEXT("bTestOnly"),TEXT("Test definitions cannot be used for gameplay."));return nullptr;}
    if(Found->DataVersion<1) {Failure(Result,EICDefinitionFailure::InvalidDefinition,TEXT("DataVersion"),TEXT("DataVersion must be positive."));return nullptr;}
    Result.bSuccess=true;Result.Code=EICDefinitionFailure::None;
    UE_LOG(LogICData, VeryVerbose, TEXT("[Definition.Found] ID=%s Type=%d Table=%s"),*ID.ToString(),static_cast<int32>(Result.Type),*Result.Table);
    return Found;
}

FICDefinitionResult UICDefinitionSubsystem::GetDefinitionIDs(EICDefinitionType Type,TArray<FName>& IDs) const
{
    IC_TRACE_FUNCTION();
    IDs.Reset();FICDefinitionResult Result;Result.Type=Type;
    if(!Catalog) {Failure(Result,EICDefinitionFailure::CatalogUnavailable,TEXT("Catalog"),TEXT("Definition catalog is unavailable."));return Result;}
    bool Registered=false;TSet<FName> Seen;
    for(const auto& Binding:Catalog->Tables)
    {
        if(Binding.Type!=Type)continue;Registered=true;
        const UDataTable* Table=LoadTable(Binding,Result);if(!Table){IDs.Reset();return Result;}
        for(const auto& Pair:Table->GetRowMap())
        {
            Result.ID=Result.Row=Pair.Key;
            const auto* Row=reinterpret_cast<const FICDefinitionRow*>(Pair.Value);
            if(!Row || Row->ID!=Pair.Key || !UICDefinitionLibrary::IsDefinitionIDValid(Pair.Key))
            {Failure(Result,EICDefinitionFailure::IDMismatch,TEXT("ID"),TEXT("Invalid ID or RowName/ID mismatch."));IDs.Reset();return Result;}
            if(Seen.Contains(Pair.Key)) {Failure(Result,EICDefinitionFailure::DuplicateID,TEXT("ID"),TEXT("Duplicate definition ID."));IDs.Reset();return Result;}
            Seen.Add(Pair.Key);if(!Row->bDeprecated && !Row->bTestOnly)IDs.Add(Pair.Key);
        }
    }
    if(!Registered) {Failure(Result,EICDefinitionFailure::TableUnregistered,TEXT("Table"),TEXT("No table registered for definition type."));return Result;}
    IDs.Sort(FNameLexicalLess());Result.bSuccess=true;Result.Code=EICDefinitionFailure::None;
    UE_LOG(LogICData, VeryVerbose, TEXT("[Definition.List] Type=%d Count=%d"),static_cast<int32>(Type),IDs.Num());return Result;
}

#define IC_DEFINITION_GETTER(Name, Type) \
FICDefinitionResult UICDefinitionSubsystem::Get##Name(FName ID,Type& Definition) const \
{ IC_TRACE_FUNCTION(); Definition=Type();FICDefinitionResult Result; if(const Type* Row=Find<Type>(ID,Result))Definition=*Row;return Result; }
IC_DEFINITION_GETTER(Item,FICItemRow)
IC_DEFINITION_GETTER(Recipe,FICRecipeRow)
IC_DEFINITION_GETTER(Production,FICProductionRow)
IC_DEFINITION_GETTER(NPC,FICNPCRow)
IC_DEFINITION_GETTER(Skill,FICSkillRow)
IC_DEFINITION_GETTER(Quest,FICQuestRow)
IC_DEFINITION_GETTER(Equipment,FICEquipmentRow)
IC_DEFINITION_GETTER(NewGame,FICNewGameRow)
IC_DEFINITION_GETTER(Class,FICClassRow)
IC_DEFINITION_GETTER(PlayerPreset,FICPlayerPresetRow)
IC_DEFINITION_GETTER(RecruitCombat,FICRecruitCombatRow)
IC_DEFINITION_GETTER(BossPattern,FICBossPatternRow)
IC_DEFINITION_GETTER(Monster,FICMonsterRow)
IC_DEFINITION_GETTER(SkillEffect,FICSkillEffectRow)
IC_DEFINITION_GETTER(StatusEffect,FICStatusEffectRow)
IC_DEFINITION_GETTER(Synergy,FICSynergyRow)
IC_DEFINITION_GETTER(Enhancement,FICEnhancementRow)
IC_DEFINITION_GETTER(Option,FICOptionRow)
IC_DEFINITION_GETTER(OptionPool,FICOptionPoolRow)
IC_DEFINITION_GETTER(Crop,FICCropRow)
IC_DEFINITION_GETTER(Fish,FICFishRow)
IC_DEFINITION_GETTER(FishingPoint,FICFishingPointRow)
IC_DEFINITION_GETTER(Resource,FICResourceRow)
IC_DEFINITION_GETTER(Dialogue,FICDialogueRow)
IC_DEFINITION_GETTER(GiftPreference,FICGiftPreferenceRow)
IC_DEFINITION_GETTER(Schedule,FICScheduleRow)
IC_DEFINITION_GETTER(Service,FICServiceRow)
IC_DEFINITION_GETTER(Shop,FICShopRow)
IC_DEFINITION_GETTER(Codex,FICCodexRow)
IC_DEFINITION_GETTER(Hint,FICHintRow)
IC_DEFINITION_GETTER(ResourceReference,FICResourceReferenceRow)
IC_DEFINITION_GETTER(Ending,FICEndingRow)
IC_DEFINITION_GETTER(Progression,FICProgressionRow)
IC_DEFINITION_GETTER(IDRegistry,FICIDRegistryRow)
IC_DEFINITION_GETTER(DropTable,FICDropTableRow)
IC_DEFINITION_GETTER(Reward,FICRewardRow)
IC_DEFINITION_GETTER(CombatSettings,FICCombatSettingsRow)
IC_DEFINITION_GETTER(InventorySettings,FICInventorySettingsRow)
IC_DEFINITION_GETTER(MapSettings,FICMapSettingsRow)
IC_DEFINITION_GETTER(PartySettings,FICPartySettingsRow)
IC_DEFINITION_GETTER(SaveSettings,FICSaveSettingsRow)
IC_DEFINITION_GETTER(StatSettings,FICStatSettingsRow)
IC_DEFINITION_GETTER(TimeSettings,FICTimeSettingsRow)
IC_DEFINITION_GETTER(Anchor,FICAnchorRow)
IC_DEFINITION_GETTER(Home,FICHomeRow)
IC_DEFINITION_GETTER(Landmark,FICLandmarkRow)
IC_DEFINITION_GETTER(Map,FICMapRow)
IC_DEFINITION_GETTER(Region,FICRegionRow)
IC_DEFINITION_GETTER(Spawner,FICSpawnerRow)
IC_DEFINITION_GETTER(Warp,FICWarpRow)
#undef IC_DEFINITION_GETTER
