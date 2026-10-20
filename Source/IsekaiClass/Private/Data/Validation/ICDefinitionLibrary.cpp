#include "Data/Validation/ICDefinitionLibrary.h"
#include "Debug/ICLog.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Misc/PackageName.h"
#include "Data/Rows/Items/ICItemRows.h"
#include "Data/Rows/Life/ICRecipeRow.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "Data/Rows/Combat/ICSkillRows.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Data/Rows/Life/ICFishRow.h"
#include "Data/Rows/Character/ICCharacterRows.h"

namespace ICDefinitionValidation
{
    void ValidateCombat(const TArray<FICDefinitionTable>&, bool, TArray<FICDefinitionIssue>&);
    void ValidateNPCWorld(const TArray<FICDefinitionTable>&, bool, TArray<FICDefinitionIssue>&);
    void ValidateLifeProgression(const TArray<FICDefinitionTable>&, bool, TArray<FICDefinitionIssue>&);
    void ValidateItemSettings(const TArray<FICDefinitionTable>&, bool, TArray<FICDefinitionIssue>&);
}

bool UICDefinitionLibrary::IsDefinitionIDValid(FName ID)
{
    IC_TRACE_FUNCTION();
    if (ID.IsNone()) return false;
    const FString Text=ID.ToString();
    if (Text.Len()>40 || Text.IsEmpty()) return false;
    for (TCHAR C : Text) if (!(C>='A' && C<='Z') && !(C>='0' && C<='9') && C!='_') return false;
    return true;
}

TArray<FICDefinitionIssue> UICDefinitionLibrary::GetValidationIssues(const TArray<FICDefinitionTable>& Tables,bool bRequireCompleteContent)
{
    IC_TRACE_FUNCTION();
    TArray<FICDefinitionIssue> Issues;
    ValidateTables(Tables,bRequireCompleteContent,Issues);
    return Issues;
}

TArray<FICDefinitionSchema> UICDefinitionLibrary::GetDefinitionSchemas()
{
    IC_TRACE_FUNCTION();
    TArray<FICDefinitionSchema> Result;
    const UEnum* Types=StaticEnum<EICDefinitionType>();
    for (TObjectIterator<UScriptStruct> It; It; ++It)
    {
        UScriptStruct* Struct=*It;
        if (Struct->GetOutermost()->GetName()!=TEXT("/Script/IsekaiClass") || Struct==FICDefinitionRow::StaticStruct() || !Struct->IsChildOf(FICDefinitionRow::StaticStruct())) continue;
        FString Kind=Struct->GetName();
        if (!Kind.RemoveFromStart(TEXT("IC")) || !Kind.RemoveFromEnd(TEXT("Row"))) continue;
        int64 Value=Types->GetValueByNameString(Kind);
        if (Kind.EndsWith(TEXT("Settings")) || Kind==TEXT("NewGame")) Value=static_cast<int64>(EICDefinitionType::Settings);
        if (Value==INDEX_NONE) continue;
        FICDefinitionSchema Schema; Schema.Type=static_cast<EICDefinitionType>(Value); Schema.RowStruct=Struct;
        Schema.AssetName=FName(*(TEXT("DT_IC_")+Kind));
        FString Group;
        switch (Schema.Type)
        {
        case EICDefinitionType::Settings: Group=TEXT("Settings"); break;
        case EICDefinitionType::PlayerPreset: case EICDefinitionType::Class: case EICDefinitionType::RecruitCombat: Group=TEXT("Character"); break;
        case EICDefinitionType::Skill: case EICDefinitionType::SkillEffect: case EICDefinitionType::StatusEffect: case EICDefinitionType::Synergy: case EICDefinitionType::Monster: case EICDefinitionType::BossPattern: Group=TEXT("Combat"); break;
        case EICDefinitionType::Item: Group=TEXT("Items"); break;
        case EICDefinitionType::Equipment: case EICDefinitionType::Enhancement: case EICDefinitionType::Option: case EICDefinitionType::OptionPool: Group=TEXT("Equipment"); break;
        case EICDefinitionType::Reward: case EICDefinitionType::DropTable: Group=TEXT("Rewards"); break;
        case EICDefinitionType::NPC: case EICDefinitionType::Schedule: case EICDefinitionType::GiftPreference: case EICDefinitionType::Dialogue: case EICDefinitionType::Service: case EICDefinitionType::Shop: Group=TEXT("NPC"); break;
        case EICDefinitionType::Region: case EICDefinitionType::Anchor: case EICDefinitionType::Spawner: case EICDefinitionType::Warp: case EICDefinitionType::Home: case EICDefinitionType::Map: case EICDefinitionType::Landmark: Group=TEXT("World"); break;
        case EICDefinitionType::Recipe: Group=TEXT("Recipes"); break;
        case EICDefinitionType::Production: case EICDefinitionType::Resource: case EICDefinitionType::Crop: case EICDefinitionType::Fish: case EICDefinitionType::FishingPoint: Group=TEXT("Life"); break;
        case EICDefinitionType::Quest: case EICDefinitionType::Progression: case EICDefinitionType::Ending: Group=TEXT("Quest"); break;
        case EICDefinitionType::IDRegistry: Group=TEXT("Registry"); break;
        default: Group=TEXT("Presentation"); break;
        }
        Schema.Folder=TEXT("/Game/IsekaiClass/Data/DataTable/")+Group;
        Result.Add(Schema);
    }
    Result.Sort([](const FICDefinitionSchema& A,const FICDefinitionSchema& B){ IC_TRACE_FUNCTION();return A.AssetName.LexicalLess(B.AssetName);});
    return Result;
}

bool UICDefinitionLibrary::ValidateTables(const TArray<FICDefinitionTable>& Tables,bool bRequireCompleteContent,TArray<FICDefinitionIssue>& Issues)
{
    IC_TRACE_FUNCTION();
    Issues.Reset();
    auto Issue=[&](const UDataTable* Table,FName Row,const FString& Field,const FString& Reason,bool bMissing=false)
    {
        IC_TRACE_FUNCTION();
        FICDefinitionIssue Item;Item.Table=Table?Table->GetPathName():TEXT("<missing table>");Item.Row=Row;Item.Field=Field;Item.Reason=Reason;
        Item.Severity=(bMissing && !bRequireCompleteContent)?EICDefinitionSeverity::Warning:EICDefinitionSeverity::Error;Issues.Add(Item);
    };
    TMap<EICDefinitionType,TSet<FName>> IDs;
    TMap<EICDefinitionType,TSet<FName>> AllIDs;
    TMap<FName,EICDefinitionType> GlobalIDs;
    TSet<const UDataTable*> SeenTables;
    TArray<FICDefinitionTable> ValidTables;
    const auto Schemas=GetDefinitionSchemas();
    for (const FICDefinitionTable& Binding:Tables)
    {
        const UDataTable* Table=Binding.Table;
        if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FICDefinitionRow::StaticStruct()))
        { Issue(Table,NAME_None,TEXT("RowStruct"),TEXT("Expected an IC definition table."));continue; }
        if (SeenTables.Contains(Table)) { Issue(Table,NAME_None,TEXT("Table"),TEXT("Table supplied more than once."));continue; }
        const FICDefinitionSchema* Schema=Schemas.FindByPredicate([Table](const FICDefinitionSchema& S){ IC_TRACE_FUNCTION();return S.RowStruct==Table->GetRowStruct();});
        if (!Schema || Schema->Type!=Binding.Type) { Issue(Table,NAME_None,TEXT("DefinitionType"),TEXT("Table binding type disagrees with RowStruct."));continue; }
        SeenTables.Add(Table);
        ValidTables.Add(Binding);
        for (const auto& Pair:Table->GetRowMap())
        {
            const auto* Row=reinterpret_cast<const FICDefinitionRow*>(Pair.Value);
            if (!Row) { Issue(Table,Pair.Key,TEXT("Row"),TEXT("Null row data."));continue; }
            if (!IsDefinitionIDValid(Row->ID)) Issue(Table,Pair.Key,TEXT("ID"),TEXT("Use A-Z, 0-9, underscore and at most 40 characters."));
            if (Pair.Key!=Row->ID) Issue(Table,Pair.Key,TEXT("ID"),TEXT("RowName and definition ID must match."));
            if (Row->DataVersion<1) Issue(Table,Pair.Key,TEXT("DataVersion"),TEXT("DataVersion must be positive."));
            if (AllIDs.FindOrAdd(Binding.Type).Contains(Row->ID)) Issue(Table,Pair.Key,TEXT("ID"),TEXT("Duplicate ID in the same definition type."));
            AllIDs.FindOrAdd(Binding.Type).Add(Row->ID);
            if(Binding.Type!=EICDefinitionType::IDRegistry)
            {
                const EICDefinitionType* Existing=GlobalIDs.Find(Row->ID);
                if(Existing && *Existing!=Binding.Type) Issue(Table,Pair.Key,TEXT("ID"),TEXT("Definition ID is already owned by another definition type."));
                GlobalIDs.Add(Row->ID,Binding.Type);
            }
            if(!bRequireCompleteContent || (!Row->bTestOnly && !Row->bDeprecated)) IDs.FindOrAdd(Binding.Type).Add(Row->ID);
            if (bRequireCompleteContent && !Row->bDeprecated && !Row->bTestOnly && Binding.Type!=EICDefinitionType::Settings && Binding.Type!=EICDefinitionType::IDRegistry && Row->NameKey.IsEmpty())
                Issue(Table,Pair.Key,TEXT("NameKey"),TEXT("Release definition needs a StringTable name key."));
        }
    }

    const TArray<TPair<FString,EICDefinitionType>> RefSuffixes={
        {TEXT("AppearanceResourceID"),EICDefinitionType::ResourceReference},{TEXT("AnimationResourceID"),EICDefinitionType::ResourceReference},
        {TEXT("ResourceID"),EICDefinitionType::ResourceReference},{TEXT("StatusImmunityID"),EICDefinitionType::StatusEffect},
        {TEXT("StatusEffectID"),EICDefinitionType::StatusEffect},{TEXT("SkillEffectID"),EICDefinitionType::SkillEffect},
        {TEXT("RecruitCombatID"),EICDefinitionType::RecruitCombat},{TEXT("GiftPreferenceID"),EICDefinitionType::GiftPreference},
        {TEXT("FishingPointID"),EICDefinitionType::FishingPoint},{TEXT("BossPatternID"),EICDefinitionType::BossPattern},
        {TEXT("DropTableID"),EICDefinitionType::DropTable},{TEXT("OptionPoolID"),EICDefinitionType::OptionPool},
        {TEXT("EnhancementID"),EICDefinitionType::Enhancement},{TEXT("ProductionID"),EICDefinitionType::Production},
        {TEXT("ProgressionID"),EICDefinitionType::Progression},{TEXT("EquipmentID"),EICDefinitionType::Equipment},
        {TEXT("DialogueID"),EICDefinitionType::Dialogue},{TEXT("ScheduleID"),EICDefinitionType::Schedule},
        {TEXT("LandmarkID"),EICDefinitionType::Landmark},{TEXT("MonsterID"),EICDefinitionType::Monster},
        {TEXT("FinalBossID"),EICDefinitionType::Monster},{TEXT("ServiceID"),EICDefinitionType::Service},
        {TEXT("SpawnerID"),EICDefinitionType::Spawner},{TEXT("WarpID"),EICDefinitionType::Warp},
        {TEXT("RegionID"),EICDefinitionType::Region},{TEXT("AnchorID"),EICDefinitionType::Anchor},
        {TEXT("RewardID"),EICDefinitionType::Reward},{TEXT("RecipeID"),EICDefinitionType::Recipe},
        {TEXT("OptionID"),EICDefinitionType::Option},{TEXT("QuestID"),EICDefinitionType::Quest},
        {TEXT("SkillID"),EICDefinitionType::Skill},{TEXT("ClassID"),EICDefinitionType::Class},
        {TEXT("NPCID"),EICDefinitionType::NPC},{TEXT("HomeID"),EICDefinitionType::Home},
        {TEXT("ShopID"),EICDefinitionType::Shop},{TEXT("ItemID"),EICDefinitionType::Item},
        {TEXT("CropID"),EICDefinitionType::Crop},{TEXT("FishID"),EICDefinitionType::Fish},
        {TEXT("CodexID"),EICDefinitionType::Codex},{TEXT("MapID"),EICDefinitionType::Map}
    };
    auto CheckRef=[&](const UDataTable* T,FName Row,const FString& Field,EICDefinitionType Type,FName ID)
    {
        IC_TRACE_FUNCTION();
        const UEnum* Enum=StaticEnum<EICDefinitionType>();
        const int32 TypeIndex=Enum->GetIndexByValue(static_cast<int64>(Type));
        if(TypeIndex==INDEX_NONE || Enum->GetNameStringByIndex(TypeIndex).EndsWith(TEXT("_MAX"))) { Issue(T,Row,Field,TEXT("Invalid referenced definition type."));return; }
        if (ID.IsNone()) return;
        if (!IsDefinitionIDValid(ID)) { Issue(T,Row,Field,TEXT("Malformed referenced definition ID."));return; }
        const TSet<FName>* Known=IDs.Find(Type);
        if (!Known || !Known->Contains(ID)) Issue(T,Row,Field,FString::Printf(TEXT("Unresolved %s reference: %s"),*StaticEnum<EICDefinitionType>()->GetNameStringByValue(static_cast<int64>(Type)),*ID.ToString()),true);
    };
    TFunction<void(const UDataTable*,FName,const UStruct*,const void*,const FString&)> Inspect;
    TFunction<void(const UDataTable*,FName,FProperty*,const void*,const FString&,const FString&)> InspectProperty;
    InspectProperty=[&](const UDataTable* T,FName Row,FProperty* P,const void* Ptr,const FString& Path,const FString& RefName)
    {
        IC_TRACE_FUNCTION();
        if(const auto* Enum=CastField<FEnumProperty>(P))
        {
            const int64 Value=Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Ptr);
            const int32 Index=Enum->GetEnum()->GetIndexByValue(Value);
            if(Index==INDEX_NONE || Enum->GetEnum()->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX"))) Issue(T,Row,Path,TEXT("Invalid enum value."));
        }
        else if (const auto* Number=CastField<FNumericProperty>(P))
        {
            const double Value=Number->IsFloatingPoint()?Number->GetFloatingPointPropertyValue(Ptr):static_cast<double>(Number->GetSignedIntPropertyValue(Ptr));
            if (!FMath::IsFinite(Value)) Issue(T,Row,Path,TEXT("Numeric value must be finite."));
#if WITH_EDITOR
            if (Number->HasMetaData(TEXT("ClampMin")) && Value<FCString::Atod(*Number->GetMetaData(TEXT("ClampMin")))) Issue(T,Row,Path,TEXT("Value is below ClampMin."));
            if (Number->HasMetaData(TEXT("ClampMax")) && Value>FCString::Atod(*Number->GetMetaData(TEXT("ClampMax")))) Issue(T,Row,Path,TEXT("Value exceeds ClampMax."));
#endif
        }
        else if (const auto* Name=CastField<FNameProperty>(P))
        {
            FString Singular=RefName;if(Singular.EndsWith(TEXT("IDs"))) Singular.LeftChopInline(1);
            for (const auto& Suffix:RefSuffixes) if (Singular.EndsWith(Suffix.Key)) { CheckRef(T,Row,Path,Suffix.Value,Name->GetPropertyValue(Ptr));break; }
        }
        else if (const auto* Soft=CastField<FSoftObjectProperty>(P))
        {
            const FSoftObjectPath Asset=Soft->GetPropertyValue(Ptr).ToSoftObjectPath();
            if(!Asset.IsNull())
            {
                UObject* Loaded=Asset.IsValid()?Asset.TryLoad():nullptr;
                if(!Loaded) Issue(T,Row,Path,TEXT("Referenced asset cannot be loaded."));
                else if(const auto* Class=CastField<FSoftClassProperty>(P))
                {
                    const UClass* LoadedClass=Cast<UClass>(Loaded);
                    if(!LoadedClass || !LoadedClass->IsChildOf(Class->MetaClass)) Issue(T,Row,Path,TEXT("Referenced class has an incompatible parent."));
                }
                else if(!Loaded->IsA(Soft->PropertyClass)) Issue(T,Row,Path,TEXT("Referenced asset has an incompatible class."));
            }
        }
        else if (const auto* Struct=CastField<FStructProperty>(P)) Inspect(T,Row,Struct->Struct,Ptr,Path);
        else if (const auto* Array=CastField<FArrayProperty>(P))
        {
            FScriptArrayHelper Helper(Array,Ptr);
            for(int32 I=0;I<Helper.Num();++I) InspectProperty(T,Row,Array->Inner,Helper.GetRawPtr(I),FString::Printf(TEXT("%s[%d]"),*Path,I),RefName);
        }
    };
    Inspect=[&](const UDataTable* T,FName Row,const UStruct* Struct,const void* Data,const FString& Path)
    {
        IC_TRACE_FUNCTION();
        if(Struct==FICDefinitionRef::StaticStruct())
        {const auto* Ref=static_cast<const FICDefinitionRef*>(Data);CheckRef(T,Row,Path,Ref->Type,Ref->ID);return;}
        if(Struct==TBaseStructure<FSoftObjectPath>::Get())
        {const auto* Ref=static_cast<const FSoftObjectPath*>(Data);if(!Ref->IsNull()&&(!Ref->IsValid()||!Ref->TryLoad()))Issue(T,Row,Path,TEXT("Referenced asset cannot be loaded."));return;}
        for(TFieldIterator<FProperty> It(Struct);It;++It)
        {
            FProperty* P=*It;const FString Field=P->GetName();

            if(Path.IsEmpty() && ((Field==TEXT("RecipeID")&&Struct->IsChildOf(FPlayerRecipeDefinition::StaticStruct())) || (Field==TEXT("ItemID")&&Struct->IsChildOf(FPlayerItemDefinition::StaticStruct()))))continue;
            FString RefField=Field;
            if(Struct->GetFName()==TEXT("ICSkillHitDefinition") && Field==TEXT("EffectID")) RefField=TEXT("SkillEffectID");
            if(Struct->GetFName()==TEXT("ICBossPatternChoice") && Field==TEXT("PatternID")) RefField=TEXT("BossPatternID");
            if(Struct->GetFName()==TEXT("ICInitialEquipment") || Field==TEXT("StartingWeaponID") || Field==TEXT("StartingClothesID")) RefField=TEXT("EquipmentID");
            if(Field.StartsWith(TEXT("SkillSlot")) && Field.EndsWith(TEXT("ID"))) RefField=TEXT("SkillID");
            InspectProperty(T,Row,P,P->ContainerPtrToValuePtr<void>(Data),Path.IsEmpty()?Field:Path+TEXT(".")+Field,RefField);
        }
    };
    for(const FICDefinitionTable& Binding:ValidTables)
    {
        const UDataTable* T=Binding.Table;if(!T||!T->GetRowStruct()||!T->GetRowStruct()->IsChildOf(FICDefinitionRow::StaticStruct()))continue;
        for(const auto& Pair:T->GetRowMap())
        {
            const auto* Row=reinterpret_cast<const FICDefinitionRow*>(Pair.Value);if(!Row||Row->bDeprecated||Row->bTestOnly)continue;
            Inspect(T,Pair.Key,T->GetRowStruct(),Pair.Value,FString());
            if(T->GetRowStruct()->IsChildOf(FICItemRow::StaticStruct()))
            {
                const auto* Item=reinterpret_cast<const FICItemRow*>(Pair.Value);
                if(Item->ItemID!=Item->ID)Issue(T,Pair.Key,TEXT("ItemID"),TEXT("Legacy ItemID must equal ID."));
                if(Item->StackLimit<1||Item->StackLimit>99||Item->BuyPrice<0||Item->SellPrice<0)Issue(T,Pair.Key,TEXT("StackLimit/Price"),TEXT("Invalid stack count or negative price."));
                if(Item->Kind==EPlayerItemKind::Equipment && (Item->StackLimit!=1||Item->EquipSlot==EPlayerEquipSlot::None))Issue(T,Pair.Key,TEXT("EquipSlot"),TEXT("Equipment requires one item per slot and an equipment slot."));
                if(Item->Tab==EPlayerItemTab::Quest && (Item->bCanDiscard||Item->bCanSell))Issue(T,Pair.Key,TEXT("Quest"),TEXT("Quest items cannot be discarded or sold."));
                if(Item->Category==EPlayerItemCategory::Book && (Item->GrowthStat==EPlayerMainStat::None||!(Item->GrowthAmount==1||Item->GrowthAmount==3||Item->GrowthAmount==5)))Issue(T,Pair.Key,TEXT("GrowthAmount"),TEXT("Growth books require one stat and +1/+3/+5."));
            }
            if(T->GetRowStruct()->IsChildOf(FICRecipeRow::StaticStruct()))
            {
                const auto* Recipe=reinterpret_cast<const FICRecipeRow*>(Pair.Value);
                if(Recipe->RecipeID!=Recipe->ID)Issue(T,Pair.Key,TEXT("RecipeID"),TEXT("Legacy RecipeID must equal ID."));
                if(Recipe->Ingredients.IsEmpty())Issue(T,Pair.Key,TEXT("Ingredients"),TEXT("Recipe needs structured ingredients."),true);
                if(Recipe->ResultKind==EICProductionResultKind::BagItems&&Recipe->Results.IsEmpty())Issue(T,Pair.Key,TEXT("Results"),TEXT("Bag recipe needs results."),true);
                if(Recipe->ResultKind==EICProductionResultKind::InstallStorage&&Recipe->InstallationAnchorIDs.IsEmpty())Issue(T,Pair.Key,TEXT("InstallationAnchorIDs"),TEXT("Storage recipe needs fixed installation anchors."),true);
            }
        }
    }
    ICDefinitionValidation::ValidateCombat(ValidTables,bRequireCompleteContent,Issues);
    ICDefinitionValidation::ValidateNPCWorld(ValidTables,bRequireCompleteContent,Issues);
    ICDefinitionValidation::ValidateLifeProgression(ValidTables,bRequireCompleteContent,Issues);
    ICDefinitionValidation::ValidateItemSettings(ValidTables,bRequireCompleteContent,Issues);
    if(bRequireCompleteContent)
    {
        for(const FICDefinitionSchema& Schema:Schemas)
        {
            const auto* Binding=ValidTables.FindByPredicate([&](const FICDefinitionTable& B){ IC_TRACE_FUNCTION();return B.Table->GetRowStruct()==Schema.RowStruct;});
            if(!Binding) Issue(nullptr,NAME_None,Schema.AssetName.ToString(),TEXT("Required definition table was not supplied."));
            else if(Schema.Type==EICDefinitionType::Settings && Binding->Table->GetRowMap().Num()!=1) Issue(Binding->Table,NAME_None,TEXT("Rows"),TEXT("Each settings table requires exactly one authoritative configuration."));
        }
        auto RequireCount=[&](EICDefinitionType Type,int32 Expected)
        {
            IC_TRACE_FUNCTION();
            int32 Count=0;for(const auto& B:ValidTables)if(B.Type==Type&&B.Table)for(const auto& P:B.Table->GetRowMap()){const auto* R=reinterpret_cast<const FICDefinitionRow*>(P.Value);if(R&&!R->bDeprecated&&!R->bTestOnly)++Count;}
            if(Count!=Expected)Issue(nullptr,NAME_None,StaticEnum<EICDefinitionType>()->GetNameStringByValue(static_cast<int64>(Type)),FString::Printf(TEXT("Release requires %d definitions; found %d."),Expected,Count));
        };
        RequireCount(EICDefinitionType::Region,8);RequireCount(EICDefinitionType::NPC,52);RequireCount(EICDefinitionType::RecruitCombat,24);RequireCount(EICDefinitionType::Class,4);RequireCount(EICDefinitionType::Fish,20);RequireCount(EICDefinitionType::Landmark,8);
        RequireCount(EICDefinitionType::PlayerPreset,2);RequireCount(EICDefinitionType::Ending,1);
        int32 Recruits=0,Residents=0,Backgrounds=0,ClassSkills=0,UniqueSkills=0,Bosses=0,RegionalBosses=0,FinalBosses=0,Books=0,MalePresets=0,FemalePresets=0;
        TMap<FName,int32> FishPerRegion,BossesPerRegion,SkillsPerClass;TMap<EPlayerMainStat,TSet<int32>> BookGrowths;
        TSet<EICCharacterClass> ClassKinds;
        for(const auto& B:ValidTables)if(B.Table)for(const auto& P:B.Table->GetRowMap())
        {
            const auto* R=reinterpret_cast<const FICDefinitionRow*>(P.Value);if(!R||R->bDeprecated||R->bTestOnly)continue;
            if(B.Table->GetRowStruct()->IsChildOf(FICNPCRow::StaticStruct())){const auto* N=reinterpret_cast<const FICNPCRow*>(P.Value);if(N->Type==EICNPCType::Recruitable)++Recruits;else if(N->Type==EICNPCType::Resident)++Residents;else ++Backgrounds;}
            if(B.Table->GetRowStruct()->IsChildOf(FICClassRow::StaticStruct())){const auto* C=reinterpret_cast<const FICClassRow*>(P.Value);ClassKinds.Add(C->CharacterClass);}
            if(B.Table->GetRowStruct()->IsChildOf(FICPlayerPresetRow::StaticStruct())){const auto* C=reinterpret_cast<const FICPlayerPresetRow*>(P.Value);if(C->Gender==EICPlayerPresetGender::Male)++MalePresets;else ++FemalePresets;}
            if(B.Table->GetRowStruct()->IsChildOf(FICSkillRow::StaticStruct())){const auto* S=reinterpret_cast<const FICSkillRow*>(P.Value);if(S->OwnerType==EICSkillOwner::CharacterClass){++ClassSkills;++SkillsPerClass.FindOrAdd(S->ClassID);}else if(S->OwnerType==EICSkillOwner::RecruitUnique)++UniqueSkills;}
            if(B.Table->GetRowStruct()->IsChildOf(FICMonsterRow::StaticStruct())){const auto* M=reinterpret_cast<const FICMonsterRow*>(P.Value);if(M->Kind!=EICMonsterKind::Normal)++Bosses;if(M->Kind==EICMonsterKind::RegionalBoss){++RegionalBosses;++BossesPerRegion.FindOrAdd(M->RegionID);}else if(M->Kind==EICMonsterKind::FinalBoss)++FinalBosses;}
            if(B.Table->GetRowStruct()->IsChildOf(FICFishRow::StaticStruct())){const auto* F=reinterpret_cast<const FICFishRow*>(P.Value);++FishPerRegion.FindOrAdd(F->RegionID);}
            if(B.Table->GetRowStruct()->IsChildOf(FICItemRow::StaticStruct())){const auto* I=reinterpret_cast<const FICItemRow*>(P.Value);if(I->Category==EPlayerItemCategory::Book){++Books;BookGrowths.FindOrAdd(I->GrowthStat).Add(I->GrowthAmount);}}
        }
        if(Recruits!=24||Residents!=16||Backgrounds!=12)Issue(nullptr,NAME_None,TEXT("NPC.Types"),TEXT("Release NPC split must be 24 recruitable / 16 resident / 12 background."));
        if(ClassSkills!=16||UniqueSkills!=24)Issue(nullptr,NAME_None,TEXT("Skill.Owners"),TEXT("Release requires 16 class skills and 24 unique recruit skills."));
        if(Bosses!=9||RegionalBosses!=8||FinalBosses!=1)Issue(nullptr,NAME_None,TEXT("Monster.Boss"),TEXT("Release requires 8 region bosses and one final boss."));
        if(ClassKinds.Num()!=4||ClassKinds.Contains(EICCharacterClass::None))Issue(nullptr,NAME_None,TEXT("Class.Types"),TEXT("Release requires each of Warrior, Archer, Mage and Healer exactly once."));
        if(MalePresets!=1||FemalePresets!=1)Issue(nullptr,NAME_None,TEXT("PlayerPreset.Gender"),TEXT("Release requires one male and one female player appearance."));
        for(const auto& C:SkillsPerClass)if(C.Value!=4)Issue(nullptr,NAME_None,TEXT("Skill.ClassID"),TEXT("Each class requires exactly four skills."));
        if(Books!=18||BookGrowths.Num()!=6)Issue(nullptr,NAME_None,TEXT("Item.Books"),TEXT("Release requires three growth books per each of six main stats."));
        for(const auto& G:BookGrowths)if(G.Value.Num()!=3)Issue(nullptr,NAME_None,TEXT("Item.Books"),TEXT("Each main stat requires +1, +3 and +5 books."));
        for(int32 I=1;I<=8;++I){const FName Region(*FString::Printf(TEXT("R%02d"),I));if(FishPerRegion.FindRef(Region)!=(I<=4?3:2))Issue(nullptr,NAME_None,TEXT("Fish.RegionID"),TEXT("Fish distribution must be 3 per R01-R04 and 2 per R05-R08."));if(BossesPerRegion.FindRef(Region)!=1)Issue(nullptr,NAME_None,TEXT("Monster.RegionID"),TEXT("Each of R01-R08 requires exactly one regional boss."));}
    }
    int32 ErrorCount=0;
    for (const FICDefinitionIssue& ValidationIssue : Issues)
    {
        ErrorCount += ValidationIssue.Severity==EICDefinitionSeverity::Error ? 1 : 0;
        UE_LOG(LogICData, Log, TEXT("[Validation.Issue] Severity=%s Table=%s Row=%s Field=%s Reason=%s"),
            ValidationIssue.Severity==EICDefinitionSeverity::Error?TEXT("Error"):TEXT("Warning"),
            *ValidationIssue.Table,*ValidationIssue.Row.ToString(),*ValidationIssue.Field,*ValidationIssue.Reason);
    }

    UE_LOG(LogICData, Log, TEXT("[Validation.Complete] Tables=%d CompleteContent=%d Errors=%d Warnings=%d"),
        Tables.Num(),bRequireCompleteContent,ErrorCount,Issues.Num()-ErrorCount);
    return ErrorCount==0;
}
