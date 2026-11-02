#include "NPC/Behavior/StateTree/ICHomeBehaviorStateTreeNodes.h"

#include "AI/ICAIController.h"
#include "NPC/Behavior/ICNPCHomeBehaviorComponent.h"
#include "NPC/Character/ICNPCBase.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"

namespace ICHomeBehaviorNodes
{
    static UICNPCHomeBehaviorComponent* GetBehavior(const AAIController* AI)
    {
        const AICNPCBase* NPC = AI ? Cast<AICNPCBase>(AI->GetPawn()) : nullptr;
        return NPC ? NPC->GetHomeBehavior() : nullptr;
    }

    static AICAIController* GetICController(AAIController* AI)
    {
        return Cast<AICAIController>(AI);
    }
}

bool FICSTCondition_HomeBehaviorCanRun::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICNPCHomeBehaviorComponent* Behavior = ICHomeBehaviorNodes::GetBehavior(Data.AIController);
    const bool bResult = Behavior && Behavior->CanRunHomeBehavior();
    return bResult != bInvert;
}

FICSTTask_HomeBehavior::FICSTTask_HomeBehavior()
{
    bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FICSTTask_HomeBehavior::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICNPCHomeBehaviorComponent* Behavior = ICHomeBehaviorNodes::GetBehavior(Data.AIController);
    AICAIController* Controller = ICHomeBehaviorNodes::GetICController(Data.AIController);
    return Behavior && Controller && Behavior->TickBehavior(*Controller)
        ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FICSTTask_HomeBehavior::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICNPCHomeBehaviorComponent* Behavior = ICHomeBehaviorNodes::GetBehavior(Data.AIController);
    AICAIController* Controller = ICHomeBehaviorNodes::GetICController(Data.AIController);
    return Behavior && Controller && Behavior->TickBehavior(*Controller)
        ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

void FICSTTask_HomeBehavior::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    if (UICNPCHomeBehaviorComponent* Behavior = ICHomeBehaviorNodes::GetBehavior(Data.AIController))
        Behavior->StopBehavior(ICHomeBehaviorNodes::GetICController(Data.AIController));
}
