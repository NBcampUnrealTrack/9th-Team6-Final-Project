#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/OutputDeviceNull.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "NPC/Character/ICNPCBase.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Debug/ICDebugCheatManager.h"
#include "Codex/ICCodexSubsystem.h"
#include "Codex/ICPetFollowerSubsystem.h"
#include "Codex/ICPetFollower.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "AI/ICAIController.h"
#include "Time/ICTimeSubsystem.h"
#include "Components/BoxComponent.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"

namespace ICPartyFollowChecks
{
struct FSession
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);UWorld* World;AICPlayerController* PC;AICPlayerCharacter* Player;
    UICWorldStateSubsystem* State;UICDebugCheatManager* Cheats;TArray<AICNPCBase*> Companions;
    FSession()
    {
        GI->InitializeStandalone();World=GI->GetWorld();auto& Context=*GEngine->GetWorldContextFromWorld(World);
        auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);
        World->InitializeActorsForPlay(FURL(),false);State=GI->GetSubsystem<UICWorldStateSubsystem>();
        FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("Party follow");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
        FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;Initial.Characters.Add(P.CharacterId,P);State->RestoreState(Initial);
        PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
        Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);PC->bCreated=true;PC->Menu.Empty();
        Player->InitializeState(P.CharacterId);Player->Inventory->BindWorldState(State);
        Cheats=NewObject<UICDebugCheatManager>(PC);PC->CheatManager=Cheats;Cheats->ICRecruitTestParty();
        auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult Lookup;
        for(int32 Index=0;Index<3;++Index)
        {
            const FName ID(*FString::Printf(TEXT("NPC_R01_%03d"),Index+1));const auto* Row=D->Find<FICNPCRow>(ID,Lookup);
            FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            auto* NPC=World->SpawnActor<AICNPCBase>(Row->ActorClass.LoadSynchronous(),FVector(-350,Index*180,100),FRotator::ZeroRotator,Params);
            NPC->InitializeNPC(ID);NPC->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Companions.Add(NPC);
        }
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    ~FSession()
    {
        for(auto* NPC:Companions)if(IsValid(NPC)){NPC->ReleaseForLoad();NPC->Destroy();}
        Player->ReleaseForLoad();GI->Shutdown();World->DestroyWorld(false);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCompanionSprintCheck,"IC.PartyFollow.CompanionSprint",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICCompanionSprintCheck::RunTest(const FString&)
{
    ICPartyFollowChecks::FSession F;
    const float PlayerBase=F.Player->GetICAbilitySystem()->Stat(EICStat::MoveSpeed);
    F.Player->bSprintHeld=true;F.Player->GetCharacterMovement()->Velocity=FVector(PlayerBase*F.Player->SprintSpeedMultiplier,0,0);
    for(int32 Index=0;Index<F.Companions.Num();++Index)
    {
        auto* NPC=F.Companions[Index];auto* Movement=NPC->GetCharacterMovement();
        const float OwnBase=NPC->GetICAbilitySystem()->Stat(EICStat::MoveSpeed);
        NPC->UpdateMovementSpeed();const float OwnSprint=OwnBase*F.Player->SprintSpeedMultiplier;
        Movement->Velocity=FVector(OwnSprint,0,0);
        auto* Anim=Cast<UICCharacterAnimInstance>(NPC->GetMesh()->GetAnimInstance());
        if(!TestNotNull(TEXT("Companion uses native animation graph"),Anim))continue;
        Anim->NativeUpdateAnimation(.016f);
        TestTrue(TEXT("Moving companion runs when leader sprints"),Anim->bSprinting);
        TestTrue(TEXT("Sprint applies the common multiplier to this NPC's own derived speed"),FMath::IsNearlyEqual(Movement->MaxWalkSpeed,OwnSprint,.01f));
        TestTrue(TEXT("Equal starting stats give player-like sprint speed"),FMath::IsNearlyEqual(Movement->MaxWalkSpeed,PlayerBase*F.Player->SprintSpeedMultiplier,1.f));
        TestTrue(TEXT("Sprint is substantially faster than base movement"),Movement->MaxWalkSpeed>OwnBase*1.4f);
        TestNotNull(TEXT("Existing own run clip is available"),Anim->Sprint.Get());
        if(Anim->Sprint)TestEqual(TEXT("Run clip belongs to this NPC skeleton"),Anim->Sprint->GetSkeleton(),NPC->GetMesh()->GetSkeletalMeshAsset()->GetSkeleton());
        F.Player->bSprintHeld=false;NPC->UpdateMovementSpeed();Anim->NativeUpdateAnimation(.016f);TestFalse(TEXT("Releasing sprint returns companion to normal locomotion"),Anim->bSprinting);
        TestTrue(TEXT("Releasing sprint restores derived base speed"),FMath::IsNearlyEqual(Movement->MaxWalkSpeed,OwnBase,.01f));
        F.Player->bSprintHeld=true;Movement->Velocity=FVector::ZeroVector;Anim->NativeUpdateAnimation(.016f);TestFalse(TEXT("Arrived companion stays idle"),Anim->bSprinting);
        Movement->Velocity=FVector(OwnSprint,0,0);
    }
    auto* NPC=F.Companions[0];auto* Anim=Cast<UICCharacterAnimInstance>(NPC->GetMesh()->GetAnimInstance());
    const float BeforeDEX=NPC->GetICAbilitySystem()->Stat(EICStat::MoveSpeed);
    TestTrue(TEXT("NPC DEX can change independently"),F.State->SetMainStat(NPC->GetCharacterID(),EICMainStat::DEX,100));
    NPC->UpdateMovementSpeed();const float AfterDEX=NPC->GetICAbilitySystem()->Stat(EICStat::MoveSpeed);
    TestTrue(TEXT("DEX raises this NPC's derived speed"),AfterDEX>BeforeDEX);
    TestTrue(TEXT("Sprint preserves independent DEX differences"),FMath::IsNearlyEqual(NPC->GetCharacterMovement()->MaxWalkSpeed,AfterDEX*F.Player->SprintSpeedMultiplier,.01f));
    TestTrue(TEXT("Changing NPC DEX does not copy player speed"),NPC->GetCharacterMovement()->MaxWalkSpeed>PlayerBase*F.Player->SprintSpeedMultiplier);
    F.State->SetPartySlot(NPC->GetCharacterID(),INDEX_NONE);Anim->NativeUpdateAnimation(.016f);
    TestFalse(TEXT("Dismissed NPC no longer follows sprint state"),Anim->bSprinting);
    F.State->SetPartySlot(NPC->GetCharacterID(),0);NPC->GetCharacterMovement()->SetMovementMode(MOVE_Falling);Anim->NativeUpdateAnimation(.016f);
    TestFalse(TEXT("Airborne companion keeps airborne animation"),Anim->bSprinting);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPetMotionContinuityCheck,"IC.PartyFollow.PetMotionContinuity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPetMotionContinuityCheck::RunTest(const FString&)
{
    ICPartyFollowChecks::FSession F;F.Cheats->ICGiveAllPetEssence();F.Cheats->ICEquipTestPets();
    auto* Pet=F.World->GetSubsystem<UICPetFollowerSubsystem>()->GetFollower(UICWorldStateSubsystem::PlayerId());
    if(!TestNotNull(TEXT("Player has a pet for motion regression"),Pet))return false;
    for(const int32 FPS:{30,60,144})
    {
        const float Step=1.f/FPS;
        F.Player->GetCharacterMovement()->Velocity=FVector::ZeroVector;
        for(int32 I=0;I<60;++I)Pet->Tick(1.f/30);
        TestFalse(TEXT("Pet settles into idle"),Pet->bMoving);
        const int32 Before=Pet->AnimationSwitchCount;
        for(int32 Frame=0;Frame<FPS*5;++Frame)
        {
            // The owner changes from a walk to a sprint without stopping.
            const float Speed=Frame<FPS*2?450.f:675.f;
            F.Player->GetCharacterMovement()->Velocity={Speed,0,0};
            F.Player->SetActorLocation(F.Player->GetActorLocation()+FVector(Speed*Step,0,0));
            const auto Position=Pet->GetActorLocation();Pet->Tick(Step);
            if(!Pet->bMoving){AddError(FString::Printf(TEXT("Pet interrupted its move loop at %d FPS frame %d"),FPS,Frame));break;}
            if(FVector::Dist2D(Position,Pet->GetActorLocation())>(Speed+200)*Step+.1f)
            {AddError(TEXT("Normal pursuit made a discontinuous position jump"));break;}
        }
        TestTrue(TEXT("Walk-to-sprint pursuit never restarts the same move loop"),Pet->AnimationSwitchCount-Before<=1);
        TestTrue(TEXT("Pet stays behind a moving owner"),FVector::Dist2D(Pet->GetActorLocation(),F.Player->GetActorLocation())<350.f);
        F.Player->GetCharacterMovement()->Velocity=FVector::ZeroVector;
        for(int32 I=0;I<FPS*2;++I)Pet->Tick(Step);
        TestFalse(TEXT("Pet smoothly returns to idle after the owner stops"),Pet->bMoving);
        const int32 Settled=Pet->AnimationSwitchCount;
        for(int32 I=0;I<FPS;++I)Pet->Tick(Step);
        TestEqual(TEXT("Settled pet does not alternate between idle and move"),Pet->AnimationSwitchCount,Settled);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPetCheatFollowCheck,"IC.PartyFollow.PetCheatsAndFollowers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPetCheatFollowCheck::RunTest(const FString&)
{
    ICPartyFollowChecks::FSession F;auto* Codex=F.GI->GetSubsystem<UICCodexSubsystem>();
    auto* Followers=F.World->GetSubsystem<UICPetFollowerSubsystem>();FOutputDeviceNull Output;
    const auto Pets=Codex->PetDefinitions();if(!TestTrue(TEXT("Pet definitions exist"),Pets.Num()>=4))return false;
    Codex->AddPetEssence(Pets[0]->PetCodexIndex,150);
    TestTrue(TEXT("Essence cheat is exposed as an Exec command"),F.Cheats->ProcessConsoleExec(TEXT("ICGiveAllPetEssence"),Output,F.PC));
    for(const auto* Pet:Pets)TestTrue(TEXT("Every registered pet is unlocked"),Codex->IsPetUnlocked(Pet->PetCodexIndex));
    TestEqual(TEXT("Previously collected essence is preserved"),Codex->GetPetEssence(Pets[0]->PetCodexIndex),150);
    F.Cheats->ICGiveAllPetEssence();TestEqual(TEXT("Repeated cheat does not accumulate essence"),Codex->GetPetEssence(Pets[0]->PetCodexIndex),150);
    TestTrue(TEXT("Test equipment cheat is exposed as an Exec command"),F.Cheats->ProcessConsoleExec(TEXT("ICEquipTestPets"),Output,F.PC));
    Followers->RefreshFollowers();const auto Members=Codex->GetPartyMembers();TestEqual(TEXT("Player and three companions receive pets"),Members.Num(),4);
    TSet<int32> Equipped;
    for(FName ID:Members)
    {
        Equipped.Add(Codex->GetEquippedPet(ID));auto* Pet=Followers->GetFollower(ID);
        if(TestNotNull(TEXT("Equipped party member has a visible follower"),Pet))
        {
            TestEqual(TEXT("Follower identifies its real owner"),Pet->CharacterID,ID);
            TestNotNull(TEXT("Follower reuses an existing skeletal mesh"),Pet->Visual->GetSkeletalMeshAsset());
            TestFalse(TEXT("Cosmetic pet has no collision"),Pet->GetActorEnableCollision());
        }
    }
    TestEqual(TEXT("Test party receives different pet appearances"),Equipped.Num(),4);
    Followers->RefreshFollowers();int32 Count=0;for(TActorIterator<AICPetFollower> It(F.World);It;++It)if(!It->IsActorBeingDestroyed())++Count;
    TestEqual(TEXT("Refreshing does not duplicate followers"),Count,4);
    auto* Pet=Followers->GetFollower(UICWorldStateSubsystem::PlayerId());
    if(Pet)
    {
        F.Player->SetActorLocation({500,0,100});const float Before=FVector::Dist(Pet->GetActorLocation(),F.Player->GetActorLocation());Pet->Tick(.5f);
        TestTrue(TEXT("Pet moves after its owner"),FVector::Dist(Pet->GetActorLocation(),F.Player->GetActorLocation())<Before);
        F.Player->SetActorLocation({5000,0,100});Pet->Tick(.1f);
        TestTrue(TEXT("Pet recovers after owner teleport"),FVector::Dist(Pet->GetActorLocation(),F.Player->GetActorLocation())<250.f);
    }
    const FName Dismissed=F.Companions[0]->GetCharacterID();F.State->SetPartySlot(Dismissed,INDEX_NONE);Followers->RefreshFollowers();
    TestNull(TEXT("Dismissed companion's follower disappears"),Followers->GetFollower(Dismissed));
    F.State->SetPartySlot(Dismissed,0);Followers->RefreshFollowers();TestNotNull(TEXT("Rejoining restores the equipped follower"),Followers->GetFollower(Dismissed));
    TestTrue(TEXT("Pet state remains save-valid"),UICWorldStateSubsystem::ValidateState(F.State->ExportState()));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCompanionNavigationRecoveryCheck,"IC.PartyFollow.NavigationRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICCompanionNavigationRecoveryCheck::RunTest(const FString&)
{
    ICPartyFollowChecks::FSession F;
    // Standalone fixtures do not create the AI/navigation systems that maps supply.
    F.World->CreateAISystem();
    FNavigationSystem::AddNavigationSystemToWorld(*F.World,FNavigationSystemRunMode::GameMode);
    if(!TestNotNull(TEXT("Navigation system supports direct movement outside mesh bounds"),FNavigationSystem::GetCurrent<UNavigationSystemV1>(F.World)))return false;
    auto Box=[&](FVector Location,FVector Extent)
    {
        auto* Actor=F.World->SpawnActor<AActor>();auto* Shape=NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Shape);Actor->AddInstanceComponent(Shape);Shape->SetBoxExtent(Extent);
        Shape->SetCollisionProfileName(TEXT("BlockAll"));Shape->SetCollisionObjectType(ECC_WorldStatic);Shape->RegisterComponent();Actor->SetActorLocation(Location);
        return Shape;
    };
    Box({0,0,-40},{10000,10000,40});F.Player->SetActorLocation({0,0,96});
    auto* NPC=F.Companions[0];auto* AI=CastChecked<AICAIController>(NPC->GetController());
    auto* Time=F.GI->GetSubsystem<UICTimeSubsystem>();Time->SetRunning(true);Time->SetMenuPaused(false);
    auto Step=[&](int32 Count){for(int32 I=0;I<Count;++I){Time->AdvanceClock(.11,false);AI->Tick(.11f);}};

    // A companion recruited from an authored tree must resume native party following.
    auto* Tree=NewObject<UBehaviorTree>(AI);auto* Root=NewObject<UBTComposite_Sequence>(Tree);Tree->RootNode=Root;
    auto& Child=Root->Children.AddDefaulted_GetRef();Child.ChildTask=NewObject<UBTTask_Wait>(Root);
    AI->StartExecution(nullptr,Tree);
    TestTrue(TEXT("Authored behavior tree starts"),AI->GetBrainComponent()&&AI->GetBrainComponent()->IsRunning());
    AI->StartExecution(nullptr,nullptr);NPC->SetActorLocation({-1000,0,96});Step(1);
    TestEqual(TEXT("Leaving an authored tree resumes movement outside navigation coverage"),AI->GetMoveStatus(),EPathFollowingStatus::Moving);

    // With no physics ticks, a requested path cannot make progress: the watchdog must recover it.
    Step(28);
    TestTrue(TEXT("A stalled companion recovers before the long-distance threshold"),NPC->GetDistanceTo(F.Player)<350.f);
    TestTrue(TEXT("Safe recovery records the new save position"),F.State->GetStoredCharacter(NPC->GetCharacterID())->Position.Equals(NPC->GetActorLocation(),.1f));

    NPC->SetActorLocation({-1000,0,96});AI->StartExecution(nullptr,nullptr);
    auto* Blocker=Box({0,0,150},{600,600,300});Step(35);
    TestTrue(TEXT("Blocked ground never teleports a companion into a wall"),NPC->GetDistanceTo(F.Player)>800.f);
    Blocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);Step(35);
    TestTrue(TEXT("Recovery retries after safe ground becomes available"),NPC->GetDistanceTo(F.Player)<350.f);

    auto* Enemy=F.World->SpawnActor<AICEnemyBase>(FVector(100,0,96),FRotator::ZeroRotator);Enemy->EnableCombat(false,false);
    auto* EnemyASC=Enemy->GetICAbilitySystem();EnemyASC->SetCombatBase(10000,0,1,1,0,0,1,0,1);
    FICCharacterState Monster;Monster.CharacterId=TEXT("FOLLOW_TEST_ENEMY");Monster.Kind=EICCharacterKind::Background;Monster.MaxHP=Monster.HP=10000;
    TestTrue(TEXT("Combat target initializes"),EnemyASC->Restore(Monster));
    NPC->SetActorLocation({0,0,96});F.Player->SetActorLocation({3500,0,96});AI->StartExecution(nullptr,nullptr);AI->OrderAttack(Enemy);Step(1);
    TestTrue(TEXT("Distant combat target cannot bypass companion recovery"),NPC->GetDistanceTo(F.Player)<350.f);
    TestNull(TEXT("Regroup clears stale combat targets"),AI->GetAttackTarget());TestNull(TEXT("Regroup clears stale orders"),AI->GetQueuedAttackTarget());
    Enemy->ReleaseForLoad();Enemy->Destroy();

    NPC->SetActorLocation({0,0,96});AI->SetInteractionHold(true,F.Player);Step(30);
    TestTrue(TEXT("Conversation hold still prevents forced movement"),NPC->GetDistanceTo(F.Player)>3000.f);
    AI->SetInteractionHold(false,nullptr);F.State->SetPartySlot(NPC->GetCharacterID(),INDEX_NONE);Step(30);
    TestTrue(TEXT("Scheduled NPC does not use companion recovery"),NPC->GetDistanceTo(F.Player)>3000.f);
    return !HasAnyErrors();
}
#endif
