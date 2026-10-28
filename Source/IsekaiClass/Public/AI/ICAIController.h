#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "NavAreas/NavArea.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "ICAIController.generated.h"
class UStateTreeAIComponent;
class UStateTree;
class UBehaviorTree;
class UNavigationQueryFilter;
class AICCharacterBase;
class AICNPCBase;

/** Assign this Area Class to a NavLink that only player companions may traverse. */
UCLASS()
class ISEKAICLASS_API UICCompanionOnlyNavArea : public UNavArea
{
    GENERATED_BODY()
};

/** The default AI filter: enemies and scheduled NPCs do not route through companion-only links. */
UCLASS()
class ISEKAICLASS_API UICNonCompanionNavigationFilter : public UNavigationQueryFilter
{
    GENERATED_BODY()
public:
    UICNonCompanionNavigationFilter(const FObjectInitializer& ObjectInitializer);
};

UCLASS(Blueprintable)
class ISEKAICLASS_API AICAIController : public AAIController
{
    GENERATED_BODY()
public:
    AICAIController();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnUnPossess() override;
    bool StartExecution(UStateTree* Tree,UBehaviorTree* Behavior);
    virtual void StopExecution();

    void SetInteractionHold(bool bHold,AActor* FaceTarget);

    void RestoreScheduleFacing();
    bool IsInteractionHeld() const { return bInteractionHold; }
    bool TryRecoverCompanion();
    /** Used by every non-companion AI path request. Companions intentionally retain the default filter. */
    TSubclassOf<UNavigationQueryFilter> GetNonCompanionNavigationFilter() const;
    UFUNCTION(BlueprintCallable,Category="IC|Party") void OrderAttack(AICCharacterBase* Enemy);
    UFUNCTION(BlueprintPure,Category="IC|Party") AICCharacterBase* GetAttackTarget() const{return Target.Get();}
    UFUNCTION(BlueprintPure,Category="IC|Party") AICCharacterBase* GetQueuedAttackTarget() const{return OrderedTarget.Get();}
protected:
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="IC|AI") TObjectPtr<UStateTreeAIComponent> ScheduleRunner;
private:
    bool bExecutionReady=false,bUsesAuthoredTree=false,bInteractionHold=false;
    double NextDecisionAt=0;
    TWeakObjectPtr<class AICCharacterBase> Target;
    TWeakObjectPtr<AICCharacterBase> OrderedTarget;
    bool DecideCompanionCombat(AICNPCBase* NPC);
    void Decide();
};
