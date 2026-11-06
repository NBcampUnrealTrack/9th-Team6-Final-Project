#pragma once
#include "CoreMinimal.h"
#include "AI/ICMonsterBTNodes.h"
#include "ICPatternBT.generated.h"
UCLASS()
class UBTTask_ICPatternAttack : public UBTTask_ICMonsterAttack
{
 GENERATED_BODY()
public:
 virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory) override;
 virtual void TickTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory,float DeltaSeconds) override;
 virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory) override;
};
UCLASS()
class UBTService_ICPatternState : public UBTService_ICMonsterState
{
 GENERATED_BODY()
public:
 virtual void TickNode(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory,float DeltaSeconds) override;
};

