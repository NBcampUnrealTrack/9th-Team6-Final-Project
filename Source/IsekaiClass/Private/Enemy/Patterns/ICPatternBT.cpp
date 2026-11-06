#include "AI/ICMonsterBTNodes.h"

#include "AI/ICAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Base/ICCharacterBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Debug/ICLog.h"
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
    const FName LastKnownTargetLocationKey(TEXT("LastKnownTargetLocation"));
    const FName IsInvestigatingKey(TEXT("IsInvestigating"));

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


#include "Enemy/Patterns/ICPatternBT.h"
#include "Enemy/Patterns/ICPatternEnemy.h"
void UBTService_ICPatternState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    UBTService::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    auto* Controller = Cast<AICAIController>(OwnerComp.GetAIOwner());
    auto* Enemy = Controller ? Cast<AICEnemyBase>(Controller->GetPawn()) : nullptr;
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Enemy || !Blackboard || !Enemy->GetICAbilitySystem() || !Enemy->GetICAbilitySystem()->IsAlive()) return;

    auto* LocalEnemy=Cast<AICPatternEnemy>(Enemy);
    if (!LocalEnemy) { UBTService_ICMonsterState::TickNode(OwnerComp,NodeMemory,DeltaSeconds); return; }
    const FICMonsterRow& Definition = Enemy->GetMonsterDefinition();
    // Finish an announced area attack before selecting a chase branch.
    if (LocalEnemy->IsHeavyAttackCommitted() && !LocalEnemy->IsOutsidePatternLeash(Enemy->GetActorLocation()))
    {
        auto* Committed=LocalEnemy->GetCommittedTarget();
        if (IsValid(Committed) && Committed->GetICAbilitySystem() && Committed->GetICAbilitySystem()->IsAlive())
        {
            WriteObject(Blackboard, TargetActorKey, Committed);
            WriteBool(Blackboard, IsInAttackRangeKey, true);
            WriteBool(Blackboard, IsReturningKey, false);
            Controller->SetMonsterBehaviorTarget(Committed);
            return;
        }
    }
    WriteObject(Blackboard, SelfActorKey, Enemy);
    WriteVector(Blackboard, HomeLocationKey, Enemy->GetOrigin());

    AICCharacterBase* PreviousTarget = Cast<AICCharacterBase>(Blackboard->GetValueAsObject(TargetActorKey));
    const bool bWasReturning = Blackboard->GetValueAsBool(IsReturningKey);
    const bool bWasInvestigating = Blackboard->GetValueAsBool(IsInvestigatingKey);
    bool bInvestigating = bWasInvestigating;
    AICCharacterBase* Target = nullptr;
    if (!Enemy->IsReturning())
    {
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
        const double ChaseLimitSquared = FMath::Square(FMath::Max(50.0, LocalEnemy->GetPatternChaseLimit()));
        const bool bEnemyOutsideChaseLimit = HomeDistanceSquared > ChaseLimitSquared;
        const bool bTargetOutsideChaseLimit = Target
            && FVector::DistSquared(Target->GetActorLocation(), Enemy->GetOrigin()) > ChaseLimitSquared;
        const bool bTargetOutsideLeash = Target && LocalEnemy->IsOutsidePatternLeash(Target->GetActorLocation());

        if (Target && !bTargetOutsideChaseLimit && !bTargetOutsideLeash)
        {
            FVector PursuitDirection = Target->GetVelocity().GetSafeNormal2D();
            if (PursuitDirection.IsNearlyZero())
                PursuitDirection = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();

            FVector InvestigationDestination = Target->GetActorLocation()
                + PursuitDirection * Definition.InvestigationAdvanceDistanceCm;
            FVector OriginToDestination = InvestigationDestination - Enemy->GetOrigin();
            OriginToDestination.Z = 0.0;
            const double ChaseLimit = FMath::Max(50.0, LocalEnemy->GetPatternChaseLimit());
            if (OriginToDestination.SizeSquared() > FMath::Square(ChaseLimit))
            {
                const FVector ClampedDirection = OriginToDestination.GetSafeNormal();
                InvestigationDestination.X = Enemy->GetOrigin().X + ClampedDirection.X * ChaseLimit;
                InvestigationDestination.Y = Enemy->GetOrigin().Y + ClampedDirection.Y * ChaseLimit;
            }
            if (LocalEnemy->IsOutsidePatternLeash(InvestigationDestination))
                InvestigationDestination = Target->GetActorLocation();

            WriteVector(Blackboard, LastKnownTargetLocationKey, InvestigationDestination);
            bInvestigating = false;
        }

        const bool bLostTarget = PreviousTarget && !Target;
        const bool bShouldInvestigate = PreviousTarget
            && (bTargetOutsideChaseLimit || bTargetOutsideLeash || bLostTarget);
        if (bTargetOutsideChaseLimit || bTargetOutsideLeash)
            Target = nullptr;
        if (bShouldInvestigate)
        {
            bInvestigating = true;
        }

        if (LocalEnemy->IsOutsidePatternLeash(Enemy->GetActorLocation()) || bEnemyOutsideChaseLimit)
        {
            Enemy->BeginReturn();
            Target = nullptr;
            bInvestigating = false;
        }
    }

    const bool bReturning = Enemy->IsReturning();
    if (bReturning) bInvestigating = false;
    const bool bInAttackRange = !bReturning && Target
        && FVector::DistSquared(Enemy->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(Enemy->GetAttackRangeCm());

    WriteObject(Blackboard, TargetActorKey, bReturning ? nullptr : Target);
    WriteBool(Blackboard, IsReturningKey, bReturning);
    WriteBool(Blackboard, IsInAttackRangeKey, bInAttackRange);
    WriteBool(Blackboard, IsInvestigatingKey, bInvestigating);
    Controller->SetMonsterBehaviorTarget(bReturning ? nullptr : Target);

    AICCharacterBase* CurrentTarget = bReturning ? nullptr : Target;
    if (PreviousTarget != CurrentTarget || bWasReturning != bReturning
        || bWasInvestigating != bInvestigating)
    {
        const double EnemyHomeDistance = FVector::Distance(Enemy->GetActorLocation(), Enemy->GetOrigin());
        const double TargetHomeDistance = CurrentTarget
            ? FVector::Distance(CurrentTarget->GetActorLocation(), Enemy->GetOrigin())
            : -1.0;
        UE_LOG(LogICCore, Log,
            TEXT("[AI.StateChanged] Pawn=%s Target=%s->%s Returning=%s->%s Investigating=%s->%s EnemyHomeDistance=%.1f TargetHomeDistance=%.1f ChaseLimit=%.1f"),
            *GetNameSafe(Enemy), *GetNameSafe(PreviousTarget), *GetNameSafe(CurrentTarget),
            bWasReturning ? TEXT("true") : TEXT("false"), bReturning ? TEXT("true") : TEXT("false"),
            bWasInvestigating ? TEXT("true") : TEXT("false"), bInvestigating ? TEXT("true") : TEXT("false"),
            EnemyHomeDistance, TargetHomeDistance, LocalEnemy->GetPatternChaseLimit());
    }
}

EBTNodeResult::Type UBTTask_ICPatternAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory)
{
 auto* AI=Cast<AICAIController>(OwnerComp.GetAIOwner());
 auto* Enemy=AI?Cast<AICPatternEnemy>(AI->GetPawn()):nullptr;
 auto* BB=OwnerComp.GetBlackboardComponent();
 auto* Target=BB?Cast<AICCharacterBase>(BB->GetValueAsObject(TEXT("TargetActor"))):nullptr;
 auto* ASC=Enemy?Enemy->GetICAbilitySystem():nullptr;
 if(Enemy&&ASC&&Target)
 {
  if(ASC->PlaySecond()<AI->GetMonsterActionReadyAt()) return EBTNodeResult::InProgress;
  if(Enemy->TryAttackPattern(Target))
  {
   if(Enemy->IsPatternActive()||ASC->GetActionID().IsValid()) return EBTNodeResult::InProgress;
   AI->SetMonsterActionReadyAt(ASC->PlaySecond()+FMath::Max(.25,Enemy->GetMonsterDefinition().AttackIntervalPlaySeconds));
   return EBTNodeResult::Succeeded;
  }
 }
 return Super::ExecuteTask(OwnerComp,NodeMemory);
}
void UBTTask_ICPatternAttack::TickTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory,float DeltaSeconds)
{
 const auto Result=ExecuteTask(OwnerComp,NodeMemory);
 if(Result!=EBTNodeResult::InProgress) FinishLatentTask(OwnerComp,Result);
}

EBTNodeResult::Type UBTTask_ICPatternAttack::AbortTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory)
{
 if(auto* AI=OwnerComp.GetAIOwner()) if(auto* Enemy=Cast<AICPatternEnemy>(AI->GetPawn())) Enemy->CancelPatternWindup();
 return Super::AbortTask(OwnerComp,NodeMemory);
}

