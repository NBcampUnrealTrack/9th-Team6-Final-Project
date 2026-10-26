#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Debug/ICLog.h"
#include "Engine/GameInstance.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Validation/ICDefinitionLibrary.h"
#include "Internationalization/StringTable.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Inventory/Use/ICItemUseService.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Life/Production/ICProductionSubsystem.h"
#include "Save/ICSaveGame.h"

namespace
{
struct FRuntimeFixture
{
    UGameInstance* GI=NewObject<UGameInstance>();
    UICDefinitionCatalog* Catalog=NewObject<UICDefinitionCatalog>();
    UICDefinitionSubsystem* Definitions=NewObject<UICDefinitionSubsystem>(GI);
    UICWorldStateSubsystem* State=NewObject<UICWorldStateSubsystem>(GI);
    UICTimeSubsystem* Time=NewObject<UICTimeSubsystem>(GI);
    UICProductionSubsystem* Production=NewObject<UICProductionSubsystem>(GI);
    UICInventoryComponent* Inventory=NewObject<UICInventoryComponent>();
    UDataTable* Items=nullptr;
    template<class T> UDataTable* Table(EICDefinitionType Type,const TArray<T>& Rows)
    {
        IC_TRACE_FUNCTION();
        auto* Data=NewObject<UDataTable>();Data->RowStruct=T::StaticStruct();
        for(const auto& Row:Rows)Data->AddRow(Row.ID,Row);
        FICDefinitionBinding Binding;Binding.Type=Type;Binding.RowStruct=T::StaticStruct();Binding.Table=Data;Catalog->Tables.Add(Binding);return Data;
    }
    FRuntimeFixture()
    {
        IC_TRACE_FUNCTION();
        FICItemRow Material;Material.ID=Material.ItemID=TEXT("ITM_MAT_998");
        FICItemRow Potion;Potion.ID=Potion.ItemID=TEXT("ITM_CONS_998");Potion.Kind=EPlayerItemKind::Consumable;Potion.UseEffect=EICItemEffect::Recovery;Potion.UseTarget=EICItemTarget::LivingParty;Potion.RestoreHP=25;
        FICItemRow Bread;Bread.ID=Bread.ItemID=TEXT("ITM_FOOD_998");Bread.Kind=EPlayerItemKind::Consumable;Bread.UseEffect=EICItemEffect::Recovery;Bread.UseTarget=EICItemTarget::LivingParty;Bread.RestoreSP=5;
        Items=Table<FICItemRow>(EICDefinitionType::Item,{Material,Potion,Bread});
        FICStatSettingsRow Stat;Stat.ID=TEXT("CFG_STAT_001");Table<FICStatSettingsRow>(EICDefinitionType::Settings,{Stat});
        FICRecipeRow Recipe;Recipe.ID=Recipe.RecipeID=TEXT("RCP_COOK_998");Recipe.ProductionID=TEXT("PRD_COOK_998");Recipe.SPCost=2;Recipe.WorkDurationPlaySeconds=5;
        FICItemAmount Cost;Cost.ItemID=Material.ID;Cost.Count=2;Recipe.Ingredients.Add(Cost);
        FICItemAmount Result;Result.ItemID=Bread.ID;Recipe.Results.Add(Result);Table<FICRecipeRow>(EICDefinitionType::Recipe,{Recipe});
        FICProductionRow Work;Work.ID=Recipe.ProductionID;Work.SPCost=2;Work.Activity=EICLifeActivity::Cooking;Table<FICProductionRow>(EICDefinitionType::Production,{Work});
        FICAnchorRow Anchor;Anchor.ID=TEXT("ANC_KITCHEN_998");Anchor.Type=EICAnchorType::Kitchen;Table<FICAnchorRow>(EICDefinitionType::Anchor,{Anchor});
        Definitions->ConfigureCatalog(Catalog);Inventory->BeginStaging(Definitions,0);
        Inventory->AddItem(Material.ID,4,false);Inventory->AddItem(Potion.ID,2,false);
        FICWorldState World;World.bPlayerCreated=true;World.PlayerName=TEXT("검증");World.FirstSkill=TEXT("SKL_FIXTURE");World.Inventory=Inventory->GetSnapshot();
        FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;World.Characters.Add(Player.CharacterId,Player);
        State->RestoreState(World);Inventory->BindWorldState(State);
        Time->SetRunning(true);Time->SetMenuPaused(false);Production->ConfigureServices(Definitions,State,Time);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICDefinitionFailureCheck,"IC.Runtime.DefinitionFailures",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICDefinitionFailureCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    FRuntimeFixture F;FICItemRow Out;FICDefinitionResult R=F.Definitions->GetItem(TEXT("ITM_CONS_998"),Out);
    TestTrue(TEXT("Registered real row resolves"),R.bSuccess);TestEqual(TEXT("Output ID"),Out.ID,FName(TEXT("ITM_CONS_998")));
    R=F.Definitions->GetItem(TEXT("PotionHP"),Out);TestFalse(TEXT("Legacy alias never resolves at runtime"),R.bSuccess);TestTrue(TEXT("Failed output cleared"),Out.ID.IsNone());
    R=F.Definitions->GetItem(TEXT("ITM_CONS_999"),Out);TestEqual(TEXT("Missing row code"),R.Code,EICDefinitionFailure::RowMissing);
    TestFalse(TEXT("Missing row cannot grant fallback items"),F.Inventory->AddItem(TEXT("ITM_CONS_999"),1,false));
    TestEqual(TEXT("Existing inventory preserved"),F.Inventory->CountItem(TEXT("ITM_CONS_998")),2);
    FICItemRow Test;Test.ID=Test.ItemID=TEXT("ITM_CONS_997");Test.bTestOnly=true;F.Items->AddRow(Test.ID,Test);
    R=F.Definitions->GetItem(Test.ID,Out);TestEqual(TEXT("Test content blocked"),R.Code,EICDefinitionFailure::TestDefinition);
    Test.bTestOnly=false;Test.bDeprecated=true;F.Items->AddRow(Test.ID,Test);
    R=F.Definitions->GetItem(Test.ID,Out);TestEqual(TEXT("Deprecated content blocked"),R.Code,EICDefinitionFailure::Deprecated);
    const FICDefinitionBinding Duplicate=F.Catalog->Tables[0];F.Catalog->Tables.Add(Duplicate);R=F.Definitions->GetItem(TEXT("ITM_CONS_998"),Out);
    TestEqual(TEXT("Duplicate catalog ID blocked"),R.Code,EICDefinitionFailure::DuplicateID);
    F.Catalog->Tables.RemoveAt(F.Catalog->Tables.Num()-1);
    F.Catalog->Tables[0].RowStruct=FICRecipeRow::StaticStruct();R=F.Definitions->GetItem(TEXT("ITM_CONS_998"),Out);
    TestEqual(TEXT("Wrong table schema blocked"),R.Code,EICDefinitionFailure::WrongRowStruct);
    F.Definitions->ConfigureCatalog(nullptr);R=F.Definitions->GetItem(TEXT("ITM_CONS_998"),Out);
    TestEqual(TEXT("Unconfigured catalog fails explicitly"),R.Code,EICDefinitionFailure::CatalogUnavailable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICItemAtomicCheck,"IC.Runtime.ItemUseAtomic",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICItemAtomicCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    FRuntimeFixture F;const auto Target=F.State->GetSnapshot(UICWorldStateSubsystem::PlayerId());
    auto Prepared=UICItemUseService::Prepare(F.Inventory,F.Definitions,TEXT("ITM_CONS_998"),Target);
    TestTrue(TEXT("Recovery prepared"),Prepared.bSuccess);TestEqual(TEXT("Prepared recovery"),Prepared.Character.HP,85.f);
    TestEqual(TEXT("Preparation does not mutate HP"),F.State->GetSnapshot(Target.CharacterId).HP,60.f);
    TestEqual(TEXT("Preparation does not consume"),F.Inventory->CountItem(TEXT("ITM_CONS_998")),2);
    FICWorldState Next=F.State->ExportState();Next.Inventory=Prepared.Inventory;Next.Characters.Add(Target.CharacterId,Prepared.Character);const FGuid Transaction=FGuid::NewGuid();
    TestTrue(TEXT("Effects and costs commit together"),F.State->CommitGameplayTransaction(Next,Transaction));
    TestEqual(TEXT("One item consumed"),F.Inventory->CountItem(TEXT("ITM_CONS_998")),1);
    TestFalse(TEXT("Same transaction cannot commit twice"),F.State->CommitGameplayTransaction(Next,Transaction));
    TestEqual(TEXT("HP remains applied once"),F.State->GetSnapshot(Target.CharacterId).HP,85.f);
    FICItemRow Broken=*F.Items->FindRow<FICItemRow>(TEXT("ITM_CONS_998"),TEXT("Fixture"));Broken.StatusEffectIDs.Add(TEXT("STE_MISSING"));F.Items->AddRow(Broken.ID,Broken);
    Prepared=UICItemUseService::Prepare(F.Inventory,F.Definitions,Broken.ID,F.State->GetSnapshot(Target.CharacterId));
    TestFalse(TEXT("Missing effect definition blocks all changes"),Prepared.bSuccess);TestEqual(TEXT("No partial consumption"),F.Inventory->CountItem(Broken.ID),1);
    TestEqual(TEXT("No partial healing"),F.State->GetSnapshot(Target.CharacterId).HP,85.f);
    FICInventoryState Invalid=F.Inventory->GetSnapshot();Invalid.Bag.Slots[0].Count=-1;Next=F.State->ExportState();Next.Inventory=Invalid;Next.Gold=0;
    TestFalse(TEXT("Invalid inventory rejects whole transaction"),F.State->CommitGameplayTransaction(Next,FGuid::NewGuid()));
    TestEqual(TEXT("Gold cannot partially change"),F.State->ExportState().Gold,200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICProductionLifecycleCheck,"IC.Runtime.ProductionLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICProductionLifecycleCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    FRuntimeFixture F;FGuid Job;FString Reason;
    if(!TestTrue(FString::Printf(TEXT("Real structured recipe starts: %s"),*Reason),F.Production->StartRecipe(TEXT("RCP_COOK_998"),TEXT("ANC_KITCHEN_998"),F.Inventory,Job,Reason)))
    { AddError(Reason);return false; }
    TestEqual(TEXT("Materials reserved without consumption"),F.Inventory->CountItem(TEXT("ITM_MAT_998")),4);
    TestEqual(TEXT("Start SP cost committed once"),F.State->GetSnapshot(UICWorldStateSubsystem::PlayerId()).SP,68.f);
    TestFalse(TEXT("Reserved materials cannot be sold or consumed"),F.Inventory->Consume(TEXT("ITM_MAT_998"),4));
    TestEqual(TEXT("Reservation failure leaves inventory intact"),F.Inventory->CountItem(TEXT("ITM_MAT_998")),4);
    auto* Save=NewObject<UICSaveGame>();Save->WorldState=F.State->ExportState();Save->Clock=F.Time->GetClock();TArray<uint8> Bytes;
    TestTrue(TEXT("Running work can serialize"),UICSaveGame::EncodeForFile(Save,Bytes));auto* Loaded=UICSaveGame::DecodeFile(Bytes);
    TestNotNull(TEXT("Running work loads"),Loaded);if(!Loaded)return false;
    TestTrue(TEXT("Restore clock"),F.Time->RestoreClock(Loaded->Clock));TestTrue(TEXT("Restore work"),F.State->RestoreState(Loaded->WorldState));
    UObject* Menu=NewObject<UGameInstance>();F.Time->SetPauseRequest(Menu,TEXT("Menu"),true);F.Time->AdvanceClock(10,false);
    TestEqual(TEXT("Menu freezes work time"),F.Time->GetClock().PlaySecond,0.0);F.Time->ReleasePauseRequests(Menu);
    F.Time->AdvanceClock(5,false);
    const auto CompletedState=F.State->ExportState();const auto* CompletedJob=CompletedState.ProductionJobs.Find(Job);
    if(!TestNotNull(TEXT("Persistent job survives restore"),CompletedJob))return false;
    TestEqual(TEXT("Completed status"),CompletedJob->Status,EICProductionJobStatus::Completed);
    TestEqual(TEXT("Reserved ingredients consumed once"),F.Inventory->CountItem(TEXT("ITM_MAT_998")),2);TestEqual(TEXT("Real output granted"),F.Inventory->CountItem(TEXT("ITM_FOOD_998")),1);
    TestFalse(TEXT("Completion cannot repeat"),F.Production->CompleteJob(Job,F.Inventory,Reason));TestEqual(TEXT("No duplicate reward"),F.Inventory->CountItem(TEXT("ITM_FOOD_998")),1);
    TestTrue(TEXT("Another task starts"),F.Production->StartRecipe(TEXT("RCP_COOK_998"),TEXT("ANC_KITCHEN_998"),F.Inventory,Job,Reason));
    TestTrue(TEXT("Cancellation releases materials"),F.Production->CancelJob(Job));TestTrue(TEXT("Released materials can be used"),F.Inventory->Consume(TEXT("ITM_MAT_998"),2));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICRealDefinitionAssetsCheck,"IC.Runtime.RealAssetBindings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICRealDefinitionAssetsCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    auto* Catalog=GetDefault<UICDefinitionSettings>()->Catalog.LoadSynchronous();TestNotNull(TEXT("Production catalog is configured"),Catalog);if(!Catalog)return false;
    TestEqual(TEXT("Catalog resolves from DataAsset folder"),Catalog->GetPathName(),
        FString(TEXT("/Game/IsekaiClass/Data/DataAsset/DA_IC_DefinitionCatalog.DA_IC_DefinitionCatalog")));
    TestEqual(TEXT("All native schemas registered"),Catalog->Tables.Num(),51);
    for(const auto& Schema:UICDefinitionLibrary::GetDefinitionSchemas())
    {
        const auto* Binding=Catalog->Tables.FindByPredicate([&](const auto& B){ IC_TRACE_FUNCTION();return B.Type==Schema.Type&&B.RowStruct==Schema.RowStruct;});
        if(!TestNotNull(Schema.AssetName.ToString()+TEXT(" catalog registration"),Binding))continue;
        UDataTable* Table=Binding->Table.LoadSynchronous();
        if(!TestNotNull(Schema.AssetName.ToString()+TEXT(" table reference"),Table))continue;
        TestTrue(TEXT("Schema uses DataTable folder"),Schema.Folder.StartsWith(TEXT("/Game/IsekaiClass/Data/DataTable/")));
        TestEqual(TEXT("Registered table matches schema path"),Table->GetPathName(),
            Schema.Folder+TEXT("/")+Schema.AssetName.ToString()+TEXT(".")+Schema.AssetName.ToString());
        TestTrue(TEXT("Registered table retains row struct"),Table->GetRowStruct()==Schema.RowStruct.Get());
    }
    TestEqual(TEXT("Definition, quest, dialogue and UI String Tables registered"),Catalog->StringTables.Num(),4);
    bool IsQuestTableRegistered = false;
    for(const auto& Reference:Catalog->StringTables)
    {
        UStringTable* Strings=Reference.LoadSynchronous();
        if(TestNotNull(TEXT("Catalog String Table reference resolves"),Strings))
        {
            TestTrue(TEXT("String Table reference uses new folder"),Strings->GetPathName().StartsWith(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/")));
            IsQuestTableRegistered |= Strings->GetPathName()==TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Quest.ST_IC_Quest");
        }
    }
    TestTrue(TEXT("Quest String Table is registered"),IsQuestTableRegistered);
    auto* GI=NewObject<UGameInstance>();auto* Service=NewObject<UICDefinitionSubsystem>(GI);Service->ConfigureCatalog(Catalog);
    FICItemRow Potion;
    if(TestTrue(TEXT("Item resolves after catalog relocation"),Service->GetItem(TEXT("ITM_CONS_001"),Potion).bSuccess))
    {
        FName TableID;FString TextKey;
        TestTrue(TEXT("Localized item name retains String Table identity"),FTextInspector::GetTableIdAndKey(Potion.DisplayName,TableID,TextKey));
        TestEqual(TEXT("Localized name uses relocated String Table"),TableID,
            FName(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions")));
        TestEqual(TEXT("Localized name key preserved"),TextKey,FString(TEXT("ITM_CONS_001.NAME")));
    }
    FICNewGameRow Rules;TestTrue(TEXT("Actual new-game definition resolves"),Service->GetNewGame(TEXT("CFG_NEWGAME_001"),Rules).bSuccess);
    TestEqual(TEXT("Starting gold preserved"),Rules.InitialGold,200);TestEqual(TEXT("Starting time preserved"),Rules.InitialGameMinute,480.0);
    TestEqual(TEXT("Initial HP preserved"),Rules.InitialHP,60.0);TestEqual(TEXT("Initial SP preserved"),Rules.InitialSP,70.0);
    for(int32 Class=0;Class<4;++Class)
    {
        UICInventoryComponent* Inventory=NewObject<UICInventoryComponent>();Inventory->BeginStaging(Service,Class);
        TestTrue(TEXT("Starting weapon resolves and grants"),Rules.StartingWeapons.IsValidIndex(Class)&&Inventory->AddItem(Rules.StartingWeapons[Class],1,false));
        TestTrue(TEXT("Class weapon equips"),Inventory->Equip(0));
        for(const auto& Item:Rules.StartingItems)TestTrue(TEXT("Starting item resolves and grants"),Inventory->AddItem(Item.ItemID,Item.Count,false));
        TestTrue(TEXT("Actual inventory definitions validate"),Inventory->ValidateState());
        FICInventoryState Snapshot=Inventory->GetSnapshot();
        if(Class==0)
        {
            Snapshot.Equipment[0].Grade=static_cast<int32>(EPlayerItemGrade::Rare);
            Snapshot.Equipment[0].Options.Add(TEXT("OPT_ATTACK_SPEED"),0.03f);
            TestTrue(TEXT("Authored equipment option restores"),Inventory->ApplySnapshot(Snapshot));
            const auto Stats=Inventory->GetEquipmentStats();
            TestEqual(TEXT("Percentage-only option is retained"),Stats.AttackSpeed,0.03f);
            const auto* Modifier=Stats.Modifiers.FindByPredicate([](const auto& M){ IC_TRACE_FUNCTION();return M.Stat==EICStat::AttackSpeed;});
            TestNotNull(TEXT("Typed modifier exposes percentage to effective stats"),Modifier);
            if(Modifier)TestEqual(TEXT("Option retains ratio units"),Modifier->Percent,static_cast<double>(0.03f));
        }
    }
    FICDialogueRow Dialogue;TestFalse(TEXT("No sample dialogue in gameplay data"),Service->GetDialogue(TEXT("DLG_TEST_STRING_TABLE_001"),Dialogue).bSuccess);
    return true;
}
#endif

