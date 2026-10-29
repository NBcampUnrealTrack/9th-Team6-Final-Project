#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "AI/ICAIController.h"
#include "AI/ICMonsterBTNodes.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Save/ICSaveSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "NavigationSystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ICMonsterBehaviorChecks
{
const TCHAR* TreePath=TEXT("/Game/IsekaiClass/Enemy/AI/BehaviorTrees/BT_ICMonsterBase.BT_ICMonsterBase");
const TCHAR* BlackboardPath=TEXT("/Game/IsekaiClass/Enemy/AI/Blackboards/BB_ICMonsterBase.BB_ICMonsterBase");

class FRuntimeCommand : public IAutomationLatentCommand
{
public:
    explicit FRuntimeCommand(FAutomationTestBase* InTest):Test(InTest),Started(FPlatformTime::Seconds()){}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if(Now-Started>75){Test->AddError(TEXT("Test-map monster BT verification timed out"));Cleanup();return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World?Cast<AICPlayerController>(World->GetFirstPlayerController()):nullptr;
        if(!PC)return false;
        if(Stage==0)
        {
            if(PC->Menu!=TEXT("Create")||!PC->GetSaves()->OnSaveFinished.Contains(PC,TEXT("SaveFinished")))return false;
            GI=PC->GetGameInstance();
            if(!Test->TestTrue(TEXT("Use isolated test save files"),PC->GetSaves()->ConfigureServices(PC->GetWorldState(),PC->GetTime(),TEXT("IC_MonsterBT_")+FGuid::NewGuid().ToString(EGuidFormats::Digits))))return true;
            if(!Test->TestTrue(TEXT("Create playable test character"),PC->DebugCreatePlayer())){Cleanup();return true;}
            Stage=1;return false;
        }
        if(Stage==1)
        {
            if(!PC->bCreated||PC->GetSaves()->bSaving)return false;
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
            if(Nav&&Nav->IsNavigationBuildInProgress())return false;
            World->GetSubsystem<UICSpawnSubsystem>()->RefreshSpawns();
            if(auto* Player=Cast<AICPlayerCharacter>(PC->GetPawn()))
            {Player->GetCharacterMovement()->DisableMovement();Player->SetActorLocation(FVector(-5000,-5000,100));}
            Stage=2;StageAt=Now;return false;
        }
        if(Now-StageAt<3)return false;
        auto* Tree=LoadObject<UBehaviorTree>(nullptr,TreePath);
        auto* SharedBB=LoadObject<UBlackboardData>(nullptr,BlackboardPath);
        TArray<AICEnemyBase*> Enemies;TSet<FName> IDs;FString Report;
        for(TActorIterator<AICEnemyBase> It(World);It;++It)
        {
            auto* Enemy=*It;const auto& Row=Enemy->GetMonsterDefinition();
            if(!Row.ID.ToString().StartsWith(TEXT("MON_R01_")))continue;
            Enemies.Add(Enemy);IDs.Add(Row.ID);
            auto* AI=Cast<AICAIController>(Enemy->GetController());
            auto* BT=AI?Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()):nullptr;
            auto* BB=AI?AI->GetBlackboardComponent():nullptr;
            Test->TestTrue(*(Row.ID.ToString()+TEXT(" uses the authored shared tree")),BT&&BT->IsRunning()&&BT->GetRootTree()==Tree);
            Test->TestTrue(*(Row.ID.ToString()+TEXT(" uses the authored shared blackboard")),BB&&BB->GetBlackboardAsset()==SharedBB);
            if(BB)
            {
                Test->TestTrue(TEXT("State service writes each monster's home"),BB->GetValueAsVector(TEXT("HomeLocation")).Equals(Enemy->GetOrigin(),1));
                Test->TestTrue(TEXT("State service writes each monster's identity"),BB->GetValueAsObject(TEXT("SelfActor"))==Enemy);
                Test->TestTrue(*(Row.ID.ToString()+TEXT(" has a reachable wander destination")),BB->IsVectorValueSet(TEXT("WanderLocation")));
            }
            Report+=FString::Printf(TEXT("%s | Running=%d | BT=%s | BB=%s | Task=%s\n"),*Row.ID.ToString(),BT&&BT->IsRunning(),*GetPathNameSafe(BT?BT->GetRootTree():nullptr),*GetPathNameSafe(BB?BB->GetBlackboardAsset():nullptr),*GetNameSafe(BT?BT->GetActiveNode():nullptr));
        }
        Test->TestEqual(TEXT("Test map spawns all eleven monsters"),Enemies.Num(),11);
        Test->TestEqual(TEXT("Every test monster has a unique definition"),IDs.Num(),11);
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("MonsterBehaviorReview");
        IFileManager::Get().MakeDirectory(*Folder,true);FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("Runtime.txt")));

        // Stop automatic decisions only inside this isolated PIE session so state
        // transitions can be tested deterministically against the real GAS actors.
        for(auto* Enemy:Enemies)if(auto* AI=Cast<AICAIController>(Enemy->GetController()))
        {AI->GetBrainComponent()->PauseLogic(TEXT("Automation state transitions"));AI->StopMovement();Enemy->GetCharacterMovement()->StopMovementImmediately();}
        auto* Player=Cast<AICPlayerCharacter>(PC->GetPawn());
        if(!Enemies.IsEmpty()&&Test->TestNotNull(TEXT("Real player pawn"),Player))CheckTransitions(Enemies[0],Player);
        Cleanup();return true;
    }
private:
    void CheckTransitions(AICEnemyBase* Enemy,AICPlayerCharacter* Player)
    {
        auto* AI=Cast<AICAIController>(Enemy->GetController());auto* BT=AI?Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()):nullptr;
        auto* BB=AI?AI->GetBlackboardComponent():nullptr;
        if(!Test->TestTrue(TEXT("Transition test has an active tree and blackboard"),BT&&BB))return;
        auto* Service=NewObject<UBTService_ICMonsterState>();
        TArray<uint8> Memory;Memory.SetNumZeroed(Service->GetSpecialMemorySize()+Service->GetInstanceMemorySize());
        auto Update=[&](){Service->TickNode(*BT,Memory.GetData()+Service->GetSpecialMemorySize(),.25f);};
        auto* ASC=Enemy->GetICAbilitySystem();ASC->CancelAction();
        const auto& Definition=Enemy->GetMonsterDefinition();const FVector Home=Enemy->GetOrigin();
        Enemy->FinishReturn();Enemy->SetActorLocation(Home);Enemy->SetActorRotation(FRotator::ZeroRotator);
        Player->GetCharacterMovement()->StopMovementImmediately();Player->SetActorEnableCollision(false);
        Player->SetActorLocation(Home+FVector(10000,0,0));
        BB->ClearValue(TEXT("TargetActor"));BB->SetValueAsBool(TEXT("IsInvestigating"),false);Update();
        Test->TestFalse(TEXT("No target allows wandering"),BB->GetValueAsBool(TEXT("IsReturning"))||BB->GetValueAsBool(TEXT("IsInvestigating"))||BB->GetValueAsBool(TEXT("IsInAttackRange")));

        Player->SetActorLocation(Home+FVector((Definition.AttackRangeCm+Definition.DetectionRangeCm)*.5,0,0));Update();
        Test->TestTrue(TEXT("Visible player becomes chase target"),BB->GetValueAsObject(TEXT("TargetActor"))==Player);
        Test->TestFalse(TEXT("Chase starts outside attack range"),BB->GetValueAsBool(TEXT("IsInAttackRange")));
        Test->TestTrue(TEXT("Chase records the investigation destination"),BB->IsVectorValueSet(TEXT("LastKnownTargetLocation")));

        Player->SetActorLocation(Home+FVector(Definition.AttackRangeCm*.5,0,0));Update();
        Test->TestTrue(TEXT("Nearby player selects attack"),BB->GetValueAsBool(TEXT("IsInAttackRange")));
        AI->SetMonsterActionReadyAt(0);auto* Attack=NewObject<UBTTask_ICMonsterAttack>();
        Test->TestTrue(TEXT("Authored attack task requests a real GAS action"),Attack->ExecuteTask(*BT,nullptr)==EBTNodeResult::Succeeded&&ASC->GetActionID().IsValid());ASC->CancelAction();

        Player->SetActorLocation(Home+FVector(10000,0,0));Update();
        Test->TestTrue(TEXT("Losing sight selects investigation"),BB->GetValueAsBool(TEXT("IsInvestigating")));
        Test->TestNull(TEXT("Investigation clears the visible target"),BB->GetValueAsObject(TEXT("TargetActor")));
        Test->TestFalse(TEXT("Losing sight does not immediately return home"),Enemy->IsReturning());
        Enemy->SetActorLocation(Home+FVector(Definition.ReturnAcceptanceRadiusCm+100,0,0));
        auto* FinishInvestigation=NewObject<UBTTask_ICMonsterFinishInvestigation>();
        Test->TestTrue(TEXT("Finishing investigation away from home starts return"),FinishInvestigation->ExecuteTask(*BT,nullptr)==EBTNodeResult::Succeeded&&Enemy->IsReturning()&&BB->GetValueAsBool(TEXT("IsReturning")));
        auto* FinishReturn=NewObject<UBTTask_ICMonsterFinishReturn>();
        Test->TestTrue(TEXT("Return cannot finish before reaching home"),FinishReturn->ExecuteTask(*BT,nullptr)==EBTNodeResult::Failed);
        Enemy->SetActorLocation(Home);Test->TestTrue(TEXT("Arriving home finishes return"),FinishReturn->ExecuteTask(*BT,nullptr)==EBTNodeResult::Succeeded&&!Enemy->IsReturning());

        Enemy->SetActorLocation(Home+FVector(Definition.ChaseLimitCm+100,0,0));Player->SetActorLocation(Enemy->GetActorLocation()+FVector(50,0,0));Update();
        Test->TestTrue(TEXT("Leaving the leash gives return priority over nearby attacks"),Enemy->IsReturning()&&BB->GetValueAsBool(TEXT("IsReturning")));
        Test->TestNull(TEXT("Returning monster clears its attack target"),BB->GetValueAsObject(TEXT("TargetActor")));
        Test->TestFalse(TEXT("Returning monster does not attack or investigate"),BB->GetValueAsBool(TEXT("IsInAttackRange"))||BB->GetValueAsBool(TEXT("IsInvestigating")));
    }
    void Cleanup(){if(auto* Instance=GI.Get())if(auto* Saves=Instance->GetSubsystem<UICSaveSubsystem>())if(!Saves->bSaving)Saves->DeleteSlot(0);}
    FAutomationTestBase* Test;TWeakObjectPtr<UGameInstance> GI;int32 Stage=0;double Started,StageAt=0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMonsterBehaviorAssetsCheck,"IC.AI.Monsters.SharedTreeBindings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMonsterBehaviorAssetsCheck::RunTest(const FString&)
{
    using namespace ICMonsterBehaviorChecks;
    auto* Tree=LoadObject<UBehaviorTree>(nullptr,TreePath);auto* BB=LoadObject<UBlackboardData>(nullptr,BlackboardPath);
    if(!TestNotNull(TEXT("Team behavior tree exists"),Tree)||!TestNotNull(TEXT("Team blackboard exists"),BB))return false;
    TestTrue(TEXT("Team tree uses its original blackboard"),Tree->BlackboardAsset==BB);
    auto* Root=Cast<UBTComposite_Selector>(Tree->RootNode);
    if(!TestNotNull(TEXT("Root is the authored priority selector"),Root))return false;
    if(!TestEqual(TEXT("Five authored behavior branches"),Root->Children.Num(),5))return false;
    TestTrue(TEXT("Root retains the team's state service"),Root->Services.ContainsByPredicate([](const auto& Service){return Cast<UBTService_ICMonsterState>(Service)!=nullptr;}));
    const FName Conditions[]={TEXT("IsReturning"),TEXT("IsInAttackRange"),TEXT("TargetActor"),TEXT("IsInvestigating")};
    for(int32 I=0;I<4;++I)
        TestTrue(TEXT("Preserve return, attack, chase, investigation priority"),Root->Children[I].Decorators.ContainsByPredicate([&](const auto& Node){auto* Decorator=Cast<UBTDecorator_BlackboardBase>(Node);return Decorator&&Decorator->GetSelectedBlackboardKey()==Conditions[I];}));
    const FName Keys[]={TEXT("SelfActor"),TEXT("TargetActor"),TEXT("HomeLocation"),TEXT("WanderLocation"),TEXT("LastKnownTargetLocation"),TEXT("IsReturning"),TEXT("IsInAttackRange"),TEXT("IsInvestigating")};
    for(FName Key:Keys)TestTrue(*Key.ToString(),BB->GetKeyID(Key)!=FBlackboard::InvalidKey);
    const int32 Branches[]={0,2,3,4};const FName MoveKeys[]={TEXT("HomeLocation"),TEXT("TargetActor"),TEXT("LastKnownTargetLocation"),TEXT("WanderLocation")};
    for(int32 I=0;I<4;++I)
    {
        auto* Branch=Root->Children[Branches[I]].ChildComposite.Get();
        TestTrue(TEXT("Authored movement uses the correct blackboard key"),Branch&&Branch->Children.ContainsByPredicate([&](const auto& Child){auto* Move=Cast<UBTTask_MoveTo>(Child.ChildTask);return Move&&Move->GetSelectedBlackboardKey()==MoveKeys[I];}));
    }
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Combat/DT_IC_Monster.DT_IC_Monster"));
    auto* Spawners=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/World/DT_IC_Spawner.DT_IC_Spawner"));
    const auto* Spawner=Spawners?Spawners->FindRow<FICSpawnerRow>(TEXT("SPN_R01_001"),TEXT("Monster bindings")):nullptr;
    if(!TestNotNull(TEXT("Monster DT exists"),Table)||!TestNotNull(TEXT("Test-map spawner exists"),Spawner))return false;
    TestEqual(TEXT("Test-map spawner has eleven slots"),Spawner->Slots.Num(),11);TSet<FName> IDs;
    for(const auto& Slot:Spawner->Slots)
    {
        IDs.Add(Slot.MonsterID);const auto* Row=Table->FindRow<FICMonsterRow>(Slot.MonsterID,TEXT("Monster bindings"));
        if(!TestNotNull(*Slot.MonsterID.ToString(),Row))continue;
        TestTrue(TEXT("Every test monster binds to the shared tree"),Row->BehaviorTree.LoadSynchronous()==Tree);
        TestTrue(TEXT("Every test monster binds to the shared blackboard"),Row->Blackboard.LoadSynchronous()==BB);
    }
    TestEqual(TEXT("Eleven distinct monster bindings"),IDs.Num(),11);return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMonsterBehaviorRuntimeCheck,"IC.AI.Monsters.TestMapRuntime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMonsterBehaviorRuntimeCheck::RunTest(const FString&)
{
    if(GEditor->PlayWorld){AddError(TEXT("Close existing PIE before monster BT verification"));return false;}
    if(!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/L_TestMap"),false,true))return false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ICMonsterBehaviorChecks::FRuntimeCommand>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));return true;
}
#endif
