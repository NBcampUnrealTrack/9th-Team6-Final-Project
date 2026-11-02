#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Inventory/Rewards/ICGachaService.h"
#include "Inventory/Actors/ICDroppedItem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Life/Gacha/ICGachaMachine.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Components/StaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGachaCheck,"IC.Inventory.GachaAndCubeDrops",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGachaCheck::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();World->InitializeActorsForPlay(FURL(),false);
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();FICDefinitionResult R;
    const auto* Rule=D->Find<FICGachaMachineRow>(TEXT("GAC_R01_BASIC_001"),R);
    if(!TestNotNull(TEXT("Actual gacha DT is registered"),Rule)){GI->Shutdown();World->DestroyWorld(false);return false;}
    TestEqual(TEXT("Draw costs 1000G by default"),Rule->CostGold,1000);TestEqual(TEXT("Two independently weighted groups"),Rule->Pools.Num(),2);
    TestEqual(TEXT("Gathering pool has weight 50"),Rule->Pools[0].Weight,50.);TestEqual(TEXT("Book pool has weight 50"),Rule->Pools[1].Weight,50.);
    TestEqual(TEXT("Ten gathering items"),Rule->Pools[0].Items.Num(),10);TestEqual(TEXT("Fifteen usable elixirs"),Rule->Pools[1].Items.Num(),15);
    TestEqual(TEXT("Growth rewards use the elixir pool"),Rule->Pools[1].PoolID,FName(TEXT("GROWTH_ELIXIR")));
    for(const auto& Prize:Rule->Pools[1].Items)
    {
        const auto* Elixir=D->Find<FICItemRow>(Prize.ItemID,R);
        if(!TestNotNull(TEXT("Every elixir prize resolves"),Elixir))continue;
        TestTrue(TEXT("Elixir category and use effect match"),Elixir->Category==EPlayerItemCategory::Elixir&&Elixir->UseEffect==EICItemEffect::GrowthElixir);
        TestTrue(TEXT("Elixirs use potion icons"),Elixir->Icon.ToSoftObjectPath().ToString().Contains(TEXT("Potion")));
        TestTrue(TEXT("Runtime item labels are elixirs"),Elixir->DisplayName.ToString().Contains(TEXT("엘릭서")));
    }
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("뽑기 검증");Initial.FirstSkill=TEXT("SKL_NPC_R01_002");Initial.Gold=10000;
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);
    int32 Gathering=0,Books=0;TSet<EPlayerMainStat> Attributes;
    for(int32 Seed=1;Seed<=256;++Seed)
    {
        auto Next=Initial;FRandomStream Random(Seed);FICGachaDrawResult Result;
        TestTrue(TEXT("Weighted draw prepares"),FICGachaService::PrepareDraw(Next,*Rule,D,Random,FVector::ZeroVector,0,Result));
        TestEqual(TEXT("One draw charges exactly once"),Next.Gold,9000);TestEqual(TEXT("One result per default draw"),Result.Prize.Count,1);
        TestEqual(TEXT("Space available result goes to bag"),Result.ReceivedCount,1);
        const FString ID=Result.Prize.ItemID.ToString();
        if(ID.StartsWith(TEXT("ITM_R01_PLANT_")))++Gathering;
        else if(ID.StartsWith(TEXT("ITM_ELIXIR_"))){++Books;const auto* Book=D->Find<FICItemRow>(Result.Prize.ItemID,R);if(Book)Attributes.Add(Book->GrowthStat);}
        else AddError(TEXT("Draw returned an item outside authored pools"));
    }
    TestTrue(TEXT("Both pools receive draws around the configured half share"),Gathering>80&&Gathering<176&&Books>80&&Books<176);
    TestEqual(TEXT("All five active growth attributes are possible"),Attributes.Num(),5);
    auto Changed=*Rule;Changed.Pools[0].Weight=0;Changed.Pools[1].Items.SetNum(1);Changed.Pools[1].Items[0].MinimumCount=Changed.Pools[1].Items[0].MaximumCount=3;Changed.CostGold=250;
    auto Next=Initial;FRandomStream Random(42);FICGachaDrawResult Draw;
    TestTrue(TEXT("Edited pool, item count and cost are used"),FICGachaService::PrepareDraw(Next,Changed,D,Random,FVector::ZeroVector,0,Draw));
    TestEqual(TEXT("Edited cost applies"),Next.Gold,9750);TestEqual(TEXT("Disabled gathering pool never wins"),Draw.Prize.ItemID,Changed.Pools[1].Items[0].ItemID);TestEqual(TEXT("Edited quantity applies"),Draw.Prize.Count,3);
    Next=Initial;Next.Gold=999;TestFalse(TEXT("Insufficient gold rejects draw"),FICGachaService::PrepareDraw(Next,*Rule,D,Random,FVector::ZeroVector,0,Draw));TestEqual(TEXT("Rejected draw does not charge"),Next.Gold,999);
    Changed.Pools[1].Weight=0;Next=Initial;TestFalse(TEXT("All disabled pools reject safely"),FICGachaService::PrepareDraw(Next,Changed,D,Random,FVector::ZeroVector,0,Draw));TestEqual(TEXT("Bad configuration never charges"),Next.Gold,10000);

    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    TArray<FName> ItemIDs;D->GetDefinitionIDs(EICDefinitionType::Item,ItemIDs);
    for(FName ID:ItemIDs){const auto* Item=D->Find<FICItemRow>(ID,R);TestTrue(TEXT("Current item drops default to cubes"),Item&&Item->WorldMesh.LoadSynchronous()==Cube);}
    FICDropState Record;Record.ID=FGuid::NewGuid();FPlayerItemInstance Prize;Prize.ItemID=TEXT("ITM_R01_PLANT_01");Prize.Count=1;Prize.InstanceID=FGuid::NewGuid();Record.Contents.Add(Prize);
    auto* Drop=World->SpawnActor<AICDroppedItem>();Drop->Initialize(Record);TestEqual(TEXT("Actual dropped actor displays the cube"),Drop->Mesh->GetStaticMesh().Get(),Cube);
    auto* Definition=const_cast<FICItemRow*>(D->Find<FICItemRow>(Prize.ItemID,R));
    {
        TGuardValue<TSoftObjectPtr<UStaticMesh>> Shape(Definition->WorldMesh,TSoftObjectPtr<UStaticMesh>(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"))));
        TGuardValue<FVector> Scale(Definition->WorldMeshScale,FVector(.6,.7,.8));Drop->Initialize(Record);
        TestEqual(TEXT("Edited DT mesh changes the actual drop"),Drop->Mesh->GetStaticMesh().Get(),Definition->WorldMesh.Get());TestTrue(TEXT("Edited DT scale applies"),Drop->Mesh->GetRelativeScale3D().Equals(Definition->WorldMeshScale));
        Definition->WorldMesh.Reset();Drop->Initialize(Record);TestEqual(TEXT("An empty DT mesh restores the cube fallback"),Drop->Mesh->GetStaticMesh().Get(),Cube);
    }
    TestTrue(TEXT("Initial gacha world restores"),State->RestoreState(Initial));
    auto* PC=World->SpawnActor<AICPlayerController>();auto* Pawn=World->SpawnActor<AICPlayerCharacter>();PC->Possess(Pawn);Pawn->Inventory->BindWorldState(State);PC->bCreated=true;PC->Menu.Empty();
    auto* Type=LoadClass<AICGachaMachine>(nullptr,TEXT("/Game/IsekaiClass/Life/Gacha/BP_IC_GachaMachine.BP_IC_GachaMachine_C"));
    auto* Machine=Type?World->SpawnActor<AICGachaMachine>(Type):nullptr;
    if(TestNotNull(TEXT("Authored cube machine BP loads"),Machine))
    {
        Pawn->SetActorLocation(Machine->GetActorLocation()+FVector(180,0,0));
        TestTrue(TEXT("F interaction commits a paid draw"),Machine->TryInteract(PC));TestEqual(TEXT("Machine updates wallet"),PC->Gold,9000);
        auto Full=Initial;for(auto& Item:Full.Inventory.Bag.Slots){Item.ItemID=TEXT("ITM_R01_SEED_01");Item.Count=99;Item.InstanceID=FGuid::NewGuid();}
        TestTrue(TEXT("Full bag state restores"),State->RestoreState(Full));
        TestTrue(TEXT("Full bag still grants the result as a world drop"),Machine->TryInteract(PC));
        const auto Overflow=State->ExportState();TestEqual(TEXT("Overflow draw charges once"),Overflow.Gold,9000);TestEqual(TEXT("Exactly one overflow drop record"),Overflow.Drops.Num(),1);
        if(!Overflow.Drops.IsEmpty())
        {
            Drop->Initialize(Overflow.Drops[0]);Drop->SetActorLocation(Pawn->GetActorLocation());TestEqual(TEXT("Overflow prize displays as a cube"),Drop->Mesh->GetStaticMesh().Get(),Cube);
            auto Room=Overflow.Inventory;Room.Bag.Slots[0]={};Pawn->Inventory->ApplySnapshot(Room);Drop->TryPickup(PC);
            TestTrue(TEXT("Overflow prize can be picked up"),State->ExportState().Drops.IsEmpty());
        }
        PC->SetGold(999);TestFalse(TEXT("F with insufficient funds fails"),Machine->TryInteract(PC));TestEqual(TEXT("Failed interaction keeps wallet"),PC->Gold,999);
    }
    GI->Shutdown();World->DestroyWorld(false);return !HasAnyErrors();
}
#endif
