#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Life/Survival/ICPlayerSurvival.h"
#include "Combat/GAS/Simulation/ICCombatSimulation.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Save/Compatibility/ICSaveDataMigration.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPlanRulesCheck,"IC.Runtime.ApprovedPlanRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPlanRulesCheck::RunTest(const FString& Parameters)
{
    FPlayerSurvivalState Survival;Survival.MaxSP=500;Survival.SP=500;
    TestEqual(TEXT("Full SP still sleeps two game hours"),Survival.CalculateSleepMinutes(1),120);
    Survival.SP=99;TestEqual(TEXT("Integer ceiling division"),Survival.CalculateSleepMinutes(3),134);
    Survival.SP=99.5f;TestEqual(TEXT("Fractional SP rounds missing SP up"),Survival.CalculateSleepMinutes(3),134);
    auto* GI=NewObject<UGameInstance>();auto* Definitions=NewObject<UICDefinitionSubsystem>(GI);
    Definitions->ConfigureCatalog(GetDefault<UICDefinitionSettings>()->Catalog.LoadSynchronous());
    FICWorldState World;FICCharacterState Member;Member.CharacterId=TEXT("NPC_R01_009");Member.Kind=EICCharacterKind::Recruitable;Member.Mode=EICCharacterMode::Companion;Member.PartySlot=0;Member.HP=1;Member.MP=0;Member.SP=0;Member.MaxSP=1;
    FICCharacterState Player;Player.CharacterId=TEXT("CHR_PLAYER");Player.Kind=EICCharacterKind::Player;Player.MainStats.CHA=10000;World.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Living companion recovers"),ICCombatSimulation::RecoverFromSleep(Member,World,Definitions));
    TestEqual(TEXT("Companion HP full"),Member.HP,Member.MaxHP);TestEqual(TEXT("Companion MP full"),Member.MP,Member.MaxMP);
    const float HP=Member.MaxHP;World.Characters.FindChecked(Player.CharacterId).MainStats.CHA=1;
    ICCombatSimulation::RecoverFromSleep(Member,World,Definitions);
    TestEqual(TEXT("Retired player CHA cannot scale companion HP"),Member.MaxHP,HP);
    Member.LifeState=EICCharacterLifeState::Downed;Member.HP=0;Member.MP=7;
    TestFalse(TEXT("Sleep cannot revive downed companions"),ICCombatSimulation::RecoverFromSleep(Member,World,Definitions));
    TestEqual(TEXT("Downed HP unchanged"),Member.HP,0.f);TestEqual(TEXT("Downed MP unchanged"),Member.MP,7.f);
    TestFalse(TEXT("Legacy CHA growth rejected"),Player.MainStats.Set(EICMainStat::CHA,2));
    auto* Time=NewObject<UICTimeSubsystem>(GI);const auto Before=Time->GetClock();Time->JumpGameMinutes(120);
    TestEqual(TEXT("Sleep advances only GameClock"),Time->GetClock().GameMinute,Before.GameMinute+120);
    TestEqual(TEXT("PlayClock stays fixed"),Time->GetClock().PlaySecond,Before.PlaySecond);
    auto* Inventory=NewObject<UICInventoryComponent>();Inventory->BeginStaging(Definitions,0);
    TestFalse(TEXT("A chest can never expand to three stages"),Inventory->SetHomeStorageStage(3));
    TestEqual(TEXT("Each chest is fixed at forty slots"),Inventory->GetSnapshot().Storage.FindChecked(TEXT("PlayerHome")).Slots.Num(),40);
    TestFalse(TEXT("Stage 4 rejected"),Inventory->SetHomeStorageStage(4));
    auto Legacy=Inventory->GetSnapshot();auto& Slots=Legacy.Storage.FindChecked(TEXT("PlayerHome")).Slots;
    Slots.SetNum(160);FPlayerItemInstance Gift;Gift.ItemID=TEXT("ITM_R01_FOOD_15");Gift.Count=1;Gift.InstanceID=FGuid::NewGuid();Slots[130]=Gift;
    TestFalse(TEXT("New inventory transactions reject expanded chests"),Inventory->ApplySnapshot(Legacy));
    FICWorldState Old;Old.Inventory=Legacy;ICSaveDataMigration::NormalizeInventoryIDs(Old);
    if(!TestTrue(TEXT("Old expanded saves compact into forty slots"),Inventory->ApplySnapshot(Old.Inventory)))return false;
    if(!TestTrue(TEXT("Compacted old item can be withdrawn"),Inventory->Transfer(TEXT("PlayerHome"),0,false)))return false;
    Legacy=Inventory->GetSnapshot();
    TestEqual(TEXT("Withdrawal preserves forty slots"),Legacy.Storage.FindChecked(TEXT("PlayerHome")).Slots.Num(),40);
    for(int32 Index=0;Index<40;++Index){auto Full=Gift;Full.Count=99;Full.InstanceID=FGuid::NewGuid();Legacy.Storage.FindChecked(TEXT("PlayerHome")).Slots[Index]=Full;}
    TestTrue(TEXT("Legacy full storage snapshot accepted"),Inventory->ApplySnapshot(Legacy));
    TestFalse(TEXT("New deposit cannot use legacy overflow slots"),Inventory->Transfer(TEXT("PlayerHome"),0,true));
    TestEqual(TEXT("Failed legacy deposit preserves bag item"),Inventory->CountItem(Gift.ItemID),1);
    Old.Inventory=Legacy;Old.Inventory.Storage[TEXT("PlayerHome")].Slots.SetNum(160);Old.Inventory.Storage[TEXT("PlayerHome")].Slots[130]=Gift;Old.Inventory.Storage[TEXT("PlayerHome")].Slots[130].InstanceID=FGuid::NewGuid();
    ICSaveDataMigration::NormalizeInventoryIDs(Old);TestEqual(TEXT("Full legacy container stays forty slots"),Old.Inventory.Storage[TEXT("PlayerHome")].Slots.Num(),40);
    TestTrue(TEXT("Legacy overflow stays in a saved non-expiring recovery drop"),Old.Drops.Num()==1&&Old.Drops[0].bStorageRecovery&&Old.Drops[0].Contents.Num()==1);
    FICDefinitionResult Result;
    const auto* Rules=Definitions->Find<FICInventorySettingsRow>(TEXT("CFG_INVENTORY_001"),Result);
    if(TestNotNull(TEXT("Actual inventory DT resolves"),Rules))TestEqual(TEXT("Actual drop lifetime is ten PlayClock minutes"),Rules->DropLifetimePlaySeconds,600.);
    const auto* Recipe=Definitions->Find<FICRecipeRow>(TEXT("RCP_R01_COOK_19"),Result);
    if(TestNotNull(TEXT("Chicken recipe resolves"),Recipe))TestTrue(TEXT("Roast chicken needs two real chicken meat"),Recipe->Ingredients.Num()==1&&Recipe->Ingredients[0].ItemID==TEXT("ITM_R01_CHICKEN_MEAT")&&Recipe->Ingredients[0].Count==2);
    return !HasAnyErrors();
}
#endif
