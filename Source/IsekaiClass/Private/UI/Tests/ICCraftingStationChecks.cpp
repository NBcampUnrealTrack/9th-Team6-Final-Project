#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Life/ICCraftingStationWidget.h"
#include "Life/Crafting/ICCraftingStation.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCraftingStationCheck,"IC.Life.CraftingStationFlow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICCraftingStationCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    auto* PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Player=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Player);
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,0);
    for(const auto* ID:{TEXT("ITM_R01_ORE_06"),TEXT("ITM_R01_PLANT_10"),TEXT("ITM_R01_CROP_04"),TEXT("ITM_R01_FISH_04"),TEXT("ITM_R01_CROP_01")})Staging->AddItem(ID,10,false);
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("제작 UI 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");Initial.Inventory=Staging->GetSnapshot();
    FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;Initial.Characters.Add(P.CharacterId,P);
    TestTrue(TEXT("Crafting test state restores"),State->RestoreState(Initial));Player->InitializeState(P.CharacterId);Player->Inventory->BindWorldState(State);PC->bCreated=true;PC->Menu.Reset();Time->SetRunning(true);Time->SetMenuPaused(false);
    for(const auto* Kind:{TEXT("Crafting"),TEXT("Cooking")})
    {
        const bool Cook=FString(Kind)==TEXT("Cooking");
        auto* Class=LoadClass<AICCraftingStation>(nullptr,Cook?TEXT("/Game/IsekaiClass/Life/Cooking/BP_IC_Kitchen.BP_IC_Kitchen_C"):TEXT("/Game/IsekaiClass/Life/Crafting/BP_IC_CraftingTable.BP_IC_CraftingTable_C"));
        auto* Station=Class?World->SpawnActor<AICCraftingStation>(Class):nullptr;if(!TestNotNull(TEXT("Station BP loads"),Station))continue;
        auto* UI=CreateWidget<UICCraftingStationWidget>(PC,Station->StationWidgetClass);if(!TestNotNull(TEXT("Editable station WBP loads"),UI))continue;
        UI->OpenStation(Station);TestTrue(TEXT("Station UI opens"),UI->IsOpen());TestTrue(TEXT("Production clock runs while station UI is open"),Time->IsRunning());
        const auto First=UI->SelectedRecipe;UI->HandleStationKey(EKeys::S);TestTrue(TEXT("S selects a different recipe"),First!=UI->SelectedRecipe);UI->HandleStationKey(EKeys::W);TestEqual(TEXT("W returns to first recipe"),UI->SelectedRecipe,First);
        UI->HandleStationKey(EKeys::Enter);TestTrue(TEXT("Enter opens recipe details"),UI->bDetails);UI->HandleStationKey(EKeys::Escape);TestFalse(TEXT("Detail cancel returns to list"),UI->bDetails);TestTrue(TEXT("Detail cancel keeps UI open"),UI->IsOpen());
        const FName RecipeID=Cook?TEXT("RCP_COOK_001"):TEXT("RCP_CRAFT_WAND_001");FICDefinitionResult Result;const auto* Recipe=D->Find<FICRecipeRow>(RecipeID,Result);
        if(TestNotNull(TEXT("Requested actual DT recipe"),Recipe))
        {
            const auto Output=Recipe->Results[0];const int32 Before=PC->GetInventory()->CountItem(Output.ItemID);
            UI->SelectRecipe(RecipeID);UI->MakeRecipe();const FGuid Job=UI->ActiveJob;TestTrue(TEXT("Make starts real production"),Job.IsValid());
            Time->AdvanceClock(2.5,false);UI->NativeTick(FGeometry(),.01f);
            auto* Bar=Cast<UProgressBar>(UI->GetWidgetFromName(TEXT("BAR_Work")));TestTrue(TEXT("Five second recipe fills gauge halfway"),Bar&&FMath::IsNearlyEqual(Bar->GetPercent(),.5f,.02f));
            UI->CloseStation();TestTrue(TEXT("Background production continues after closing"),Time->IsRunning());UI->OpenStation(Station);TestEqual(TEXT("Reopening resumes the same job"),UI->ActiveJob,Job);
            Time->AdvanceClock(2.51,false);UI->NativeTick(FGeometry(),.01f);TestFalse(TEXT("Completion returns to gameplay"),UI->IsOpen());TestEqual(TEXT("Actual item granted once"),PC->GetInventory()->CountItem(Output.ItemID),Before+Output.Count);
        }
        UI->CloseStation();Station->Destroy();
    }
    Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
