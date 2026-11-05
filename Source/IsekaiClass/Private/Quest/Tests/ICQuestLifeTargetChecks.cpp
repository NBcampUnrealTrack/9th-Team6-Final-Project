#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Quest/ICQuestLifeTarget.h"
#include "Life/World/ICLifeActor.h"
#include "Life/World/ICLifeTargetSubsystem.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Quest/ICQuestGuideComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Quest/ICQuestGuideLight.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestLifeTargetMatchCheck, "IC.Runtime.QuestGuide.LifeTargetMatching",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestLifeTargetMatchCheck::RunTest(const FString& Parameters)
{
    FICQuestObjectiveDefinition Objective;
    Objective.Type = EICQuestObjectiveType::DeliverItem;
    Objective.Target.Type = EICDefinitionType::Item;
    Objective.Target.ID = TEXT("Mushroom");
    FICItemAmount Yield; Yield.ItemID = TEXT("Mushroom"); Yield.Count = 1;
    auto Matches = [&Objective, &Yield](EICLifeActivity Activity)
    {
        return ICQuestLifeTarget::Matches(Objective, EICDefinitionType::Resource,
            TEXT("Source"), TEXT("Gather"), Activity, {Yield});
    };
    TestTrue(TEXT("Delivery finds the resource by its item yield"), Matches(EICLifeActivity::Gathering));
    Yield.ItemID = TEXT("Other");
    TestFalse(TEXT("Different yields cannot guide a delivery"), Matches(EICLifeActivity::Gathering));
    Yield.ItemID = TEXT("Mushroom"); Yield.Count = 0;
    TestFalse(TEXT("Empty yields are ignored"), Matches(EICLifeActivity::Gathering));
    Yield.Count = 1;
    Objective.Type = EICQuestObjectiveType::Craft;
    Objective.bFilterLifeActivity = true; Objective.LifeActivity = EICLifeActivity::Fishing;
    TestFalse(TEXT("Activity restrictions exclude gathering"), Matches(EICLifeActivity::Gathering));
    TestTrue(TEXT("Fish output can satisfy an item activity target"), Matches(EICLifeActivity::Fishing));
    Objective.OutputItemID = TEXT("Other");
    TestFalse(TEXT("Output filter is required in addition to target"), Matches(EICLifeActivity::Fishing));
    Objective.OutputItemID = NAME_None;
    Objective.Target.Type = EICDefinitionType::Production; Objective.Target.ID = TEXT("Plant");
    Objective.LifeActivity = EICLifeActivity::Planting;
    TestTrue(TEXT("Planting action resolves without an immediate item yield"),
        ICQuestLifeTarget::Matches(Objective, EICDefinitionType::Crop, TEXT("Crop"), TEXT("Plant"), EICLifeActivity::Planting, {}));
    Objective.Target.Type = EICDefinitionType::Crop; Objective.Target.ID = TEXT("Crop");
    TestTrue(TEXT("Crop-specific planting resolves by crop definition"),
        ICQuestLifeTarget::Matches(Objective, EICDefinitionType::Crop, TEXT("Crop"), TEXT("Plant"), EICLifeActivity::Planting, {}));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestLifeTargetRegistryCheck, "IC.Runtime.QuestGuide.LifeTargetRegistryAndWater",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestLifeTargetRegistryCheck::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World exists"), World)) return false;
    auto* Registry = World->GetSubsystem<UICLifeTargetSubsystem>();
    auto* Actor = World->SpawnActor<AICLifeActor>();
    if (TestNotNull(TEXT("Registry exists"), Registry) && TestNotNull(TEXT("Life actor exists"), Actor))
    {
        Registry->Register(Actor); Registry->Register(Actor);
        TestEqual(TEXT("Registration is unique"), Registry->GetTargets().Num(), 1);
        Actor->LifeType = EICDefinitionType::FishingPoint;
        Actor->VisualMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Actor->VisualMesh->SetRelativeRotation(FRotator(0, 37, 0));
        Actor->VisualMesh->SetWorldScale3D(FVector(10, 3, .2));
        TArray<FVector> Samples; Actor->GetFishingGuideSamples(FVector(2000, 0, 0), Samples);
        TestTrue(TEXT("Fishing surfaces generate shore samples"), Samples.Num() >= 20);
        for (const FVector& Point : Samples)
            TestTrue(TEXT("Samples stay on a rotated fishing surface"), Actor->ContainsFishingLanding(Point));
        Registry->Unregister(Actor);
        TestTrue(TEXT("Unloaded targets leave the registry"), Registry->GetTargets().IsEmpty());
    }
    World->DestroyWorld(false);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestLifeTargetSelectionCheck, "IC.Runtime.QuestGuide.LifeTargetSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestLifeTargetSelectionCheck::RunTest(const FString& Parameters)
{
    UGameInstance* GI = NewObject<UGameInstance>(GEngine);
    GI->InitializeStandalone();
    UWorld* World = GI->GetWorld();
    auto* State = GI->GetSubsystem<UICWorldStateSubsystem>();
    auto* Clock = GI->GetSubsystem<UICTimeSubsystem>();
    auto* Registry = World->GetSubsystem<UICLifeTargetSubsystem>();
    auto* Pawn = World->SpawnActor<APawn>();
    auto* Guide = NewObject<UICQuestGuideComponent>(Pawn);
    auto* Near = World->SpawnActor<AICLifeActor>();
    auto* Far = World->SpawnActor<AICLifeActor>();
    Near->DefinitionID = Far->DefinitionID = TEXT("RES_R01_GATHER_07");
    Near->PointID = TEXT("GuideNear"); Far->PointID = TEXT("GuideFar");
    Near->SetActorLocation(FVector(40, 0, 0)); Far->SetActorLocation(FVector(100, 0, 0));
    Registry->Register(Near); Registry->Register(Far);
    FICQuestObjectiveDefinition Objective;
    Objective.ObjectiveID = TEXT("MushroomDelivery");
    Objective.Type = EICQuestObjectiveType::DeliverItem;
    Objective.Target.Type = EICDefinitionType::Item;
    Objective.Target.ID = TEXT("ITM_R01_PLANT_07");
    FICWorldState Snapshot;
    auto* Inventory = NewObject<UICInventoryComponent>();
    Inventory->BeginStaging(GI->GetSubsystem<UICDefinitionSubsystem>(), 0);
    Snapshot.Inventory = Inventory->GetSnapshot();
    TestTrue(TEXT("Initial availability state restores"), State->RestoreState(Snapshot));
    FVector Goal; FName Key;
    TestTrue(TEXT("Existing mushroom delivery finds a placed resource without an anchor"),
        Guide->ResolveLifeTarget(Objective, State->ExportState(), Goal, Key));
    TestTrue(TEXT("Nearest available mushroom wins"), Goal.Equals(Near->GetActorLocation()));
    const FName FirstKey = Key;
    Snapshot = State->ExportState();
    Snapshot.ResourceReadyAtGameMinute.Add(Near->PointID, Clock->GetClock().GameMinute + 300);
    TestTrue(TEXT("Depleted availability state restores"), State->RestoreState(Snapshot));
    TestTrue(TEXT("Depleting the nearest resource switches to another"),
        Guide->ResolveLifeTarget(Objective, State->ExportState(), Goal, Key));
    TestTrue(TEXT("The new target is the farther available mushroom"), Goal.Equals(Far->GetActorLocation()));
    TestTrue(TEXT("Switching resources changes the pulse target key"), FirstKey != Key);
    Far->SetActorLocation(FVector(1000, 0, 0));
    TestFalse(TEXT("A distant resource without a navigation path is excluded"),
        Guide->ResolveLifeTarget(Objective, State->ExportState(), Goal, Key));
    Registry->Unregister(Far);
    TestFalse(TEXT("Unloaded and depleted resources cannot be selected"),
        Guide->ResolveLifeTarget(Objective, State->ExportState(), Goal, Key));
    Registry->Unregister(Near);
    GI->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestGuideOpenWorldCheck, "IC.Runtime.QuestGuide.OpenWorldMushroomRouteAndVFX",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestGuideOpenWorldCheck::RunTest(const FString& Parameters)
{
    if (GEditor->PlayWorld) { AddError(TEXT("Close PIE before checking the saved OpenWorld")); return false; }
    if (!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/Maps/L_OpenWorld"), false, true)) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    UGameInstance* GI = NewObject<UGameInstance>(GEngine);
    GI->InitializeStandalone();
    UWorld* FixtureWorld = GI->GetWorld();
    UGameInstance* PreviousGI = World->GetGameInstance();
    World->SetGameInstance(GI);
    auto* Registry = World->GetSubsystem<UICLifeTargetSubsystem>();
    FVector Start = FVector::ZeroVector;
    for (TActorIterator<APlayerStart> It(World); It; ++It) { Start = It->GetActorLocation(); break; }
    int32 PlotCount = 0, FishingCount = 0;
    for (TActorIterator<AICLifeActor> It(World); It; ++It)
    {
        Registry->Register(*It);
        PlotCount += It->bPlantingPlot;
        FishingCount += It->LifeType == EICDefinitionType::FishingPoint;
    }
    TestTrue(TEXT("Saved map contains planting plots and a fishing zone"), PlotCount > 0 && FishingCount > 0);
    // Test fixtures exist only for this run. The authored level need not contain mushrooms.
    TArray<AICLifeActor*> TestResources;
    const FVector Locations[] = {FVector(-53840, 629440, -5193.3658), FVector(-54190, 629190, -5197.976)};
    for (int32 Index = 0; Index < 2; ++Index)
    {
        UClass* ResourceClass = LoadClass<AICLifeActor>(nullptr, *FString::Printf(
            TEXT("/Game/IsekaiClass/Life/Collection/BP_R01_Collection_%02d.BP_R01_Collection_%02d_C"), 7 + Index, 7 + Index));
        if (!TestNotNull(TEXT("Production mushroom class exists"), ResourceClass)) continue;
        FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
        AICLifeActor* Resource = World->SpawnActor<AICLifeActor>(ResourceClass, Locations[Index], FRotator::ZeroRotator, Params);
        if (!TestNotNull(TEXT("Temporary mushroom fixture spawns"), Resource)) continue;
        Registry->Register(Resource); TestResources.Add(Resource);
    }
    UClass* PlayerClass = LoadClass<AICPlayerCharacter>(nullptr,
        TEXT("/Game/IsekaiClass/Player/Blueprints/BP_ICPlayerCharacter.BP_ICPlayerCharacter_C"));
    const auto* PlayerDefaults = PlayerClass ? PlayerClass->GetDefaultObject<AICPlayerCharacter>() : nullptr;
    APawn* Pawn = World->SpawnActor<APawn>();
    // APawn has no movable root by default; a test scene component supplies its start position.
    auto* Root = NewObject<USceneComponent>(Pawn); Pawn->SetRootComponent(Root); Root->RegisterComponent();
    Pawn->SetActorLocation(Start);
    auto* Guide = NewObject<UICQuestGuideComponent>(Pawn);
    Pawn->AddInstanceComponent(Guide); Guide->RegisterComponent();
    if (PlayerDefaults && PlayerDefaults->QuestGuide) Guide->GuideLightClass = PlayerDefaults->QuestGuide->GuideLightClass;
    FICQuestRow Definition;
    const bool bDefinition = GI->GetSubsystem<UICDefinitionSubsystem>()->GetQuest(TEXT("QST_SCN_R01_004"), Definition).bSuccess;
    TestTrue(TEXT("Actual early quest definition loads"), bDefinition);
    FICWorldState Snapshot = GI->GetSubsystem<UICWorldStateSubsystem>()->ExportState();
    if (bDefinition)
    {
        for (const auto& Group : Definition.ObjectiveGroups) for (const auto& Objective : Group.Objectives)
        {
            FVector Goal; FName Key; TArray<FVector> Points;
            const bool bFound = Guide->ResolveLifeTarget(Objective, Snapshot, Goal, Key);
            TestTrue(TEXT("Actual delivery objective resolves a placed mushroom"), bFound);
            if (bFound && TestTrue(TEXT("PlayerStart has a full guide route to the mushroom"), Guide->BuildGuidePath(*Pawn, Goal, Points)))
            {
                Guide->SpawnPulse(*Pawn, Points);
                if (TestEqual(TEXT("Resolved route spawns a guide pulse"), Guide->ActiveLights.Num(), 1))
                {
                    AICQuestGuideLight* Light = Guide->ActiveLights[0];
                    auto* Niagara = Light->FindComponentByClass<UNiagaraComponent>();
                    TestTrue(TEXT("Guide actor is playing and visible"), Light->IsPlaying() && !Light->IsHidden());
                    TestTrue(TEXT("Production pulse has the authored Niagara system"), Niagara && Niagara->GetAsset()
                        && Niagara->GetAsset()->GetFName() == TEXT("NS_IC_QuestGuideWisp"));
                }
                Guide->ResetPulse();
            }
        }
    }
    Pawn->Destroy();
    for (AICLifeActor* Resource : TestResources) { Registry->Unregister(Resource); Resource->Destroy(); }
    World->SetGameInstance(PreviousGI);
    GI->Shutdown(); GEngine->DestroyWorldContext(FixtureWorld); FixtureWorld->DestroyWorld(false);
    return !HasAnyErrors();
}

#endif
