#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "ICAIController.generated.h"
class UStateTreeAIComponent;
class UStateTree;
class UBehaviorTree;
UCLASS(Blueprintable,meta=(ToolTip="상태 복원이 끝난 NPC의 StateTree 또는 몬스터의 BehaviorTree를 실행합니다. 에셋이 없으면 NPC 일과·동료 추종과 몬스터 탐지·스킬·귀환을 기본 C++ 경로로 실행합니다."))
class ISEKAICLASS_API AICAIController : public AAIController
{
    GENERATED_BODY()
public:
    AICAIController();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnUnPossess() override;
    bool StartExecution(UStateTree* Tree,UBehaviorTree* Behavior);
    void StopExecution();
protected:
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="IC|AI",meta=(ToolTip="NPC 일과 StateTree 실행 컴포넌트입니다. 자동 시작을 끄고 상태 복원 후 명시적으로 시작합니다.")) TObjectPtr<UStateTreeAIComponent> ScheduleRunner;
private:
    bool bExecutionReady=false,bUsesAuthoredTree=false;
    double NextDecisionAt=0;
    TWeakObjectPtr<class AICCharacterBase> Target;
    void Decide();
};
