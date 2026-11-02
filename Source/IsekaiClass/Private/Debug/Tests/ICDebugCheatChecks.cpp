#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/OutputDeviceNull.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Debug/ICDebugCheatManager.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Life/World/ICLifeActor.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICDebugCheatCheck, "IC.Debug.StarterWeaponsAndGameHours", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICDebugCheatCheck::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GEngine); GI->InitializeStandalone();
    auto* World = GI->GetWorld(); World->InitializeActorsForPlay(FURL(), false);
    auto* State = GI->GetSubsystem<UICWorldStateSubsystem>(); auto* Time = GI->GetSubsystem<UICTimeSubsystem>();
    auto* D = GI->GetSubsystem<UICDefinitionSubsystem>();
    FICWorldState Initial; Initial.bPlayerCreated = true; Initial.PlayerName = TEXT("치트 검증"); Initial.FirstSkill = TEXT("SKL_NPC_R01_002");
    FICCharacterState Player; Player.CharacterId = UICWorldStateSubsystem::PlayerId(); Player.Kind = EICCharacterKind::Player; Initial.Characters.Add(Player.CharacterId, Player);
    TestTrue(TEXT("Cheat test world restores"), State->RestoreState(Initial));
    auto* PC = World->SpawnActor<AICPlayerController>(); auto* Pawn = World->SpawnActor<AICPlayerCharacter>(); PC->Possess(Pawn);
    auto* Inventory = Pawn->Inventory.Get(); Inventory->BindWorldState(State);
    auto* Cheats = NewObject<UICDebugCheatManager>(PC); PC->CheatManager = Cheats;
    // UPlayer forwards console commands to the cheat manager's Exec entry point.
    FOutputDeviceNull Output;
    TestTrue(TEXT("Configured cheat class is the project manager"), PC->CheatClass == UICDebugCheatManager::StaticClass());
    TestTrue(TEXT("Weapon console command resolves without parameters"), Cheats->ProcessConsoleExec(TEXT("ICGiveStarterWeapons"), Output, PC));
    const FName IDs[] = {TEXT("EQP_WPN_SWORD_001"), TEXT("EQP_WPN_BOW_001"), TEXT("EQP_WPN_STAFF_001"), TEXT("EQP_WPN_WAND_001")};
    for (FName ID : IDs) TestEqual(TEXT("Each starter weapon is granted once"), Inventory->CountItem(ID), 1);
    TestTrue(TEXT("Grant uses valid saved inventory instances"), UICWorldStateSubsystem::ValidateState(State->ExportState()));
    const int32 GoldBefore=State->ExportState().Gold;
    TestTrue(TEXT("Gold command resolves without parameters"),Cheats->ProcessConsoleExec(TEXT("ICGiveGold"),Output,PC));
    TestEqual(TEXT("Gold cheat adds exactly 10000"),State->ExportState().Gold,GoldBefore+10000);
    TestEqual(TEXT("Gold cheat refreshes the visible wallet"),PC->Gold,GoldBefore+10000);

    // Three empty slots must not leave a partially granted four-weapon set.
    FICInventoryState Full;
    for (int32 I = 0; I < Full.Bag.Slots.Num() - 3; ++I)
    { auto& Item = Full.Bag.Slots[I]; Item.ItemID = TEXT("ITM_R01_SEED_01"); Item.Count = 99; Item.InstanceID = FGuid::NewGuid(); }
    TestTrue(TEXT("Nearly full bag fixture applies"), Inventory->ApplySnapshot(Full));
    Cheats->ProcessConsoleExec(TEXT("ICGiveStarterWeapons"), Output, PC);
    for (FName ID : IDs) TestEqual(TEXT("Insufficient space grants no partial set"), Inventory->CountItem(ID), 0);
    TestTrue(TEXT("Insufficient space gives a useful notice"), PC->Notice.Contains(TEXT("공간")));
    TestEqual(TEXT("Existing items remain intact"), Inventory->CountItem(TEXT("ITM_R01_SEED_01")), (Full.Bag.Slots.Num() - 3) * 99);

    FICDefinitionResult R; const auto* Crop = D->Find<FICCropRow>(TEXT("CRP_R01_01"), R);
    auto* Plot = Crop ? World->SpawnActor<AICLifeActor>(Crop->CropActorClass.LoadSynchronous()) : nullptr;
    if (TestNotNull(TEXT("Actual natural crop loads for hour skip"), Plot))
    {
        Plot->PointID = TEXT("CHEAT_TEST_CROP"); if (!Plot->HasActorBegunPlay()) Plot->DispatchBeginPlay(); Plot->RefreshLife();
        TestEqual(TEXT("Crop begins at first stage"), Plot->GetGrowthStage(), 0);
    }
    const auto Before = Time->GetClock();
    auto Next = State->ExportState(); Next.ResourceReadyAtGameMinute.Add(TEXT("CHEAT_TEST_RESOURCE"), Before.GameMinute + 120);
    TestTrue(TEXT("Pending calendar regeneration commits"), State->CommitGameplayTransaction(Next, FGuid::NewGuid()));
    auto* Resource = World->SpawnActor<AICLifeActor>(); Resource->PointID = TEXT("CHEAT_TEST_RESOURCE");
    TestFalse(TEXT("Resource is initially depleted"), Resource->IsResourceAvailable());
    TestTrue(TEXT("Hour console command resolves"), Cheats->ProcessConsoleExec(TEXT("ICAddGameHours 2"), Output, PC));
    TestEqual(TEXT("Two hours advances 120 game minutes"), Time->GetClock().GameMinute, Before.GameMinute + 120);
    TestTrue(TEXT("Hour skip regenerates depleted resource"), Resource->IsResourceAvailable());
    Cheats->ProcessConsoleExec(TEXT("ICAddGameHours 0.5"), Output, PC);
    TestEqual(TEXT("Fractional hours work"), Time->GetClock().GameMinute, Before.GameMinute + 150);
    Cheats->ProcessConsoleExec(TEXT("ICAddGameHours -1"), Output, PC);
    Cheats->ICAddGameHours(DBL_MAX);
    TestEqual(TEXT("Negative and overflowing hours do not change time"), Time->GetClock().GameMinute, Before.GameMinute + 150);
    Cheats->ProcessConsoleExec(TEXT("ICAddGameHours 24"), Output, PC);
    TestEqual(TEXT("A day advances 1440 more minutes"), Time->GetClock().GameMinute, Before.GameMinute + 1590);
    if (Plot) TestEqual(TEXT("Hour command advances actual crop growth"), Plot->GetGrowthStage(), 1);
    TestEqual(TEXT("Hour skip does not advance combat/production play seconds"), Time->GetClock().PlaySecond, Before.PlaySecond);
    Cheats->ProcessConsoleExec(TEXT("ICAddGameMinutes 30"), Output, PC);
    TestEqual(TEXT("Original minute command still works"), Time->GetClock().GameMinute, Before.GameMinute + 1620);
    GI->Shutdown(); World->DestroyWorld(false); return !HasAnyErrors();
}
#endif
