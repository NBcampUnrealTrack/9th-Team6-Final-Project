#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "ICAIController.generated.h"
class UStateTreeAIComponent;
class UStateTree;
class UBehaviorTree;
UCLASS(Blueprintable)
class ISEKAICLASS_API AICAIController : public AAIController
{
    GENERATED_BODY()
public:
    AICAIController();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnUnPossess() override;
    bool StartExecution(UStateTree* Tree,UBehaviorTree* Behavior);
    void StopExecution();

    void SetInteractionHold(bool bHold,AActor* FaceTarget);

    void RestoreScheduleFacing();
    bool IsInteractionHeld() const { return bInteractionHold; }
    bool TryRecoverCompanion();
    void SetMonsterBehaviorTarget(class AICCharacterBase* NewTarget) { Target = NewTarget; }
    double GetMonsterActionReadyAt() const { return NextMonsterActionAt; }
    void SetMonsterActionReadyAt(double PlaySecond) { NextMonsterActionAt = PlaySecond; }
protected:
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="IC|AI") TObjectPtr<UStateTreeAIComponent> ScheduleRunner;
private:
    bool bExecutionReady=false,bUsesAuthoredTree=false,bInteractionHold=false;
    double NextDecisionAt=0,NextMonsterActionAt=0;
    TWeakObjectPtr<class AICCharacterBase> Target;
    void Decide();
};
