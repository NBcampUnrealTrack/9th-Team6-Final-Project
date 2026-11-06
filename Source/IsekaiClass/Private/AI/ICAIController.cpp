#include "AI/ICAIController.h"
#include "StateTree.h"
#include "NPC/Character/ICNPCBase.h"
#include "NPC/Behavior/ICNPCHomeBehaviorComponent.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Components/StateTreeAIComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Debug/ICLog.h"
AICAIController::AICAIController()
{
    IC_TRACE_FUNCTION();PrimaryActorTick.bCanEverTick=true;ScheduleRunner=CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("ScheduleRunner"));ScheduleRunner->SetStartLogicAutomatically(false);bStartAILogicOnPossess=false;
}
UICNonCompanionNavigationFilter::UICNonCompanionNavigationFilter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AddExcludedArea(UICCompanionOnlyNavArea::StaticClass());
}
TSubclassOf<UNavigationQueryFilter> AICAIController::GetNonCompanionNavigationFilter() const
{
    return UICNonCompanionNavigationFilter::StaticClass();
}
bool AICAIController::StartExecution(UStateTree* Tree,UBehaviorTree* Behavior)
{
    IC_TRACE_FUNCTION();if(!GetPawn())return false;StopExecution();bExecutionReady=true;NextMonsterActionAt=0;
    if(Behavior){bUsesAuthoredTree=RunBehaviorTree(Behavior);if(!bUsesAuthoredTree)UE_LOG(LogICCore,Warning,TEXT("[AI.BehaviorFailed] %s"),*GetPathNameSafe(Behavior));}
    else if(Tree){ScheduleRunner->SetStateTree(Tree);ScheduleRunner->StartLogic();bUsesAuthoredTree=ScheduleRunner->IsRunning();if(!bUsesAuthoredTree)UE_LOG(LogICCore,Warning,TEXT("[AI.StateTreeFailed] %s"),*GetPathNameSafe(Tree));}
    NextDecisionAt=0;return true;
}
void AICAIController::StopExecution() { IC_TRACE_FUNCTION();bExecutionReady=false;bUsesAuthoredTree=false;ResetCompanionFollow();StopMovement();Target.Reset();OrderedTarget.Reset();ScheduleRunner->StopLogic(TEXT("상태 전환"));if(BrainComponent&&BrainComponent!=ScheduleRunner)BrainComponent->StopLogic(TEXT("상태 전환")); }
void AICAIController::OrderAttack(AICCharacterBase* Enemy)
{
    auto* NPC=Cast<AICNPCBase>(GetPawn());auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    if(!NPC||!State||State->GetSnapshot(NPC->GetCharacterID()).Mode!=EICCharacterMode::Companion||!IsValid(Enemy)||!NPC->IsHostileTo(Enemy)||!Enemy->GetICAbilitySystem()||!Enemy->GetICAbilitySystem()->IsAlive())return;
    OrderedTarget=Enemy;NextDecisionAt=0;
}
bool AICAIController::DecideCompanionCombat(AICNPCBase* NPC)
{
    auto* ASC=NPC?NPC->GetICAbilitySystem():nullptr;auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    if(!NPC||!ASC||!ASC->IsAlive()||State->GetSnapshot(NPC->GetCharacterID()).Mode!=EICCharacterMode::Companion)return false;
    auto Valid=[NPC](AICCharacterBase* Enemy){return IsValid(Enemy)&&NPC->IsHostileTo(Enemy)&&Enemy->GetICAbilitySystem()&&Enemy->GetICAbilitySystem()->IsAlive();};
    if(!Valid(Target.Get()))Target.Reset();if(!Valid(OrderedTarget.Get()))OrderedTarget.Reset();
    if(!Target.IsValid()&&OrderedTarget.IsValid()){Target=OrderedTarget;OrderedTarget.Reset();}
    if(!Target.IsValid())
    {
        FICDefinitionResult Lookup;
        const auto* Rules=ASC->Definitions()->Find<FICPartySettingsRow>(TEXT("CFG_PARTY_001"),Lookup);
        double Nearest=FMath::Square(Rules?Rules->SearchDistanceCm:800.);
        for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(Valid(*It)&&LineOfSightTo(*It)&&(ASC->HasRecentCombat()||It->GetICAbilitySystem()->HasRecentCombat()))
        {const double Distance=FVector::DistSquared(NPC->GetActorLocation(),It->GetActorLocation());if(Distance<Nearest){Nearest=Distance;Target=*It;}}
    }
    if(!Target.IsValid()){ClearFocus(EAIFocusPriority::Gameplay);return false;}
    if(auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);!Player||FVector::DistSquared(Player->GetActorLocation(),Target->GetActorLocation())>FMath::Square(4500.f))
    {Target.Reset();return false;}
    SetFocus(Target.Get());const float Distance=FVector::Distance(NPC->GetActorLocation(),Target->GetActorLocation());
    const float Range=FMath::Max(120.f,ASC->GetNormalAttackRangeCm());
    if(Distance>Range*.9f||!LineOfSightTo(Target.Get())){NPC->GetCharacterMovement()->bOrientRotationToMovement=true;MoveToActor(Target.Get(),FMath::Max(50.f,Range*.7f),false);return true;}
    StopMovement();NPC->GetCharacterMovement()->StopMovementImmediately();NPC->GetCharacterMovement()->bOrientRotationToMovement=false;
    const FRotator Facing=(Target->GetActorLocation()-NPC->GetActorLocation()).GetSafeNormal2D().Rotation();SetControlRotation(Facing);NPC->SetActorRotation(Facing);
    const auto Snapshot=State->GetSnapshot(NPC->GetCharacterID());FICDefinitionResult R;
    const auto* Base=ASC->Definitions()->Find<FICRecruitCombatRow>(NPC->GetNPCDefinition().RecruitCombatID,R);
    if(Base&&Snapshot.bAutoSkillsEnabled){const auto Result=ASC->RequestAction(TEXT("Skill"),Base->UniqueSkillID,Target.Get());if(Result.bSuccess)return true;}
    ASC->RequestAction(TEXT("Attack"),NAME_None,Target.Get());return true;
}
void AICAIController::SetInteractionHold(bool bHold,AActor* FaceTarget)
{
    IC_TRACE_FUNCTION();bInteractionHold=bHold;
    if(bHold){StopMovement();if(FaceTarget)SetFocus(FaceTarget);}
    else ClearFocus(EAIFocusPriority::Gameplay);
}
void AICAIController::RestoreScheduleFacing()
{
    IC_TRACE_FUNCTION();
    auto* NPC=Cast<AICNPCBase>(GetPawn());
    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    auto* T=GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;
    auto* Spawns=GetWorld()?GetWorld()->GetSubsystem<UICSpawnSubsystem>():nullptr;
    if(!NPC||!D||!T||!Spawns)return;
    auto* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    if (!State || State->GetSnapshot(NPC->GetCharacterID()).Mode != EICCharacterMode::Scheduled) return;

    FICDefinitionResult R;
    const auto* Schedule=D->Find<FICScheduleRow>(NPC->GetNPCDefinition().ScheduleID,R);
    if(!Schedule)return;
    const int32 Minute=int32(FMath::FloorToDouble(T->GetClock().GameMinute))%1440;
    for(const auto& Segment:Schedule->Segments)
    {
        if(Minute<Segment.StartGameMinute||Minute>=Segment.EndGameMinute)continue;
        FTransform Anchor;
        if(!Spawns->ResolveLoadedAnchor(Segment.DestinationAnchorID,Anchor))return;
        NPC->GetCharacterMovement()->StopMovementImmediately();
        NPC->GetCharacterMovement()->bOrientRotationToMovement=false;
        SetControlRotation(Anchor.Rotator());
        NPC->SetActorRotation(Anchor.Rotator());
        return;
    }
}
bool AICAIController::RequestHomeBehaviorMove(const FVector& Goal, float AcceptanceRadiusCm)
{
    if (!GetPawn() || bInteractionHold) return false;
    if (AICCharacterBase* ControlledCharacter = Cast<AICCharacterBase>(GetPawn()))
        ControlledCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
    return MoveToLocation(Goal, FMath::Max(10.f, AcceptanceRadiusCm), true, true, true, false, GetNonCompanionNavigationFilter()) != EPathFollowingRequestResult::Failed;
}
void AICAIController::StopHomeBehaviorMove()
{
    StopMovement();
    if (AICCharacterBase* ControlledCharacter = Cast<AICCharacterBase>(GetPawn())) ControlledCharacter->GetCharacterMovement()->StopMovementImmediately();
}
void AICAIController::FaceHomeBehavior(const FRotator& Facing)
{
    if (AICCharacterBase* ControlledCharacter = Cast<AICCharacterBase>(GetPawn()))
    {
        ControlledCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
        SetControlRotation(Facing);
        ControlledCharacter->SetActorRotation(Facing);
    }
}
void AICAIController::OnUnPossess() { IC_TRACE_FUNCTION();StopExecution();Super::OnUnPossess(); }
bool AICAIController::TryRecoverCompanion(bool bPathFailure)
{
    auto* NPC = Cast<AICNPCBase>(GetPawn());
    auto* GI = GetGameInstance();
    auto* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    auto* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    auto* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    const auto* Member = State && NPC ? State->GetStoredCharacter(NPC->GetCharacterID()) : nullptr;
    if (!Member || !Player || bInteractionHold || Member->Kind != EICCharacterKind::Recruitable
        || Member->Mode != EICCharacterMode::Companion || Member->PartySlot < 0 || Member->IsDowned()) return false;
    if (auto* ASC = NPC->GetICAbilitySystem(); ASC && !ASC->IsAlive()) return false;
    FICDefinitionResult Result;
    const auto* Rules = Definitions ? Definitions->Find<FICPartySettingsRow>(TEXT("CFG_PARTY_001"), Result) : nullptr;
    const double Distance = Rules ? Rules->RecoveryDistanceCm : 2000.;
    const double Separation=FVector::DistSquared(NPC->GetActorLocation(),Player->GetActorLocation());
    if (Distance<=0 || Separation<=FMath::Square(bPathFailure?350.:Distance))return false;
    auto* Spawns = GetWorld()->GetSubsystem<UICSpawnSubsystem>();
    FTransform Placement;
    if (!Spawns || !Spawns->FindSafePartyPlacement(NPC->GetCharacterID(), Placement)) return false;
    if(auto* ASC=NPC->GetICAbilitySystem())ASC->CancelAction();
    Target.Reset();OrderedTarget.Reset();ClearFocus(EAIFocusPriority::Gameplay);
    StopMovement();
    NPC->GetCharacterMovement()->StopMovementImmediately();
    if (!NPC->TeleportTo(Placement.GetLocation(), Placement.Rotator())) return false;
    ResetCompanionFollow();bRegrouping=true;
    IICStateParticipant::Execute_FlushToWorldState(NPC, State);
    UE_LOG(LogICCore, Log, TEXT("[Party.Recovered] NPC=%s PathFailure=%d Position=%s"), *NPC->GetCharacterID().ToString(),bPathFailure,*Placement.GetLocation().ToString());
    return true;
}

void AICAIController::Tick(float DeltaSeconds)
{
    IC_TRACE_FUNCTION();Super::Tick(DeltaSeconds);if(!bExecutionReady)return;auto* T=GetGameInstance()?GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;
    if(!T||!T->IsRunning())return;
    if(!bUsesAuthoredTree&&T->GetClock().PlaySecond>=NextDecisionAt)
    {
        auto* NPC=Cast<AICNPCBase>(GetPawn());auto* State=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
        const auto* Member=State&&NPC?State->GetStoredCharacter(NPC->GetCharacterID()):nullptr;
        NextDecisionAt=T->GetClock().PlaySecond+(Member&&Member->Mode==EICCharacterMode::Companion?.1:.25);Decide();
    }
    if(T->IsRunning())
        if(auto* Enemy=Cast<AICEnemyBase>(GetPawn());Enemy&&!Enemy->IsReturning()&&Enemy->GetICAbilitySystem()&&Enemy->GetICAbilitySystem()->IsAlive()&&Target.IsValid()&&Enemy->IsHostileTo(Target.Get()))
            if(auto* TargetASC=Target->GetICAbilitySystem();TargetASC&&TargetASC->IsAlive())
            {Enemy->GetICAbilitySystem()->MarkCombat();TargetASC->MarkCombat();}
}
void AICAIController::Decide()
{
    IC_TRACE_FUNCTION();auto* C=Cast<AICCharacterBase>(GetPawn());if(!C)return;auto* ASC=C->GetICAbilitySystem();auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();auto* S=GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();auto* T=GetGameInstance()->GetSubsystem<UICTimeSubsystem>();FICDefinitionResult R;
    if(auto* NPC=Cast<AICNPCBase>(C))
    {
        if(bInteractionHold){StopMovement();return;}
        const auto State=S->GetSnapshot(C->GetCharacterID());if(State.IsDowned()){StopExecution();return;}
        if(State.Mode==EICCharacterMode::Companion){DecideCompanionFollow(NPC);return;}
        if (UICNPCHomeBehaviorComponent* HomeBehavior = NPC->GetHomeBehavior(); HomeBehavior && HomeBehavior->TickBehavior(*this)) return;
        constexpr float ArrivalDistanceCm=150.f;
        FVector Goal;FRotator GoalRotation=FRotator::ZeroRotator;bool HasGoal=false;
        {
            const auto* Schedule=D->Find<FICScheduleRow>(NPC->GetNPCDefinition().ScheduleID,R);const int32 Minute=int32(FMath::FloorToDouble(T->GetClock().GameMinute))%1440;
            if(Schedule)for(const auto& Segment:Schedule->Segments)if(Minute>=Segment.StartGameMinute&&Minute<Segment.EndGameMinute)
            {FTransform Anchor;HasGoal=GetWorld()->GetSubsystem<UICSpawnSubsystem>()->ResolveLoadedAnchor(Segment.DestinationAnchorID,Anchor);Goal=Anchor.GetLocation();GoalRotation=Anchor.Rotator();if(HasGoal&&FVector::DistSquared(C->GetActorLocation(),Goal)<=FMath::Square(ArrivalDistanceCm)){auto Updated=State;if(Updated.CurrentAnchorID!=Segment.DestinationAnchorID){Updated.CurrentAnchorID=Segment.DestinationAnchorID;S->SetCharacterState(Updated);}}break;}
        }
        if(HasGoal&&FVector::DistSquared(C->GetActorLocation(),Goal)>FMath::Square(ArrivalDistanceCm))
        {

            C->GetCharacterMovement()->bOrientRotationToMovement=true;
            MoveToLocation(Goal, 100.f, true, true, true, false, GetNonCompanionNavigationFilter());
        }
        else
        {
            StopMovement();
            if(HasGoal)
            {
                C->GetCharacterMovement()->StopMovementImmediately();
                C->GetCharacterMovement()->bOrientRotationToMovement=false;
                SetControlRotation(GoalRotation);
                C->SetActorRotation(GoalRotation);
            }
        }
        return;
    }
    auto* Enemy=Cast<AICEnemyBase>(C);if(!Enemy||!ASC||!ASC->IsAlive())return;const auto& M=Enemy->GetMonsterDefinition();
    if(Enemy->OutsideLeash())Enemy->BeginReturn();
    if(Enemy->IsReturning())
    {Target.Reset();ClearFocus(EAIFocusPriority::Gameplay);if(FVector::DistSquared(C->GetActorLocation(),Enemy->GetOrigin())<=FMath::Square(FMath::Max(50.,M.ReturnAcceptanceRadiusCm))){StopMovement();Enemy->FinishReturn();}else MoveToLocation(Enemy->GetOrigin(),FMath::Max(20.,M.ReturnAcceptanceRadiusCm*.6),false,true,true,false,GetNonCompanionNavigationFilter());return;}
    if(!Target.IsValid()||!Target->GetICAbilitySystem()||!Target->GetICAbilitySystem()->IsAlive())
    {
        Target.Reset();double Distance=FMath::Square(M.DetectionRangeCm);
        for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(C->IsHostileTo(*It)&&It->GetICAbilitySystem()&&It->GetICAbilitySystem()->IsAlive()&&It->GetICAbilitySystem()->Capture().Kind==EICCharacterKind::Player)
        {const auto Delta=It->GetActorLocation()-C->GetActorLocation();if(Delta.SizeSquared()<Distance&&(!M.bRequiresLineOfSight||LineOfSightTo(*It))&&(M.SightAngleDegrees>=360||FVector::DotProduct(C->GetActorForwardVector(),Delta.GetSafeNormal())>=FMath::Cos(FMath::DegreesToRadians(M.SightAngleDegrees*.5)))){Distance=Delta.SizeSquared();Target=*It;}}
    }
    if(!Target.IsValid())
    {
        StopMovement();ClearFocus(EAIFocusPriority::Gameplay);
        if(FVector::DistSquared(C->GetActorLocation(),Enemy->GetOrigin())>FMath::Square(FMath::Max(50.,M.ReturnAcceptanceRadiusCm)))Enemy->BeginReturn();
        return;
    }
    SetFocus(Target.Get());const double Distance=FVector::Distance(C->GetActorLocation(),Target->GetActorLocation());
    if(Distance>M.DetectionRangeCm||(M.bRequiresLineOfSight&&!LineOfSightTo(Target.Get()))){StopMovement();Enemy->BeginReturn();return;}
    if(Distance>Enemy->GetAttackRangeCm()){MoveToActor(Target.Get(),FMath::Max(20.,Enemy->GetAttackRangeCm()*.7),false,true,false,GetNonCompanionNavigationFilter());return;}StopMovement();
    C->GetCharacterMovement()->StopMovementImmediately();
    const FVector Facing=(Target->GetActorLocation()-C->GetActorLocation()).GetSafeNormal2D();
    if(!Facing.IsNearlyZero())
    {
        const FRotator Rotation(0,Facing.Rotation().Yaw,0);
        SetControlRotation(Rotation);C->SetActorRotation(Rotation);
    }
    TArray<const FICMonsterSkillChoice*> Choices;double Weight=0;for(const auto& Choice:M.Skills)if(Choice.Weight>0&&ASC->Conditions(Choice.Conditions,Target->GetICAbilitySystem())){const auto* Skill=D->Find<FICSkillRow>(Choice.SkillID,R);if(Skill&&ASC->Capture().MP>=Skill->MPCost&&T->GetClock().PlaySecond>=ASC->Capture().Combat.SkillReadyAtPlaySecond.FindRef(Choice.SkillID)){Choices.Add(&Choice);Weight+=Choice.Weight;}}
    if(Weight>0){double Roll=FMath::FRand()*Weight;for(const auto* Choice:Choices){Roll-=Choice->Weight;if(Roll<=0){ASC->RequestAction(TEXT("Skill"),Choice->SkillID,Target.Get());return;}}}
    ASC->RequestAction(TEXT("Attack"),NAME_None,Target.Get());NextDecisionAt=T->GetClock().PlaySecond+FMath::Max(.25,M.AttackIntervalPlaySeconds);
}

