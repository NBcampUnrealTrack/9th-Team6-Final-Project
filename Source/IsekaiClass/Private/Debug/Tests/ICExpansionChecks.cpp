#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Debug/ICDebugCheatManager.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Production/ICProductionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Codex/ICCodexSubsystem.h"
#include "Combat/VFX/ICCombatVFX.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICExpandedGameplayCheck,"IC.Expansion.PartyWeaponsRecipesAndItemUse",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICExpandedGameplayCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("확장 기능 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Test state restores"),State->RestoreState(Initial));
    auto* PC=World->SpawnActor<AICPlayerController>();auto* Pawn=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Pawn);PC->bCreated=true;
    auto* Inventory=Pawn->Inventory.Get();Inventory->BindWorldState(State);auto* Cheats=NewObject<UICDebugCheatManager>(PC);PC->CheatManager=Cheats;
    Cheats->ICRecruitTestParty();
    for(int32 Index=0;Index<3;++Index)
    {
        const FName ID(*FString::Printf(TEXT("NPC_R01_%03d"),Index+1));const auto NPC=State->GetSnapshot(ID);
        TestEqual(TEXT("Party cheat sets exact affinity 300"),NPC.Affinity,300);TestEqual(TEXT("Party cheat assigns the three slots"),NPC.PartySlot,Index);
        TestEqual(TEXT("Party cheat uses companion mode"),NPC.Mode,EICCharacterMode::Companion);
        TestEqual(TEXT("NPC codex shows current affinity"),GI->GetSubsystem<UICCodexSubsystem>()->NPCAffinity(ID),300);
    }
    Cheats->ICEquipStarterWeapons();const auto Loadout=Inventory->GetWeaponLoadout();
    for(const auto& Weapon:Loadout)TestFalse(TEXT("All four starter weapons are equipped"),Weapon.IsEmpty());
    Inventory->AddItem(TEXT("EQP_WPN_SWORD_002"),1,false);
    auto FindBag=[&](FName ID){return Inventory->GetSnapshot().Bag.Slots.IndexOfByPredicate([&](const auto& Item){return Item.ItemID==ID;});};
    TestFalse(TEXT("Low STR cannot equip village sword"),Inventory->Equip(FindBag(TEXT("EQP_WPN_SWORD_002"))));
    TestTrue(TEXT("Permanent STR can grow"),State->SetMainStat(Player.CharacterId,EICMainStat::STR,10));
    TestTrue(TEXT("Requirement boundary allows village sword"),Inventory->Equip(FindBag(TEXT("EQP_WPN_SWORD_002"))));
    TestEqual(TEXT("The new weapon occupies the sword slot"),Inventory->GetWeaponLoadout()[0].ItemID,FName(TEXT("EQP_WPN_SWORD_002")));

    Inventory->AddItem(TEXT("ITM_BOOK_001"),1,false);const FName Companion=TEXT("NPC_R01_002");const int32 BeforeSTR=State->GetSnapshot(Companion).MainStats.STR;FText Reason;
    TestTrue(TEXT("Growth book can be used on the chosen party member"),PC->UseInventoryItem(TEXT("ITM_BOOK_001"),Companion,Reason));
    TestEqual(TEXT("Only the chosen member grows"),State->GetSnapshot(Companion).MainStats.STR,BeforeSTR+1);TestEqual(TEXT("Book consumes once"),Inventory->CountItem(TEXT("ITM_BOOK_001")),0);
    Inventory->AddItem(TEXT("ITM_CONS_001"),2,false);
    TestFalse(TEXT("Full HP target rejects ineffective food/potion"),PC->UseInventoryItem(TEXT("ITM_CONS_001"),Companion,Reason));
    TestEqual(TEXT("Failed use does not consume"),Inventory->CountItem(TEXT("ITM_CONS_001")),2);
    const int32 PotionSlot=FindBag(TEXT("ITM_CONS_001"));Inventory->BindQuickSlot(0,PotionSlot);Inventory->BindQuickSlot(2,PotionSlot);
    TestEqual(TEXT("The same item may remain in another quick slot"),Inventory->GetSnapshot().QuickSlots[0],FName(TEXT("ITM_CONS_001")));
    TestEqual(TEXT("New quick binding remains"),Inventory->GetSnapshot().QuickSlots[2],FName(TEXT("ITM_CONS_001")));
    TestTrue(TEXT("Quick binding can be cleared"),Inventory->ClearQuickSlot(2));

    FICDefinitionResult Lookup;const auto* Recipe=D->Find<FICRecipeRow>(TEXT("RCP_WPN_SWORD_004"),Lookup);
    if(TestNotNull(TEXT("Fish-scale recipe exists"),Recipe))
    {
        const auto* Production=D->Find<FICProductionRow>(Recipe->ProductionID,Lookup);const FName Anchor=Production&&!Production->WorkAnchorIDs.IsEmpty()?Production->WorkAnchorIDs[0]:FName(TEXT("ANC_MERCHANT_SHOP"));
        Inventory->AddItem(TEXT("ITM_R01_FISH_01"),4,false);Inventory->AddItem(TEXT("ITM_R01_FISH_02"),5,false);Inventory->AddItem(TEXT("ITM_R01_ORE_04"),4,false);
        FGuid Job;FString Failure;auto* Service=GI->GetSubsystem<UICProductionSubsystem>();
        TestFalse(TEXT("Nine mixed fish cannot satisfy ten"),Service->StartRecipe(Recipe->ID,Anchor,Inventory,Job,Failure));
        Inventory->AddItem(TEXT("ITM_R01_FISH_02"),1,false);
        TestTrue(TEXT("Four fish1 and six fish2 satisfy one group"),Service->StartRecipe(Recipe->ID,Anchor,Inventory,Job,Failure));
        if(Job.IsValid())
        {
            const auto Pending=State->ExportState().ProductionJobs.FindChecked(Job);int32 FishReserved=0;
            for(const auto& Material:Pending.Materials)if(Material.ItemID.ToString().StartsWith(TEXT("ITM_R01_FISH_")))FishReserved+=Material.Count;
            TestEqual(TEXT("Exactly ten fish are reserved"),FishReserved,10);Time->SetRunning(true);Time->SetMenuPaused(false);Time->AdvanceClock(Recipe->WorkDurationPlaySeconds+.1,false);
            TestEqual(TEXT("Mixed-fish recipe completes automatically"),State->ExportState().ProductionJobs.FindChecked(Job).Status,EICProductionJobStatus::Completed);
            TestEqual(TEXT("Fish-scale sword is received"),Inventory->CountItem(TEXT("EQP_WPN_SWORD_004")),1);
            TestEqual(TEXT("Fish1 consumed exactly"),Inventory->CountItem(TEXT("ITM_R01_FISH_01")),0);TestEqual(TEXT("Fish2 consumed exactly"),Inventory->CountItem(TEXT("ITM_R01_FISH_02")),0);
        }
    }
    GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCSkillDataCheck,"IC.Expansion.NPCSkillDataAndWeaponProgression",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICNPCSkillDataCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    for(int32 Index=1;Index<=11;++Index)
    {
        const FName ID(*FString::Printf(TEXT("SKL_NPC_R01_%03d"),Index));const auto* Skill=D->Find<FICSkillRow>(ID,R);
        if(!TestNotNull(TEXT("Every NPC retains its unique skill"),Skill))continue;
        const auto* Visuals=ICCombatVFX::Resolve(Skill->Presentation);TestNotNull(TEXT("Unique skill has a DT visual profile"),Visuals);
        if(Visuals){TestFalse(TEXT("Hit effect is configured"),Visuals->Impact.System.IsNull());TestTrue(TEXT("Impact follows its recipient"),Visuals->bImpactFollowsTarget);}
        for(const auto& Hit:Skill->Hits)TestNotNull(TEXT("Every hit has a valid effect definition"),D->Find<FICSkillEffectRow>(Hit.EffectID,R));
    }
    for(const FString Family:{TEXT("SWORD"),TEXT("BOW"),TEXT("STAFF"),TEXT("WAND")})for(int32 Tier=1;Tier<=5;++Tier)
    {
        const auto* Gear=D->Find<FICEquipmentRow>(FName(*FString::Printf(TEXT("EQU_WPN_%s_%03d"),*Family,Tier)),R);
        if(!TestNotNull(TEXT("Five tiers exist for each weapon family"),Gear))continue;
        TestEqual(TEXT("Stat requirements rise by ten"),Gear->RequiredMainStatValue,(Tier-1)*10);
        double Damage=0;for(const auto& Mod:Gear->BaseModifiers)if(Mod.Stat==EICStat::PhysicalAttack||Mod.Stat==EICStat::MagicAttack)Damage+=Mod.Flat;
        TestEqual(TEXT("Weapon attack rises by ten"),Damage,double(Tier*10));
    }
    auto* World=GI->GetWorld();GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
