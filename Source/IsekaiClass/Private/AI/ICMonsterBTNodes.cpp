#include "AI/ICMonsterBTNodes.h"

#include "AI/ICAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Base/ICCharacterBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

namespace
{
    const FName SelfActorKey(TEXT("SelfActor"));
    const FName TargetActorKey(TEXT("TargetActor"));
    const FName HomeLocationKey(TEXT("HomeLocation"));
    const FName IsReturningKey(TEXT("IsReturning"));
    const FName IsInAttackRangeKey(TEXT("IsInAttackRange"));

    bool IsVisibleHostileTarget(const AICEnemyBase* Enemy, AICCharacterBase* Candidate,
        AICAIController* Controller, const FICMonsterRow& Definition)
    {
        if (!Enemy || !Candidate || Candidate == Enemy || !Enemy->IsHostileTo(Candidate)) return false;
        const UICAbilitySystemComponent* TargetASC = Candidate->GetICAbilitySystem();
        if (!TargetASC || !TargetASC->IsAlive()) return false;

        const FVector Delta = Candidate->GetActorLocation() - Enemy->GetActorLocation();
        if (Delta.SizeSquared() > FMath::Square(Definition.DetectionRangeCm)) return false;
        if (Definition.bRequiresLineOfSight && (!Controller || !Controller->LineOfSightTo(Candidate))) return false;
        if (Definition.SightAngleDegrees < 360.0)
        {
            const double MinDot = FMath::Cos(FMath::DegreesToRadians(Definition.SightAngleDegrees * 0.5));
            if (FVector::DotProduct(Enemy->GetActorForwardVector(), Delta.GetSafeNormal()) < MinDot) return false;
        }
        return true;
    }

    void WriteObject(UBlackboardComponent* Blackboard, const FName Key, UObject* Value)
    {
        if (Blackboard && Blackboard->GetKeyID(Key) != FBlackboard::InvalidKey)
            Blackboard->SetValueAsObject(Key, Value);
    }

    void WriteVector(UBlackboardComponent* Blackboard, const FName Key, const FVector& Value)
    {
        if (Blackboard && Blackboard->GetKeyID(Key) != FBlackboard::InvalidKey)
            Blackboard->SetValueAsVector(Key, Value);
    }

    void WriteBool(UBlackboardComponent* Blackboard, const FName Key, const bool Value)
    {
        if (Blackboard && Blackboard->GetKeyID(Key) != FBlackboard::InvalidKey)
            Blackboard->SetValueAsBool(Key, Value);
    }
}

UBTTask_ICMonsterPickWanderLocation::UBTTask_ICMonsterPickWanderLocation()
{
    NodeName = TEXT("Pick Wander Location");
    HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ICMonsterPickWanderLocation, HomeLocationKey));
    WanderLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ICMonsterPickWanderLocation, WanderLocationKey));
}

EBTNodeResult::Type UBTTask_ICMonsterPickWanderLocation::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    UWorld* World = OwnerComp.GetWorld();
    if (!Blackboard || !World || !HomeLocationKey.IsSet() || !WanderLocationKey.IsSet()
        || Blackboard->GetKeyID(HomeLocationKey.SelectedKeyName) == FBlackboard::InvalidKey
        || Blackboard->GetKeyID(WanderLocationKey.SelectedKeyName) == FBlackboard::InvalidKey)
        return EBTNodeResult::Failed;

    const FVector Center = Blackboard->GetValueAsVector(HomeLocationKey.SelectedKeyName);
    const float Radius = FMath::Max(0.0f, WanderRadius);
    if (Radius <= 0.0f) return EBTNodeResult::Failed;

    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    FNavLocation Candidate;
    if (!Navigation || !Navigation->GetRandomReachablePointInRadius(Center, Radius, Candidate))
        return EBTNodeResult::Failed;

    Blackboard->SetValueAsVector(WanderLocationKey.SelectedKeyName, Candidate.Location);
    return EBTNodeResult::Succeeded;
}

UBTService_ICMonsterState::UBTService_ICMonsterState()
{
    NodeName = TEXT("Update Monster State");
    Interval = 0.25f;
    RandomDeviation = 0.0f;
}

void UBTService_ICMonsterState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    auto* Controller = Cast<AICAIController>(OwnerComp.GetAIOwner());
    auto* Enemy = Controller ? Cast<AICEnemyBase>(Controller->GetPawn()) : nullptr;
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Enemy || !Blackboard || !Enemy->GetICAbilitySystem() || !Enemy->GetICAbilitySystem()->IsAlive()) return;

    const FICMonsterRow& Definition = Enemy->GetMonsterDefinition();
    WriteObject(Blackboard, SelfActorKey, Enemy);
    WriteVector(Blackboard, HomeLocationKey, Enemy->GetOrigin());

    AICCharacterBase* Target = nullptr;
    if (!Enemy->IsReturning())
    {
        auto* PreviousTarget = Cast<AICCharacterBase>(Blackboard->GetValueAsObject(TargetActorKey));
        if (IsVisibleHostileTarget(Enemy, PreviousTarget, Controller, Definition))
        {
            Target = PreviousTarget;
        }
        else
        {
            double BestDistanceSquared = FMath::Square(Definition.DetectionRangeCm);
            for (TActorIterator<AICCharacterBase> It(Enemy->GetWorld()); It; ++It)
            {
                AICCharacterBase* Candidate = *It;
                if (!IsVisibleHostileTarget(Enemy, Candidate, Controller, Definition)) continue;
                const double DistanceSquared = FVector::DistSquared(Enemy->GetActorLocation(), Candidate->GetActorLocation());
                if (DistanceSquared < BestDistanceSquared)
                {
                    BestDistanceSquared = DistanceSquared;
                    Target = Candidate;
                }
            }
        }

        const double HomeDistanceSquared = FVector::DistSquared(Enemy->GetActorLocation(), Enemy->GetOrigin());
        const double ReturnRadius = FMath::Max(50.0, Definition.ReturnAcceptanceRadiusCm);
        if (Enemy->OutsideLeash() || (!Target && HomeDistanceSquared > FMath::Square(ReturnRadius)))
        {
            Enemy->BeginReturn();
            Target = nullptr;
        }
    }

    const bool bReturning = Enemy->IsReturning();
    const bool bInAttackRange = !bReturning && Target
        && FVector::DistSquared(Enemy->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(Definition.AttackRangeCm);

    WriteObject(Blackboard, TargetActorKey, bReturning ? nullptr : Target);
    WriteBool(Blackboard, IsReturningKey, bReturning);
    WriteBool(Blackboard, IsInAttackRangeKey, bInAttackRange);
    Controller->SetMonsterBehaviorTarget(bReturning ? nullptr : Target);
}

UBTTask_ICMonsterBeginReturn::UBTTask_ICMonsterBeginReturn()
{
    NodeName = TEXT("Begin Return");
}

EBTNodeResult::Type UBTTask_ICMonsterBeginReturn::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    auto* Controller = Cast<AICAIController>(OwnerComp.GetAIOwner());
    auto* Enemy = Controller ? Cast<AICEnemyBase>(Controller->GetPawn()) : nullptr;
    if (!Enemy || !Enemy->GetICAbilitySystem() || !Enemy->GetICAbilitySystem()->IsAlive()) return EBTNodeResult::Failed;
    Enemy->BeginReturn();
    return EBTNodeResult::Succeeded;
}

UBTTask_ICMonsterFinishReturn::UBTTask_ICMonsterFinishReturn()
{
    NodeName = TEXT("Finish Return");
}

EBTNodeResult::Type UBTTask_ICMonsterFinishReturn::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    auto* Controller = Cast<AICAIController>(OwnerComp.GetAIOwner());
    auto* Enemy = Controller ? Cast<AICEnemyBase>(Controller->GetPawn()) : nullptr;
    if (!Enemy || !Enemy->IsReturning()) return EBTNodeResult::Failed;

    const double AcceptanceRadius = FMath::Max(50.0, Enemy->GetMonsterDefinition().ReturnAcceptanceRadiusCm);
    if (FVector::DistSquared(Enemy->GetActorLocation(), Enemy->GetOrigin()) > FMath::Square(AcceptanceRadius))
        return EBTNodeResult::Failed;

    Enemy->FinishReturn();
    if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
    {
        WriteBool(Blackboard, IsReturningKey, false);
        WriteObject(Blackboard, TargetActorKey, nullptr);
    }
    Controller->SetMonsterBehaviorTarget(nullptr);
    return EBTNodeResult::Succeeded;
}

UBTTask_ICMonsterAttack::UBTTask_ICMonsterAttack()
{
    NodeName = TEXT("Request Attack or Skill");
    bNotifyTick = true;
    bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ICMonsterAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    auto* Controller = Cast<AICAIController>(OwnerComp.GetAIOwner());
    auto* Enemy = Controller ? Cast<AICEnemyBase>(Controller->GetPawn()) : nullptr;
    UICAbilitySystemComponent* ASC = Enemy ? Enemy->GetICAbilitySystem() : nullptr;
    if (!Enemy || !ASC || !ASC->IsAlive()) return EBTNodeResult::Failed;

    return ASC->PlaySecond() < Controller->GetMonsterActionReadyAt()
        ? EBTNodeResult::InProgress
        : PerformAttack(OwnerComp);
}

void UBTTask_ICMonsterAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    auto* Controller = Cast<AICAIController>(OwnerComp.GetAIOwner());
    auto* Enemy = Controller ? Cast<AICEnemyBase>(Controller->GetPawn()) : nullptr;
    UICAbilitySystemComponent* ASC = Enemy ? Enemy->GetICAbilitySystem() : nullptr;
    if (!Enemy || !ASC || !ASC->IsAlive())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    if (ASC->PlaySecond() >= Controller->GetMonsterActionReadyAt())
        FinishLatentTask(OwnerComp, PerformAttack(OwnerComp));
}

EBTNodeResult::Type UBTTask_ICMonsterAttack::PerformAttack(UBehaviorTreeComponent& OwnerComp) const
{
    auto* Controller = Cast<AICAIController>(OwnerComp.GetAIOwner());
    auto* Enemy = Controller ? Cast<AICEnemyBase>(Controller->GetPawn()) : nullptr;
    auto* Blackboard = OwnerComp.GetBlackboardComponent();
    auto* Target = Blackboard ? Cast<AICCharacterBase>(Blackboard->GetValueAsObject(TargetActorKey)) : nullptr;
    UICAbilitySystemComponent* ASC = Enemy ? Enemy->GetICAbilitySystem() : nullptr;
    if (!Enemy || !Target || !ASC || Enemy->IsReturning() || !Enemy->IsHostileTo(Target)
        || !Target->GetICAbilitySystem() || !Target->GetICAbilitySystem()->IsAlive()) return EBTNodeResult::Failed;

    const FICMonsterRow& Definition = Enemy->GetMonsterDefinition();
    const double DistanceSquared = FVector::DistSquared(Enemy->GetActorLocation(), Target->GetActorLocation());
    if (DistanceSquared > FMath::Square(Definition.AttackRangeCm)) return EBTNodeResult::Failed;

    Controller->StopMovement();
    Enemy->GetCharacterMovement()->StopMovementImmediately();
    const FVector Facing = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
    if (!Facing.IsNearlyZero())
    {
        const FRotator Rotation(0.0, Facing.Rotation().Yaw, 0.0);
        Controller->SetControlRotation(Rotation);
        Enemy->SetActorRotation(Rotation);
    }

    UICDefinitionSubsystem* Definitions = ASC->Definitions();
    FICDefinitionResult Result;
    TArray<const FICMonsterSkillChoice*> Choices;
    double TotalWeight = 0.0;
    for (const FICMonsterSkillChoice& Choice : Definition.Skills)
    {
        if (Choice.Weight <= 0.0 || !ASC->Conditions(Choice.Conditions, Target->GetICAbilitySystem())) continue;
        const FICSkillRow* Skill = Definitions ? Definitions->Find<FICSkillRow>(Choice.SkillID, Result) : nullptr;
        if (!Skill) continue;
        const FICCharacterState Snapshot = ASC->Capture();
        if (Snapshot.MP < Skill->MPCost || ASC->PlaySecond() < Snapshot.Combat.SkillReadyAtPlaySecond.FindRef(Choice.SkillID)) continue;
        Choices.Add(&Choice);
        TotalWeight += Choice.Weight;
    }

    FICCombatResult ActionResult;
    if (TotalWeight > 0.0)
    {
        double Roll = FMath::FRand() * TotalWeight;
        for (const FICMonsterSkillChoice* Choice : Choices)
        {
            Roll -= Choice->Weight;
            if (Roll <= 0.0)
            {
                ActionResult = ASC->RequestAction(TEXT("Skill"), Choice->SkillID, Target);
                break;
            }
        }
    }
    if (!ActionResult.bSuccess)
        ActionResult = ASC->RequestAction(TEXT("Attack"), NAME_None, Target);

    if (!ActionResult.bSuccess) return EBTNodeResult::Failed;
    Controller->SetMonsterActionReadyAt(ASC->PlaySecond() + FMath::Max(0.25, Definition.AttackIntervalPlaySeconds));
    return EBTNodeResult::Succeeded;
}
