#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Life/World/ICLifeActor.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveGame.h"
#include "Quest/ICQuestSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLifePrototypeCheck,"IC.Life.R01AssetsAndSavedGrowth",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICLifePrototypeCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    auto* PC=World->SpawnActor<AICPlayerController>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("생활검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);
    auto* Staging=NewObject<UICInventoryComponent>();Staging->BeginStaging(D,0);
    for(int32 Index=1;Index<=5;++Index)TestTrue(TEXT("Authored seed item exists"),Staging->AddItem(FName(*FString::Printf(TEXT("ITM_R01_SEED_%02d"),Index)),1,false));
    Initial.Inventory=Staging->GetSnapshot();TestTrue(TEXT("Initial life world restores"),State->RestoreState(Initial));
    Time->SetRunning(true);Time->SetMenuPaused(false);FICDefinitionResult R;TArray<AICLifeActor*> Plots;
    const int32 Days[]={5,4,5,2,2};
    for(int32 Index=1;Index<=5;++Index)
    {
        const FName ID(*FString::Printf(TEXT("CRP_R01_%02d"),Index));const auto* Crop=D->Find<FICCropRow>(ID,R);
        if(!TestNotNull(TEXT("Real crop DT row"),Crop))continue;
        TestEqual(TEXT("Original visible meshes are the complete stage list"),Crop->GrowthStages.Num(),Days[Index-1]);
        TestFalse(TEXT("First growth stage has a real mesh"),Crop->GrowthStages[0].Mesh.IsNull());
        auto* Type=Crop->CropActorClass.LoadSynchronous();if(!TestNotNull(TEXT("Real crop BP"),Type))continue;
        auto* Plot=World->SpawnActor<AICLifeActor>(Type,FVector(Index*400,0,0),FRotator::ZeroRotator);Plots.Add(Plot);
        Plot->RefreshLife();
        TestEqual(TEXT("Natural crop starts at its first stage"),Plot->GetGrowthStage(),0);TestNotNull(TEXT("First stage is visible"),Plot->VisualMesh->GetStaticMesh().Get());
        TestFalse(TEXT("Growing crop cannot be harvested early"),Plot->TryInteract(PC));
        TestFalse(TEXT("An occupied plot rejects duplicate planting"),Plot->Plant(PC));
    }
    for(int32 Day=1;Day<=5;++Day)
    {
        TestTrue(TEXT("An actual game day advances"),Time->JumpGameMinutes(1440));
        for(int32 Index=0;Index<Plots.Num();++Index)
        {
            auto* Plot=Plots[Index];Plot->RefreshLife();const int32 Expected=FMath::Min(Day,Days[Index]-1);
            TestEqual(TEXT("Crop advances exactly one stage per day"),Plot->GetGrowthStage(),Expected);
            TestEqual(TEXT("Only the last stage can be harvested"),Plot->IsHarvestable(),Day>=Days[Index]-1);
            TestNotNull(TEXT("Grown crop displays its DT mesh"),Plot->VisualMesh->GetStaticMesh().Get());
        }
        if(Day==1)
        {
            auto* Save=NewObject<UICSaveGame>();Save->WorldState=State->ExportState();Save->Clock=Time->GetClock();
            TArray<uint8> Bytes;TestTrue(TEXT("Growing crops serialize through the real save format"),UICSaveGame::EncodeForFile(Save,Bytes));
            auto* Loaded=UICSaveGame::DecodeFile(Bytes);
            if(TestNotNull(TEXT("Crop save decodes"),Loaded))
            {TestTrue(TEXT("Saved game clock restores"),Time->RestoreClock(Loaded->Clock));TestTrue(TEXT("Saved crop state restores"),State->RestoreState(Loaded->WorldState));for(auto* Plot:Plots)TestEqual(TEXT("Loading retains crop stage"),Plot->GetGrowthStage(),1);}
        }
    }
    for(auto* Plot:Plots)
    {FText Reason;TestTrue(TEXT("Ripe crop grants its actual yield at work completion"),Plot->CompletePlayerAction(PC,NAME_None,Reason));TestEqual(TEXT("Harvest hides the crop during regrowth"),Plot->GetGrowthStage(),INDEX_NONE);TestFalse(TEXT("Cooldown cannot grant duplicate harvest"),Plot->CompletePlayerAction(PC,NAME_None,Reason));TestNull(TEXT("Harvested crop has no visible mesh"),Plot->VisualMesh->GetStaticMesh().Get());}
    const auto Dormant=State->ExportState();
    TestTrue(TEXT("A harvested crop state can be saved"),UICWorldStateSubsystem::ValidateStateClock(Dormant,Time->GetClock()));
    TestTrue(TEXT("119 calendar minutes elapse"),Time->JumpGameMinutes(119));
    for(auto* Plot:Plots)TestEqual(TEXT("Regrowth never happens before two game hours"),Plot->GetGrowthStage(),INDEX_NONE);
    TestTrue(TEXT("One more calendar minute elapses"),Time->JumpGameMinutes(1));
    for(auto* Plot:Plots){Plot->RefreshLife();TestEqual(TEXT("Two game hours restarts at stage one"),Plot->GetGrowthStage(),0);TestFalse(TEXT("A regenerated crop is not mature"),Plot->IsHarvestable());Plot->Destroy();}
    for(int32 Index=1;Index<=10;++Index)
    {
        const auto* Row=D->Find<FICResourceRow>(FName(*FString::Printf(TEXT("RES_R01_GATHER_%02d"),Index)),R);
        TestTrue(TEXT("Every requested R01 gathering plant has a BP, mesh and yield"),Row&&Row->ResourceActorClass.LoadSynchronous()&&Row->AvailableMesh.LoadSynchronous()&&!Row->Yields.IsEmpty());
        const auto* Fish=D->Find<FICFishRow>(FName(*FString::Printf(TEXT("FSH_R01_%02d"),Index)),R);
        TestTrue(TEXT("Every fish has matching mesh and animation BP"),Fish&&Fish->FishActorClass.LoadSynchronous()&&Fish->SkeletalMesh.LoadSynchronous()&&Fish->AnimationClass.LoadSynchronous());
    }
    for(int32 Index=1;Index<=6;++Index)
    {
        const auto* Row=D->Find<FICResourceRow>(FName(*FString::Printf(TEXT("RES_R01_ORE_%02d"),Index)),R);
        auto* Type=Row?Row->ResourceActorClass.LoadSynchronous():nullptr;auto* Ore=Type?Cast<AICLifeActor>(Type->GetDefaultObject()):nullptr;
        TestTrue(TEXT("Every ore uses geometry collection C and a material"),Ore&&Ore->OreMesh->GetRestCollection()&&Ore->OreMesh->GetMaterial(0));
    }
    for(int32 Index=1;Index<=4;++Index)
    {
        const auto* Row=D->Find<FICRanchingRow>(FName(*FString::Printf(TEXT("RAN_R01_%02d"),Index)),R);
        TestTrue(TEXT("Ranching DT links the animal BP, mesh and products"),Row&&Row->AnimalActorClass.LoadSynchronous()&&Row->Mesh.LoadSynchronous()&&!Row->Products.IsEmpty());
    }
    for(int32 Index=1;Index<=30;++Index)
    {
        const auto* Recipe=D->Find<FICRecipeRow>(FName(*FString::Printf(TEXT("RCP_R01_COOK_%02d"),Index)),R);
        if(!TestNotNull(TEXT("Each of thirty cooking recipes exists"),Recipe))continue;
        const auto* Production=D->Find<FICProductionRow>(Recipe->ProductionID,R);
        TestTrue(TEXT("Recipe links to actual cooking production"),Production&&Production->Activity==EICLifeActivity::Cooking&&Production->Results.Num()==1);
        for(const auto& Ingredient:Recipe->Ingredients)
        {TestNotNull(TEXT("Recipe ingredient is an actual item"),D->Find<FICItemRow>(Ingredient.ItemID,R));TestFalse(TEXT("Recipes do not use cow, pig or sheep"),Ingredient.ItemID==TEXT("ITM_R01_RANCH_02")||Ingredient.ItemID==TEXT("ITM_R01_RANCH_03")||Ingredient.ItemID==TEXT("ITM_R01_RANCH_04"));}
    }
    const auto* Fishing=D->Find<FICFishingPointRow>(TEXT("FPT_R01_RANDOM_001"),R);
    if(TestNotNull(TEXT("Future fishing pool exists"),Fishing))
    {
        TestEqual(TEXT("Fishing pool contains fish 1..10"),Fishing->FishSelections.Num(),10);
        for(const auto& Selection:Fishing->FishSelections)TestEqual(TEXT("Each fish has the same future random weight"),Selection.Weight,1.);
        auto* Type=Fishing->PointActorClass.LoadSynchronous();auto* Box=Type?Cast<AICLifeActor>(Type->GetDefaultObject()):nullptr;
        TestTrue(TEXT("Fishing BP displays its name label"),Box&&Box->bShowLifeLabel);
        TestTrue(TEXT("Fishing BP is a fishing point rather than a resource"),Box&&Box->LifeType==EICDefinitionType::FishingPoint);
    }
    PC->Destroy();GI->Shutdown();World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCQuestMarkerCheck,"IC.Quest.NPCOverheadMarkers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICNPCQuestMarkerCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Q=GI->GetSubsystem<UICQuestSubsystem>();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("퀘스트표시");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Marker world initialized"),State->RestoreState(Initial));
    TestEqual(TEXT("Quest giver has green exclamation marker"),Q->GetNPCQuestMarker(TEXT("NPC_R01_001")),1);
    TestEqual(TEXT("NPC without offered quest has no marker"),Q->GetNPCQuestMarker(TEXT("NPC_R01_012")),0);
    TestTrue(TEXT("Accept first sample quest"),Q->AcceptQuest(TEXT("QST_NPC_TALK_001")));
    TestTrue(TEXT("Start first sample quest"),Q->StartQuest(TEXT("QST_NPC_TALK_001")));
    TestEqual(TEXT("Current talk target takes orange question priority"),Q->GetNPCQuestMarker(TEXT("NPC_R01_002")),2);
    TestEqual(TEXT("Accepted giver loses availability marker"),Q->GetNPCQuestMarker(TEXT("NPC_R01_001")),0);
    const auto Saved=State->ExportState();
    TestTrue(TEXT("Accept second sample quest"),Q->AcceptQuest(TEXT("QST_NPC_TALK_002")));
    TestTrue(TEXT("Switch the started quest"),Q->StartQuest(TEXT("QST_NPC_TALK_002")));
    TestEqual(TEXT("Previous target loses its question"),Q->GetNPCQuestMarker(TEXT("NPC_R01_002")),0);
    TestEqual(TEXT("New target receives the question"),Q->GetNPCQuestMarker(TEXT("NPC_R01_003")),2);
    TestTrue(TEXT("Loading quest state restores"),State->RestoreState(Saved));
    TestEqual(TEXT("Load restores the question on the correct NPC"),Q->GetNPCQuestMarker(TEXT("NPC_R01_002")),2);
    FICQuestRow TalkQuest;
    GI->GetSubsystem<UICDefinitionSubsystem>()->GetQuest(TEXT("QST_NPC_TALK_001"), TalkQuest);
    Q->ReportObjectiveEvent(EICQuestObjectiveType::Talk,TEXT("NPC_R01_002"),1,FGuid::NewGuid(),
        EICLifeActivity::Gathering,NAME_None,TalkQuest.ObjectiveGroups[0].Objectives[0].DialogueID);
    TestEqual(TEXT("Completion removes question and reveals that NPC's available quest"),Q->GetNPCQuestMarker(TEXT("NPC_R01_002")),1);
    TestTrue(TEXT("Accept another quest for abandon"),Q->AcceptQuest(TEXT("QST_NPC_TALK_002")));
    TestTrue(TEXT("Start another quest for abandon"),Q->StartQuest(TEXT("QST_NPC_TALK_002")));
    TestTrue(TEXT("Abandon started quest"),Q->AbandonQuest(TEXT("QST_NPC_TALK_002")));
    TestNotEqual(TEXT("Abandon removes target question"),Q->GetNPCQuestMarker(TEXT("NPC_R01_003")),2);
    GI->Shutdown();World->DestroyWorld(false);return true;
}
#endif
