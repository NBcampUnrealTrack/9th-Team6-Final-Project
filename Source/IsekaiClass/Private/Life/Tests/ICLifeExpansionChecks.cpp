#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Inventory/Rewards/ICRewardService.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "Life/Production/ICProductionSubsystem.h"
#include "Life/World/ICPlayerHomeActors.h"
#include "NPC/LifeNPC/ICShopSubsystem.h"
#include "Data/Rows/NPC/ICServiceRow.h"
#include "Data/Rows/NPC/ICShopRow.h"
#include "Codex/ICCodexSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

namespace ICLifeExpansionChecks
{
struct FSession
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);
    UWorld* World=nullptr;UICWorldStateSubsystem* State=nullptr;UICDefinitionSubsystem* Definitions=nullptr;UICTimeSubsystem* Time=nullptr;
    AICPlayerController* PC=nullptr;AICPlayerCharacter* Player=nullptr;UICInventoryComponent* Bag=nullptr;
    FSession()
    {
        GI->InitializeStandalone();World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
        State=GI->GetSubsystem<UICWorldStateSubsystem>();Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();Time=GI->GetSubsystem<UICTimeSubsystem>();
        FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("생활 확장 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");Initial.Gold=10000;
        FICCharacterState Character;Character.CharacterId=UICWorldStateSubsystem::PlayerId();Character.Kind=EICCharacterKind::Player;Initial.Characters.Add(Character.CharacterId,Character);
        State->RestoreState(Initial);State->EnsureCharacter(TEXT("NPC_R01_006"),EICCharacterKind::Recruitable);
        PC=World->SpawnActor<AICPlayerController>();Player=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Player);PC->bCreated=true;PC->Menu.Reset();
        Player->InitializeState(Character.CharacterId);Bag=Player->Inventory;Bag->BindWorldState(State);Time->SetRunning(true);Time->SetMenuPaused(false);
    }
    ~FSession(){Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLifeCartCheck,"IC.Life.Expansion.AtomicSeedCart",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICLifeCartCheck::RunTest(const FString&)
{
    ICLifeExpansionChecks::FSession F;auto* Shop=F.GI->GetSubsystem<UICShopSubsystem>();TArray<FICShopOffer> Offers;FString Failure;FText Reason;
    const FName Service=TEXT("SRV_R01_006_SHOP");FICDefinitionResult Lookup;
    const auto* ServiceDefinition=F.Definitions->Find<FICServiceRow>(Service,Lookup);
    if(!TestNotNull(TEXT("Seed service is configured in the current DT"),ServiceDefinition))return false;
    const auto* ShopDefinition=F.Definitions->Find<FICShopRow>(ServiceDefinition->ShopID,Lookup);
    if(!TestNotNull(TEXT("Seed service resolves its current shop DT"),ShopDefinition))return false;
    TestTrue(TEXT("NPC06 serves seeds independently of work schedule"),Shop->GetOffers(Service,Offers,Failure));
    TestEqual(TEXT("Seed offers match the current shop DT entry count"),Offers.Num(),ShopDefinition->Entries.Num());
    TSet<FName> ExpectedItems;
    for(const auto& Entry:ShopDefinition->Entries)
    {
        ExpectedItems.Add(Entry.ItemID);
        const auto* Item=F.Definitions->Find<FICItemRow>(Entry.ItemID,Lookup);
        TestTrue(TEXT("Every configured seed has a valid positive purchase price"),Item&&Item->BuyPrice>0);
        TestTrue(TEXT("Every configured seed is offered"),Offers.ContainsByPredicate([&](const auto& Offer){return Offer.ItemID==Entry.ItemID;}));
    }
    TestEqual(TEXT("Seed shop DT contains no duplicate items"),ExpectedItems.Num(),ShopDefinition->Entries.Num());
    for(const auto& Offer:Offers)TestTrue(TEXT("No unconfigured seed is offered"),ExpectedItems.Contains(Offer.ItemID));
    if(!TestTrue(TEXT("Atomic cart test has at least two configured seed offers"),Offers.Num()>=2))return false;
    auto Line=[](FName ID,int32 Count){FICItemAmount Value;Value.ItemID=ID;Value.Count=Count;return Value;};
    const auto A=Offers[0],B=Offers[1];const int32 Before=F.State->ExportState().Gold;
    TestTrue(TEXT("One cart purchases multiple seeds atomically"),Shop->Checkout(F.PC,Service,{Line(A.ItemID,2),Line(B.ItemID,3)},false,Reason));
    TestEqual(TEXT("First seed count"),F.Bag->CountItem(A.ItemID),2);TestEqual(TEXT("Second seed count"),F.Bag->CountItem(B.ItemID),3);TestEqual(TEXT("Exact cart total deducted"),F.State->ExportState().Gold,Before-A.FinalPrice*2-B.FinalPrice*3);
    const int32 Paid=F.State->ExportState().Gold;
    TestFalse(TEXT("Unavailable cart item rejects entire purchase"),Shop->Checkout(F.PC,Service,{Line(A.ItemID,1),Line(TEXT("ITM_QST_001"),1)},false,Reason));
    TestEqual(TEXT("Rejected cart never charges gold"),F.State->ExportState().Gold,Paid);TestEqual(TEXT("Rejected cart never adds its valid line"),F.Bag->CountItem(A.ItemID),2);
    TestFalse(TEXT("Oversell rejects the entire cart"),Shop->Checkout(F.PC,Service,{Line(A.ItemID,1),Line(B.ItemID,4)},true,Reason));TestEqual(TEXT("Failed sell leaves first line intact"),F.Bag->CountItem(A.ItemID),2);
    TestTrue(TEXT("Several items sell in one transaction"),Shop->Checkout(F.PC,Service,{Line(A.ItemID,1),Line(B.ItemID,2)},true,Reason));TestEqual(TEXT("Sale total credited exactly"),F.State->ExportState().Gold,Paid+A.SellPrice+B.SellPrice*2);
    F.Bag->AddItem(TEXT("ITM_R01_ORE_01"),1,false);F.Bag->AddItem(TEXT("ITM_R01_ORE_02"),1,false);F.State->SetMainStat(UICWorldStateSubsystem::PlayerId(),EICMainStat::CON,10);
    FGuid Job;auto* Production=F.GI->GetSubsystem<UICProductionSubsystem>();FICDefinitionResult R;const auto* Recipe=F.Definitions->Find<FICRecipeRow>(TEXT("RCP_PLAYER_STORAGE_01"),R);const auto* Work=Recipe?F.Definitions->Find<FICProductionRow>(Recipe->ProductionID,R):nullptr;
    if(!TestNotNull(TEXT("Storage recipe production exists"),Work))return false;
    TestTrue(TEXT("Starting chest craft reserves ore"),Production->StartRecipe(Recipe->ID,Work->WorkAnchorIDs[0],F.Bag,Job,Failure));
    TestEqual(TEXT("Reserved ore is excluded from shop sell count"),Shop->GetSellableCount(F.PC,TEXT("ITM_R01_ORE_01")),0);
    TestFalse(TEXT("Shop cannot sell reserved production ingredients"),Shop->Checkout(F.PC,Service,{Line(TEXT("ITM_R01_ORE_01"),1)},true,Reason));Production->CancelJob(Job);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLifeStorageCheck,"IC.Life.Expansion.SequentialStorage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICLifeStorageCheck::RunTest(const FString&)
{
    ICLifeExpansionChecks::FSession F;auto* Craft=F.Player->FindComponentByClass<UICCraftingComponent>();auto* Production=F.GI->GetSubsystem<UICProductionSubsystem>();FICDefinitionResult R;
    F.Bag->AddItem(TEXT("ITM_R01_ORE_01"),2,false);F.Bag->AddItem(TEXT("ITM_R01_ORE_02"),2,false);
    auto* Chest=F.World->SpawnActor<AICPlayerStorage>();Chest->RequiredExtraStorage=1;Chest->RefreshAvailability();TestTrue(TEXT("Additional chest starts hidden"),Chest->IsHidden());
    for(int32 Step=1;Step<=2;++Step)
    {
        const FName ID(*FString::Printf(TEXT("RCP_PLAYER_STORAGE_%02d"),Step));const auto* Recipe=F.Definitions->Find<FICRecipeRow>(ID,R);if(!TestNotNull(TEXT("Sequential chest recipe exists"),Recipe))return false;
        const auto* Work=F.Definitions->Find<FICProductionRow>(Recipe->ProductionID,R);if(!TestTrue(TEXT("Work anchor configured"),Work&&!Work->WorkAnchorIDs.IsEmpty()))return false;
        TestTrue(TEXT("Current chest recipe appears"),Craft->GetRecipes(EPlayerRecipeType::Craft).Contains(ID));
        F.State->SetMainStat(UICWorldStateSubsystem::PlayerId(),EICMainStat::CON,Step*10-1);FGuid Job;FString Failure;
        TestFalse(TEXT("CON below DT requirement cannot start"),Production->StartRecipe(ID,Work->WorkAnchorIDs[0],F.Bag,Job,Failure));
        F.State->SetMainStat(UICWorldStateSubsystem::PlayerId(),EICMainStat::CON,Step*10);TestTrue(TEXT("CON at DT requirement starts craft"),Production->StartRecipe(ID,Work->WorkAnchorIDs[0],F.Bag,Job,Failure));
        if(!Job.IsValid())return false;
        F.Time->AdvanceClock(4.9,false);TestEqual(TEXT("Chest remains pending before five seconds"),F.State->ExportState().ProgressionStages.FindRef(TEXT("PLAYER_STORAGE")),Step-1);
        F.Time->AdvanceClock(.11,false);const auto Data=F.State->ExportState();TestEqual(TEXT("Completed chest increments once"),Data.ProgressionStages.FindRef(TEXT("PLAYER_STORAGE")),Step);
        TestTrue(TEXT("Each chest has its independent forty slots"),Data.Inventory.Storage.Contains(Recipe->InstalledContainerID)&&Data.Inventory.Storage.FindChecked(Recipe->InstalledContainerID).Slots.Num()==40);
        TestEqual(TEXT("Exactly one ore per craft is consumed"),F.Bag->CountItem(TEXT("ITM_R01_ORE_01")),2-Step);
        TestFalse(TEXT("Completed chest recipe disappears"),Craft->GetRecipes(EPlayerRecipeType::Craft).Contains(ID));
        TestFalse(TEXT("Completing the same job twice cannot add a chest"),Production->CompleteJob(Job,F.Bag,Failure));
    }
    Chest->RefreshAvailability();TestFalse(TEXT("Crafted chest becomes visible"),Chest->IsHidden());
    const auto Final=F.State->ExportState();TestEqual(TEXT("One base plus two crafted inventories"),Final.Inventory.Storage.Num(),3);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLifePlantFishPetCheck,"IC.Life.Expansion.PlotsFishingAndEssence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICLifePlantFishPetCheck::RunTest(const FString&)
{
    ICLifeExpansionChecks::FSession F;FICDefinitionResult R;FText Failure;auto* Codex=F.GI->GetSubsystem<UICCodexSubsystem>();
    F.Bag->AddItem(TEXT("ITM_R01_SEED_01"),8,false);TArray<AICFarmPlot*> Plots;
    for(int32 Index=1;Index<=8;++Index)
    {
        const FName Point(*FString::Printf(TEXT("ANC_PLAYER_PLOT_%02d"),Index));TestNotNull(TEXT("Each of eight planting anchors has DT data"),F.Definitions->Find<FICAnchorRow>(Point,R));
        auto* Plot=F.World->SpawnActor<AICFarmPlot>();Plot->PointID=Point;Plot->DefinitionID=TEXT("CRP_R01_01");TestTrue(TEXT("Every tile can plant exactly one seed"),Plot->Plant(F.PC));TestFalse(TEXT("An occupied tile rejects another seed"),Plot->Plant(F.PC));Plots.Add(Plot);
    }
    TestEqual(TEXT("Eight tiles consume eight seeds"),F.Bag->CountItem(TEXT("ITM_R01_SEED_01")),0);TestEqual(TEXT("Eight distinct crop save records"),F.State->ExportState().Crops.Num(),8);
    F.Time->JumpGameMinutes(1440);for(auto* Plot:Plots){Plot->RefreshLife();TestEqual(TEXT("One day advances one growth stage"),Plot->GetGrowthStage(),1);}
    F.Time->JumpGameMinutes(1440*5);TestTrue(TEXT("A mature planted tile harvests"),Plots[0]->CompletePlayerAction(F.PC,NAME_None,Failure));TestEqual(TEXT("Harvest frees the tile"),F.State->ExportState().Crops.Num(),7);
    const auto* Point=F.Definitions->Find<FICFishingPointRow>(TEXT("FPT_R01_RANDOM_001"),R);if(!TestNotNull(TEXT("Real random fishing point"),Point))return false;
    const auto* Work=F.Definitions->Find<FICProductionRow>(Point->ProductionID,R);TestTrue(TEXT("Fishing takes twenty play seconds and has no repeat restriction"),Work&&Work->DurationPlaySeconds==20&&Work->bAllowRepeat&&!Work->WorkToolClass.IsNull());
    auto* Spot=F.World->SpawnActor<AICLifeActor>(Point->PointActorClass.LoadSynchronous());Spot->PointID=TEXT("TEST_R01_FISH_RANDOM");
    TestTrue(TEXT("Fishing completion grants a fish"),Spot->CompletePlayerAction(F.PC,NAME_None,Failure));TestTrue(TEXT("A second catch is immediately possible"),Spot->CompletePlayerAction(F.PC,NAME_None,Failure));
    int32 FishCount=0;for(int32 I=1;I<=10;++I)FishCount+=F.Bag->CountItem(FName(*FString::Printf(TEXT("ITM_R01_FISH_%02d"),I)));TestEqual(TEXT("Two catches grant two fish"),FishCount,2);
    TArray<FName> Monsters;F.Definitions->GetDefinitionIDs(EICDefinitionType::Monster,Monsters);TestEqual(TEXT("Pet codex has one cell per actual monster"),Codex->GetPetIDs().Num(),Monsters.Num());
    const auto* Monster=F.Definitions->Find<FICMonsterRow>(TEXT("MON_COMBAT_TEST_001"),R);if(!TestNotNull(TEXT("Test creature participates in pet essence"),Monster))return false;
    auto Candidate=F.State->ExportState();const FName Key=TEXT("TEST_ESSENCE:0");auto& Slot=Candidate.SpawnSlots.Add(Key);Slot.MonsterID=Monster->ID;Slot.SpawnerID=TEXT("TEST_ESSENCE");Slot.IncarnationID=FGuid::NewGuid();Slot.bDefeated=true;Slot.bRewardPending=true;
    const FGuid Receipt=Slot.IncarnationID;FString Reason;TestTrue(TEXT("Real defeat reward grants essence even without an item drop table"),FICRewardService::PrepareMonsterDefeat(Candidate,Key,*Monster,F.Definitions,0,Reason));
    const int32 Count=Candidate.Codex.PetEssence.FindRef(Monster->PetCodexIndex);TestTrue(TEXT("Each kill grants one to ten essence"),Count>=1&&Count<=10);
    TestTrue(TEXT("Retrying a death receipt succeeds without duplicate grants"),FICRewardService::PrepareMonsterDefeat(Candidate,Key,*Monster,F.Definitions,0,Reason));TestEqual(TEXT("Essence receipt is idempotent"),Candidate.Codex.PetEssence.FindRef(Monster->PetCodexIndex),Count);
    Candidate.SpawnSlots.Remove(Key);TestTrue(TEXT("Essence result commits"),F.State->CommitGameplayTransaction(Candidate,Receipt));
    TestTrue(TEXT("Essence reaches exact unlock threshold"),Codex->AddPetEssence(Monster->PetCodexIndex,100-Count));TestTrue(TEXT("One hundred essence permits pet use"),Codex->EquipPet(Monster->PetCodexIndex,UICWorldStateSubsystem::PlayerId()));TestFalse(TEXT("Equipped pet exposes usage label"),Codex->PetUsageText(Monster->PetCodexIndex).IsEmpty());
    return !HasAnyErrors();
}
#endif
