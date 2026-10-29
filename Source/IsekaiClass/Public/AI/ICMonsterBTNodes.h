#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "ICMonsterBTNodes.generated.h"

UCLASS()
class ISEKAICLASS_API UBTService_ICMonsterState : public UBTService
{
    GENERATED_BODY()
public:
    UBTService_ICMonsterState();
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS()
class ISEKAICLASS_API UBTTask_ICMonsterPickWanderLocation : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTTask_ICMonsterPickWanderLocation();
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector HomeLocationKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector WanderLocationKey;

    UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
    float WanderRadius = 800.0f;
};

UCLASS()
class ISEKAICLASS_API UBTTask_ICMonsterBeginReturn : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTTask_ICMonsterBeginReturn();
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class ISEKAICLASS_API UBTTask_ICMonsterFinishReturn : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTTask_ICMonsterFinishReturn();
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class ISEKAICLASS_API UBTTask_ICMonsterFinishInvestigation : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTTask_ICMonsterFinishInvestigation();
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class ISEKAICLASS_API UBTTask_ICMonsterAttack : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTTask_ICMonsterAttack();
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
private:
    EBTNodeResult::Type PerformAttack(UBehaviorTreeComponent& OwnerComp) const;
};
