#pragma once
#include "CoreMinimal.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Engine/DataTable.h"
#include "ICPatternEnemy.generated.h"

USTRUCT(BlueprintType)
struct FICAttackPatternDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PatternID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0",ClampMax="1")) float MinHPRatio = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0",ClampMax="1")) float MaxHPRatio = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float MinDistanceCm = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float MaxDistanceCm = 1000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float CooldownSeconds = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float StepIntervalSeconds = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRequireLineOfSight = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FICConditionGroup Conditions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> SkillSequence;
};

USTRUCT(BlueprintType)
struct FICMonsterPatternProfileRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICAttackPatternDefinition> Patterns;
};

/** Data-driven monster patterns. Uses existing GAS skill execution. */
UCLASS()
class ISEKAICLASS_API AICPatternEnemy : public AICEnemyBase
{
    GENERATED_BODY()
public:
    AICPatternEnemy();
    UPROPERTY(EditDefaultsOnly, Category="IC|Monster|Telegraph", meta=(ClampMin="0.1")) float TelegraphLeadSeconds = .5f;
    UPROPERTY(EditDefaultsOnly, Category="IC|Monster|Patterns", meta=(ClampMin="0.1")) float NormalAttackRangeMultiplier = 1.3f;
    virtual double GetAttackRangeCm() const override { return Super::GetAttackRangeCm() * FMath::Max(.1f,NormalAttackRangeMultiplier); }
    UPROPERTY(EditDefaultsOnly, Category="IC|Monster|Telegraph") TSoftObjectPtr<class UMaterialInterface> TelegraphMaterial;
    UPROPERTY(VisibleAnywhere, Category="IC|Monster|Telegraph") TObjectPtr<class UDecalComponent> TelegraphDecal;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Monster|Patterns") bool bUseAttackPatterns = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Monster|Patterns", meta=(RowType="/Script/IsekaiClass.ICMonsterPatternProfileRow")) FDataTableRowHandle PatternProfile;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Monster|Debug") bool bShowAttackLog = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Monster|Patterns") TArray<FICAttackPatternDefinition> AttackPatterns;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Monster|Return", meta=(ClampMin="1")) float CombatReturnDistanceMultiplier = 3.f;
    virtual void Tick(float DeltaSeconds) override;
    bool TryAttackPattern(AICCharacterBase* Target);
    bool IsPatternActive() const { return ActivePattern != INDEX_NONE; }
    // A range/LOS branch abort cancels the warning, not completed combo steps.
    void CancelPatternWindup() { bAwaitingTelegraph=false; }
    bool IsHeavyAttackCommitted() const;
    AICCharacterBase* GetCommittedTarget() const { return CommittedTarget.Get(); }
    bool IsOutsidePatternLeash(const FVector& Location) const;
    double GetPatternChaseLimit() const;
private:
    UPROPERTY(Transient) TObjectPtr<class UBehaviorTree> LocalBehavior;
    bool bProfileLoaded = false;
    UPROPERTY(Transient) TArray<TObjectPtr<class UNiagaraSystem>> PreparedPatternVFX;
    FGuid TelegraphAction;
    FGuid CommittedHeavyAction;
    TWeakObjectPtr<AICCharacterBase> CommittedTarget;
    bool bAwaitingTelegraph = false;
    bool bTelegraphNeedsTarget = true;
    double TelegraphEndsAt = 0;
    float TelegraphRadiusCm = 0;
    TMap<int32,double> ReadyAt;
    int32 ActivePattern = INDEX_NONE;
    int32 StepIndex = 0;
    double NextStepAt = 0;
    bool bStoppedDefaultAI = false;
};


