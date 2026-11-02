#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Widgets/SOverlay.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Production/ICGatheringSubsystem.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Life/World/ICPlayerHomeActors.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

namespace ICPlantingChecks
{
struct FSession
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);
    UWorld* World=nullptr;
    UICWorldStateSubsystem* State=nullptr;
    UICTimeSubsystem* Time=nullptr;
    UICDefinitionSubsystem* Definitions=nullptr;
    UICGatheringSubsystem* Gather=nullptr;
    AICPlayerController* PC=nullptr;
    AICPlayerCharacter* Player=nullptr;
    UICInventoryComponent* Bag=nullptr;
    FSession()
    {
        GI->InitializeStandalone();World=GI->GetWorld();
        auto& Context=*GEngine->GetWorldContextFromWorld(World);
        auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;
        Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
        State=GI->GetSubsystem<UICWorldStateSubsystem>();Time=GI->GetSubsystem<UICTimeSubsystem>();
        Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();Gather=GI->GetSubsystem<UICGatheringSubsystem>();
        PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
        Player=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Player);PC->bCreated=true;PC->Menu.Reset();
        FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("심기 검증");Initial.FirstSkill=TEXT("SKL_NPC_R01_002");
        FICCharacterState Character;Character.CharacterId=UICWorldStateSubsystem::PlayerId();Character.Kind=EICCharacterKind::Player;
        Initial.Characters.Add(Character.CharacterId,Character);State->RestoreState(Initial);
        Player->InitializeState(Character.CharacterId);Bag=Player->Inventory;Bag->BindWorldState(State);
        Time->SetRunning(true);Time->SetMenuPaused(false);
    }
    ~FSession()
    {
        Gather->CancelGather();Player->ReleaseForLoad();GI->Shutdown();
        GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
    AICFarmPlot* Plot()
    {
        auto* Type=LoadClass<AICFarmPlot>(nullptr,TEXT("/Game/IsekaiClass/Life/Farming/Blueprints/BP_IC_FarmPlot.BP_IC_FarmPlot_C"));
        return Type?World->SpawnActor<AICFarmPlot>(Type,Player->GetActorLocation()+FVector(180,0,0),FRotator::ZeroRotator):nullptr;
    }
    void Step(double Seconds){Time->AdvanceClock(Seconds,false);Gather->Tick(Seconds);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPlantingProgressCheck,"IC.Life.Planting.ProgressAndSeedConsumption",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPlantingProgressCheck::RunTest(const FString&)
{
    ICPlantingChecks::FSession F;
    for(int32 Index=1;Index<=5;++Index)
    {
        const FName CropID(*FString::Printf(TEXT("CRP_R01_%02d"),Index));FICDefinitionResult Lookup;
        const auto* Crop=F.Definitions->Find<FICCropRow>(CropID,Lookup);
        if(!TestNotNull(TEXT("Default crop DT exists"),Crop))return false;
        const auto* Work=F.Definitions->Find<FICProductionRow>(Crop->PlantingProductionID,Lookup);
        if(!TestNotNull(TEXT("Planting duration is defined"),Work))return false;
        auto* Plot=F.Plot();if(!TestNotNull(TEXT("Actual farm blueprint loads"),Plot))return false;
        TestTrue(TEXT("Newly placed plots have no hardcoded anchor"),Plot->PointID.IsNone());
        TestTrue(TEXT("Seed fixture commits"),F.Bag->AddItem(Crop->SeedItemID,2,false));
        TestTrue(TEXT("Selecting this seed starts timed planting"),F.Gather->StartPlant(Plot,F.PC,CropID));
        TestTrue(TEXT("Planting work presentation starts"),F.Player->LifeActions->IsWorking());
        F.Step(Work->DurationPlaySeconds/2);Plot->RefreshLife();F.Gather->Tick(0);
        TestTrue(TEXT("Refreshing an empty plot does not cancel pending planting"),F.Gather->IsGathering());
        TestEqual(TEXT("Seed remains until completion"),F.Bag->CountItem(Crop->SeedItemID),2);
        TestFalse(TEXT("No crop record before completion"),F.State->ExportState().Crops.Contains(Plot->GetFName()));
        F.Step(Work->DurationPlaySeconds/2+.01);
        TestFalse(TEXT("Planting finishes after its DT duration"),F.Gather->IsGathering());
        TestFalse(TEXT("Work presentation ends"),F.Player->LifeActions->IsWorking());
        TestEqual(TEXT("Completion consumes exactly one seed"),F.Bag->CountItem(Crop->SeedItemID),1);
        TestEqual(TEXT("Crop immediately displays growth stage one"),Plot->GetGrowthStage(),0);
        TestNotNull(TEXT("Planted crop has a visible mesh"),Plot->VisualMesh->GetStaticMesh().Get());
        const auto Snapshot=F.State->ExportState();const auto* Saved=Snapshot.Crops.Find(Plot->GetFName());
        if(TestNotNull(TEXT("Crop persists using the placed actor's unique name"),Saved))TestEqual(TEXT("Correct selected crop persists"),Saved->CropID,CropID);
        F.Step(Work->DurationPlaySeconds);
        TestEqual(TEXT("Extra ticks cannot consume a second seed"),F.Bag->CountItem(Crop->SeedItemID),1);
        TestFalse(TEXT("An occupied plot cannot replay planting"),F.Gather->StartPlant(Plot,F.PC,CropID));
        TestFalse(TEXT("Rejected planting does not play work presentation"),F.Player->LifeActions->IsWorking());
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPlacedFarmPlantingCheck,"IC.Life.Planting.PlacedOpenWorldPlots",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPlacedFarmPlantingCheck::RunTest(const FString&)
{
    auto* Map=LoadObject<UWorld>(nullptr,TEXT("/Game/IsekaiClass/World/Maps/L_OpenWorld.L_OpenWorld"));
    if(!TestTrue(TEXT("Current saved open world loads"),Map&&Map->PersistentLevel))return false;
    ICPlantingChecks::FSession F;FICDefinitionResult Lookup;
    const FName CropID=TEXT("CRP_R01_01");const auto* Crop=F.Definitions->Find<FICCropRow>(CropID,Lookup);
    const auto* Work=Crop?F.Definitions->Find<FICProductionRow>(Crop->PlantingProductionID,Lookup):nullptr;
    if(!TestNotNull(TEXT("Current planting production loads"),Work))return false;
    int32 PlotCount=0;
    for(const auto& Actor:Map->PersistentLevel->Actors)if(auto* Placed=Cast<AICFarmPlot>(Actor.Get()))
    {
        FActorSpawnParameters Params;Params.Name=Placed->GetFName();
        auto* Plot=F.World->SpawnActor<AICFarmPlot>(Placed->GetClass(),F.Player->GetActorLocation()+FVector(180,0,0),FRotator::ZeroRotator,Params);
        if(!TestNotNull(TEXT("Placed farm plot class spawns"),Plot))return false;
        Plot->PointID=Placed->PointID;F.Bag->AddItem(Crop->SeedItemID,1,false);
        TestTrue(*FString::Printf(TEXT("Current placed plot %s accepts a seed"),*Placed->GetName()),F.Gather->StartPlant(Plot,F.PC,CropID));
        F.Step(Work->DurationPlaySeconds+.01);
        TestEqual(TEXT("Placed plot consumes its seed"),F.Bag->CountItem(Crop->SeedItemID),0);
        TestEqual(TEXT("Placed plot displays its crop"),Plot->GetGrowthStage(),0);++PlotCount;
    }
    TestTrue(TEXT("Current open world contains farm plots"),PlotCount>0);
    TestEqual(TEXT("Each placed plot saves independently"),F.State->ExportState().Crops.Num(),PlotCount);
    AddInfo(FString::Printf(TEXT("Verified all %d currently placed open-world plots"),PlotCount));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPlantingCancellationCheck,"IC.Life.Planting.CancelAndRevalidate",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPlantingCancellationCheck::RunTest(const FString&)
{
    ICPlantingChecks::FSession F;auto* Plot=F.Plot();if(!TestNotNull(TEXT("Farm blueprint loads"),Plot))return false;
    const FName CropID=TEXT("CRP_R01_01"),Seed=TEXT("ITM_R01_SEED_01");FText Failure;
    TestFalse(TEXT("Missing seed rejects planting before the animation"),F.Gather->StartPlant(Plot,F.PC,CropID));
    TestFalse(TEXT("Missing seed produces no work animation"),F.Player->LifeActions->IsWorking());
    TestFalse(TEXT("Missing seed reports a reason"),F.PC->Notice.IsEmpty());
    F.Bag->AddItem(Seed,1,false);
    TestTrue(TEXT("Planting starts before cancellation"),F.Gather->StartPlant(Plot,F.PC,CropID));F.Step(1);
    F.Gather->CancelGather();F.Step(5);
    TestEqual(TEXT("Cancellation keeps the seed"),F.Bag->CountItem(Seed),1);
    TestTrue(TEXT("Cancellation creates no saved crop"),F.State->ExportState().Crops.IsEmpty());
    TestTrue(TEXT("Retry can start immediately"),F.Gather->StartPlant(Plot,F.PC,CropID));
    F.Bag->Consume(Seed,1);F.Step(5);
    TestTrue(TEXT("Losing the seed during work cannot create a free crop"),F.State->ExportState().Crops.IsEmpty());
    TestEqual(TEXT("Failed completion leaves the plot definition empty"),Plot->DefinitionID,NAME_None);
    F.Bag->AddItem(Seed,1,false);
    TestTrue(TEXT("Failed completion leaves the plot usable"),F.Gather->StartPlant(Plot,F.PC,CropID));F.Step(5);
    TestEqual(TEXT("Successful retry consumes one seed"),F.Bag->CountItem(Seed),0);
    TestEqual(TEXT("Successful retry creates exactly one crop"),F.State->ExportState().Crops.Num(),1);
    TestFalse(TEXT("Completion cannot replace an occupied plot"),Plot->CompletePlayerAction(F.PC,TEXT("CRP_R01_02"),Failure));
    TestEqual(TEXT("Rejected completion retains original crop"),Plot->DefinitionID,CropID);
    F.Time->JumpGameMinutes(1440*6);Plot->RefreshLife();
    TestTrue(TEXT("A planted crop grows and can be harvested"),Plot->CompletePlayerAction(F.PC,NAME_None,Failure));
    TestTrue(TEXT("Harvest frees the saved plot"),F.State->ExportState().Crops.IsEmpty());
    F.Bag->AddItem(Seed,1,false);
    TestTrue(TEXT("Harvested plot can be replanted"),F.Gather->StartPlant(Plot,F.PC,CropID));F.Step(5);
    TestEqual(TEXT("Replanting consumes exactly one seed"),F.Bag->CountItem(Seed),0);
    return !HasAnyErrors();
}
#endif
