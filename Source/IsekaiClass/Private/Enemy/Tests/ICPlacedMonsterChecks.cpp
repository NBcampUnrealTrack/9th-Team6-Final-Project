#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "AI/ICAIController.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Save/ICSaveGame.h"
#include "Save/ICSaveSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "World/Spawn/ICPlacedMonsterSpawnPoint.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

namespace ICPlacedMonsterChecks
{
struct FPlacement
{
    TWeakObjectPtr<AICEnemyBase> EditorActor;
    FGuid Guid;FName MonsterID;FVector Origin;
};
class FPlacedCommand : public IAutomationLatentCommand
{
public:
    FPlacedCommand(FAutomationTestBase* InTest,TArray<FPlacement> InPlacements):Test(InTest),Placements(MoveTemp(InPlacements)),Started(FPlatformTime::Seconds()){}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if(Now-Started>100){Test->AddError(FString::Printf(TEXT("Placed monster integration timed out at stage %d"),Stage));Cleanup();return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World?Cast<AICPlayerController>(World->GetFirstPlayerController()):nullptr;if(!PC)return false;
        auto* Spawns=World->GetSubsystem<UICSpawnSubsystem>();auto* State=PC->GetWorldState();auto* Time=PC->GetTime();
        if(Stage==0)
        {
            if(PC->Menu!=TEXT("Create")||!PC->GetSaves()->OnSaveFinished.Contains(PC,TEXT("SaveFinished")))return false;
            GI=PC->GetGameInstance();
            Test->TestTrue(TEXT("Placed monster saves are isolated"),PC->GetSaves()->ConfigureServices(State,Time,TEXT("IC_Placed_")+FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            int32 Waiting=0;for(TActorIterator<AICEnemyBase> It(World);It;++It)if(It->PlacementGuid.IsValid()){++Waiting;Test->TestFalse(TEXT("Map copies wait for a created player"),It->HasInitializedEnemy());Test->TestTrue(TEXT("PIE preserves editor placement identity"),Placements.ContainsByPredicate([&](const auto& P){return P.Guid==It->PlacementGuid;}));}
            Test->TestEqual(TEXT("All manually placed copies reached BeginPlay"),Waiting,Placements.Num());
            if(!Test->TestTrue(TEXT("Create a real GAS player"),PC->DebugCreatePlayer())){Cleanup();return true;}
            Stage=1;return false;
        }
        auto* Player=Cast<AICPlayerCharacter>(PC->GetPawn());if(!Player)return false;
        if(Stage==1)
        {
            if(!PC->bCreated||PC->GetSaves()->bSaving)return false;
            Player->SetActorLocation(FVector(-4000,-12000,100));Player->GetCharacterMovement()->DisableMovement();
            Spawns->RefreshSpawns();Stage=2;StageAt=Now;return false;
        }
        if(Stage==2)
        {
            if(Now-StageAt<1)return false;
            TSet<FName> Keys;int32 Count=0,DTSpawns=0,Moved=0;
            for(TActorIterator<AICEnemyBase> It(World);It;++It)
            {
                auto* Enemy=*It;
                if(!Enemy->GetSpawnKey().ToString().StartsWith(TEXT("PLACED_MONSTER_"))){++DTSpawns;continue;}
                ++Count;Keys.Add(Enemy->GetSpawnKey());
                const auto* Placement=Placements.FindByPredicate([&](const auto& P){return P.Guid==Enemy->PlacementGuid;});
                if(!Test->TestNotNull(TEXT("PIE preserves each authored placement GUID"),Placement))continue;
                Test->TestEqual(TEXT("Each BP reads its own preset DT monster ID"),Enemy->GetMonsterDefinition().ID,Placement->MonsterID);
                Test->TestTrue(TEXT("Authored placement is the return origin"),Enemy->GetOrigin().Equals(Placement->Origin,1));
                Test->TestTrue(TEXT("Placement radius is enforced"),Enemy->IsOutsideLeash(Enemy->GetOrigin()+FVector(Enemy->PlacedLeashRadiusCm+10,0,0)));
                Test->TestTrue(TEXT("DT health initializes GAS"),FMath::IsNearlyEqual(Enemy->GetICAbilitySystem()->Capture().HP,float(Enemy->GetMonsterDefinition().CombatStats.MaxHP)));
                Test->TestTrue(TEXT("DT mesh initializes presentation"),Enemy->GetMesh()->GetSkeletalMeshAsset()==Enemy->GetMonsterDefinition().Mesh.LoadSynchronous());
                auto* AI=Cast<AICAIController>(Enemy->GetController());auto* BT=AI?Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()):nullptr;auto* BB=AI?AI->GetBlackboardComponent():nullptr;
                Test->TestTrue(TEXT("Every placed BP runs the existing team tree"),BT&&BT->IsRunning()&&BT->GetRootTree()==Enemy->GetMonsterDefinition().BehaviorTree.LoadSynchronous());
                Test->TestTrue(TEXT("Every placed BP uses the existing team blackboard"),BB&&BB->GetBlackboardAsset()==Enemy->GetMonsterDefinition().Blackboard.LoadSynchronous());
                Test->TestTrue(TEXT("Placed monsters generate reachable wander destinations"),BB&&BB->IsVectorValueSet(TEXT("WanderLocation")));
                if(FVector::Dist2D(Enemy->GetActorLocation(),Enemy->GetOrigin())>10)++Moved;
                if(Placement->Guid==Placements[0].Guid){Primary=Enemy;PrimaryKey=Enemy->GetSpawnKey();}
                else OtherKey=Enemy->GetSpawnKey();
            }
            Test->TestEqual(TEXT("All eleven BPs and two duplicate copies initialize"),Count,13);
            Test->TestEqual(TEXT("Copies of the same BP receive independent save keys"),Keys.Num(),13);
            Test->TestEqual(TEXT("Original DT test spawner still produces eleven monsters"),DTSpawns,11);
            Test->TestTrue(TEXT("Placed monsters actually move through the NavMesh"),Moved>0);
            int32 Points=0;for(TActorIterator<AICPlacedMonsterSpawnPoint> It(World);It;++It)++Points;
            Test->TestEqual(TEXT("One persistent runtime point per placed BP"),Points,13);
            if(!Test->TestTrue(TEXT("Primary placed enemy exists"),Primary.IsValid())){Cleanup();return true;}
            for(TActorIterator<AICEnemyBase> It(World);It;++It)if(*It!=Primary.Get())if(auto* AI=Cast<AICAIController>(It->GetController())){AI->StopExecution();It->GetCharacterMovement()->StopMovementImmediately();}
            auto* Enemy=Primary.Get();Home=Enemy->GetOrigin();Player->SetActorLocation(Home+FVector(500,0,20));BeforeHP=Player->GetICAbilitySystem()->Capture().HP;
            Stage=3;StageAt=Now;return false;
        }
        if(Stage==3)
        {
            auto* Enemy=Primary.Get();if(!Enemy){Test->AddError(TEXT("Placed enemy vanished during chase"));Cleanup();return true;}
            if(Player->GetICAbilitySystem()->Capture().HP>=BeforeHP||FVector::Dist2D(Enemy->GetActorLocation(),Home)<100)return false;
            Test->TestTrue(TEXT("Placed monster chases and damages the real player"),true);
            Enemy->GetICAbilitySystem()->CancelAction();Player->SetActorLocation(FVector(-4000,-12000,100));
            Enemy->SetActorLocation(Home+FVector(Enemy->PlacedLeashRadiusCm+150,0,0));
            Stage=4;StageAt=Now;return false;
        }
        if(Stage==4)
        {
            auto* Enemy=Primary.Get();if(!Enemy){Cleanup();return true;}
            bSawReturn|=Enemy->IsReturning();
            if(!bSawReturn||Enemy->IsReturning()||FVector::Dist2D(Enemy->GetActorLocation(),Home)>100)return false;
            Test->TestTrue(TEXT("Placed monster walks back to its authored origin after leaving its leash"),true);
            if(auto* AI=Cast<AICAIController>(Enemy->GetController()))AI->StopExecution();
            Enemy->GetICAbilitySystem()->CancelAction();const int32 BeforeGold=State->ExportState().Gold;
            Incarnation=State->ExportState().SpawnSlots.FindChecked(PrimaryKey).IncarnationID;
            Enemy->GetICAbilitySystem()->SetVitals(0,0,0);Enemy->HandleZeroHP();
            const auto Death=State->ExportState();
            Test->TestTrue(TEXT("Placed monster death grants its DT reward"),Death.Gold>BeforeGold);
            Test->TestTrue(TEXT("Placed monster reward has an idempotent receipt"),Death.CompletedTransactionIDs.Contains(Incarnation)&&!Death.SpawnSlots.FindChecked(PrimaryKey).bRewardPending);
            Enemy->HandleZeroHP();Test->TestEqual(TEXT("Duplicate death cannot duplicate rewards"),State->ExportState().Gold,Death.Gold);
            if(auto* Other=Cast<AICEnemyBase>(Spawns->FindCharacter(OtherKey)))Other->GetICAbilitySystem()->SetVitals(37,0,0);
            Test->TestTrue(TEXT("Live placed monsters flush into the existing save state"),State->FlushParticipants());
            auto* Save=NewObject<UICSaveGame>();Save->WorldState=State->ExportState();Save->Clock=Time->GetClock();
            TArray<uint8> Bytes;Test->TestTrue(TEXT("Existing save codec encodes placed spawns"),UICSaveGame::EncodeForFile(Save,Bytes));
            auto* Decoded=UICSaveGame::DecodeFile(Bytes);
            if(!Test->TestNotNull(TEXT("Existing save codec decodes placed spawns"),Decoded)){Cleanup();return true;}
            Test->TestTrue(TEXT("Placed spawn definitions pass load validation"),Spawns->CanRestoreActors(Decoded->WorldState));
            Test->TestTrue(TEXT("Load restores the placed monster world state"),State->RestoreState(Decoded->WorldState));Spawns->RefreshSpawns();
            auto* Other=Cast<AICEnemyBase>(Spawns->FindCharacter(OtherKey));Test->TestTrue(TEXT("Load preserves another placed enemy's HP"),Other&&Other->GetICAbilitySystem()->Capture().HP==37);
            const auto Slot=State->ExportState().SpawnSlots.FindChecked(PrimaryKey);
            Test->TestTrue(TEXT("A defeated placed enemy stays absent after load"),!Spawns->FindCharacter(PrimaryKey)&&Slot.bDefeated);
            Time->JumpGameMinutes(FMath::Max(0.,Slot.RespawnReadyAtGameMinute-Time->GetClock().GameMinute-.1));Spawns->RefreshSpawns();
            Test->TestNull(TEXT("Placed enemy cannot respawn before its DT deadline"),Spawns->FindCharacter(PrimaryKey));
            Time->JumpGameMinutes(.2);Spawns->RefreshSpawns();Stage=5;StageAt=Now;return false;
        }
        if(Stage==5)
        {
            if(Now-StageAt<.6)return false;
            auto* Enemy=Cast<AICEnemyBase>(Spawns->FindCharacter(PrimaryKey));
            if(Test->TestNotNull(TEXT("Placed enemy respawns after its DT cooldown"),Enemy))
            {
                Test->TestTrue(TEXT("Respawn uses the original placement"),Enemy->GetOrigin().Equals(Home,1));
                Test->TestTrue(TEXT("Respawn gets a fresh reward identity"),State->ExportState().SpawnSlots.FindChecked(PrimaryKey).IncarnationID!=Incarnation);
                auto* AI=Cast<AICAIController>(Enemy->GetController());auto* BT=AI?Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()):nullptr;
                Test->TestTrue(TEXT("Respawn restarts the existing team BT"),BT&&BT->IsRunning());
            }
            int32 Points=0;for(TActorIterator<AICPlacedMonsterSpawnPoint> It(World);It;++It)++Points;
            Test->TestEqual(TEXT("Respawning does not register extra placement points"),Points,13);
            Cleanup();return true;
        }
        return false;
    }
private:
    void Cleanup(){if(auto* Instance=GI.Get())if(auto* Saves=Instance->GetSubsystem<UICSaveSubsystem>())if(!Saves->bSaving)Saves->DeleteSlot(0);}
    FAutomationTestBase* Test;TArray<FPlacement> Placements;TWeakObjectPtr<UGameInstance> GI;TWeakObjectPtr<AICEnemyBase> Primary;
    FName PrimaryKey,OtherKey;FVector Home;FGuid Incarnation;float BeforeHP=0;bool bSawReturn=false;int32 Stage=0;double Started,StageAt=0;
};
class FCleanupEditorActors : public IAutomationLatentCommand
{
public:
    explicit FCleanupEditorActors(TArray<FPlacement> In):Placements(MoveTemp(In)){}
    bool Update() override{for(const auto& P:Placements)if(P.EditorActor.IsValid())P.EditorActor->Destroy();if(GEditor&&GEditor->GetEditorWorldContext().World())GEditor->GetEditorWorldContext().World()->GetOutermost()->SetDirtyFlag(false);return true;}
private:TArray<FPlacement> Placements;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPlacedMonsterCheck,"IC.AI.Monsters.PlacedBlueprints",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPlacedMonsterCheck::RunTest(const FString&)
{
    if(GEditor->PlayWorld){AddError(TEXT("Close PIE before placed monster verification"));return false;}
    if(!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/L_TestMap"),false,true))return false;
    auto* World=GEditor->GetEditorWorldContext().World();TArray<ICPlacedMonsterChecks::FPlacement> Placements;
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster.DT_IC_Monster"));
    if(!TestNotNull(TEXT("Actual monster DT"),Table))return false;
    for(int32 Index=0;Index<13;++Index)
    {
        const FName ID=Index==10?FName(TEXT("MON_R01_BOSS_001")):FName(*FString::Printf(TEXT("MON_R01_%03d"),Index<10?Index+1:1));
        const auto* Row=Table->FindRow<FICMonsterRow>(ID,TEXT("Placed monster test"));auto* Class=Row?Row->PawnClass.LoadSynchronous():nullptr;
        if(!TestNotNull(TEXT("Existing individual monster BP"),Class))continue;
        const auto* Default=CastChecked<AICEnemyBase>(Class->GetDefaultObject());
        TestEqual(TEXT("Each BP has a preset monster DT ID"),Default->MonsterID,ID);
        const FVector Origin(-2500,Index*800,Default->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2);
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Enemy=World->SpawnActor<AICEnemyBase>(Class,Origin,FRotator::ZeroRotator,Params);
        if(!TestNotNull(TEXT("Place BP into editor level without a spawner"),Enemy))continue;
        TestTrue(TEXT("Editor placement receives a persistent GUID"),Enemy->PlacementGuid.IsValid());
        Placements.Add({Enemy,Enemy->PlacementGuid,ID,Origin});
    }
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ICPlacedMonsterChecks::FPlacedCommand>(this,Placements));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ICPlacedMonsterChecks::FCleanupEditorActors>(Placements));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));return true;
}
#endif
