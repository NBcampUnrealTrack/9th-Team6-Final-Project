#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Data/Rows/Combat/ICSkillRows.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICAbilitySystemComponent.generated.h"

class AICCharacterBase;
class UICGameplayAbility;
class UICCombatAttributeSet;
class UICPrimaryAttributeSet;
class UICSurvivalAttributeSet;
class UICDefinitionSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICGASStateChanged);

USTRUCT(BlueprintType)
struct FICCombatResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FString Reason;
    UPROPERTY(BlueprintReadOnly) FGuid ActionID;
};

USTRUCT(BlueprintType)
struct FICAttackCapture
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) double Magnitude = 0;
    UPROPERTY(BlueprintReadOnly) double CriticalChance = 0;
    UPROPERTY(BlueprintReadOnly) double CriticalMultiplier = 1;
    UPROPERTY(BlueprintReadOnly) double OutputMultiplier = 1;
};

UCLASS()
class ISEKAICLASS_API UICAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()
public:
    UICAbilitySystemComponent();
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS") TSubclassOf<UGameplayAbility> ActionAbilityClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS") TSubclassOf<UGameplayEffect> HealthEffectClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS") TSubclassOf<UGameplayEffect> ManaEffectClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS") TSubclassOf<UGameplayEffect> SPEffectClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS") TSubclassOf<UGameplayEffect> StatusEffectClass;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual bool GetShouldTick() const override;
    void InitializeForCharacter(AICCharacterBase* Character, bool PrimaryStats, bool PlayerSP);
    bool CanRestore(const FICCharacterState& State, FString& Reason) const;
    bool Restore(const FICCharacterState& State);
    FICCharacterState Capture() const;
    void RebuildDerivedStats();
    UICDefinitionSubsystem* Definitions() const;
    double PlaySecond() const;
    double Stat(EICStat Value) const;
    bool Conditions(const FICConditionGroup& Group, const UICAbilitySystemComponent* Target = nullptr, bool BaseOnly = false) const;
    bool ChangeResource(FName Resource, float Amount);
    void SetVitals(float HP, float MP, float SP);
    void SetBaseStats(const FICMainStats& Stats);
    void SetCombatBase(double HP, double MP, double Physical, double Magical, double Defense, double Speed, double AttackSpeed, double CritChance, double CritMultiplier);
    UFUNCTION(BlueprintCallable, Category="IC|Combat") FICCombatResult RequestAction(FName Action, FName SkillID, AICCharacterBase* Target);
    UFUNCTION(BlueprintCallable,Category="IC|Combat") FICCombatResult RequestSkillByID(FName SkillID,FName TargetID);
    UFUNCTION(BlueprintPure, Category="IC|Weapons") EICWeaponFamily GetEquippedWeaponFamily() const;
    UFUNCTION(BlueprintPure, Category="IC|Weapons") EICWeaponFamily GetSkillWeaponFamily(FName SkillID) const;
    UFUNCTION(BlueprintPure, Category="IC|Weapons") FName GetCurrentWeaponSkill() const;
    UFUNCTION(BlueprintCallable, Category="IC|Weapons") bool EquipWeaponSkill(EICWeaponFamily Family, FName SkillID);
    UFUNCTION(BlueprintPure, Category="IC|Combat") double GetSkillCooldownRemaining(FName SkillID) const;
    UFUNCTION(BlueprintPure, Category="IC|Movement") double GetMovementCooldownRemaining() const;
    UFUNCTION(BlueprintPure, Category="IC|Movement") float GetMovementCooldownProgress() const;
    UFUNCTION(BlueprintCallable, Category="IC|Combat")
    void CancelAction();
    UFUNCTION(BlueprintCallable, Category="IC|Combat") bool NotifyCombatEvent(FGuid ActionID, FName Event, int32 HitIndex);
    UFUNCTION(BlueprintPure, Category="IC|Combat") FGuid GetActionID() const;
    UFUNCTION(BlueprintCallable, Category="IC|Combat") bool ApplyStatus(FName EffectID, UICAbilitySystemComponent* Source);
    bool CanApplyStatus(FName EffectID,UICAbilitySystemComponent* Source) const;
    UFUNCTION(BlueprintCallable, Category="IC|Combat") bool RemoveStatus(FGuid InstanceID);
    bool ExecuteEffect(const FICSkillEffectRow& Effect, UICAbilitySystemComponent* Target, const FICAttackCapture* Captured = nullptr);
    FICAttackCapture CaptureAttack(const FICCombatMagnitude& Magnitude, EICDamageType Type, bool PersonalCHA) const;
    bool ApplyDamage(const FICAttackCapture& Attack, UICAbilitySystemComponent* Source, bool Critical, bool Guardable, bool AutoDodge, bool CancelCasting);
    void SetGuarding(bool Guarding);
    bool IsAlive() const;
    bool HasRecentCombat() const;
    void MarkCombat();
    bool BeginRequestedAction(UICGameplayAbility* Ability);
    void EndCurrentAction(bool Cancelled);
    void Shutdown();
    void ResetExecutionForLoad();
    void RemoveSleepEffects();
    void AdvanceNaturalSP(double Seconds);
    void NotifyEquipmentChanged();
    bool NotifyFromMontage(int32 MontageInstanceID,FName Event,int32 HitIndex);
    void PlayCombatCue(bool bImpact, const FICCombatPresentation& Presentation, AActor* Recipient, const FVector& Location);
    void SetOrdinaryStaggerImmunity(bool Immune) { bOrdinaryStaggerImmune=Immune; }
    UPROPERTY(BlueprintAssignable) FICGASStateChanged OnStateChanged;
private:
    UPROPERTY(Transient) TObjectPtr<UICCombatAttributeSet> Combat;
    UPROPERTY(Transient) TObjectPtr<UICPrimaryAttributeSet> Primary;
    UPROPERTY(Transient) TObjectPtr<UICSurvivalAttributeSet> Survival;
    UPROPERTY(Transient) TObjectPtr<UICGameplayAbility> RunningAbility;
    FGameplayAbilitySpecHandle AbilityHandle;
    TMap<FName, FGameplayAbilitySpecHandle> SkillAbilityHandles;
    TMap<FName, FActiveGameplayEffectHandle> CooldownEffectHandles;
    FICCharacterState Runtime;
    TMap<FGuid, FActiveGameplayEffectHandle> EffectHandles;
    TMap<EICStat, double> MonsterBase;
    TWeakObjectPtr<AICCharacterBase> TargetActor;
    FICSkillRow Skill;
    FName RequestedAction, RequestedSkill;
    FGuid CurrentID;
    TSet<int32> EmittedHits;
    TSet<FString> HitReceipts;
    double StartedAt = 0, RecentCombatAt = -100, DodgeReadyAt = 0, StaggerReadyAt = 0, StaggerUntil = 0;
    double ActionEndsAt = 0;
    double MovementCooldownDuration = 0, MovementDuration = 0, MovementDistance = 0, MovementAlpha = 0;
    int32 ComboIndex = 0;
    double ComboReadyUntil = 0, BufferedAttackUntil = 0;
    bool bCommitted = false, bReady = false, bRestoring = false, bEnding = false, bGuarding = false;
    bool bOrdinaryStaggerImmune = false;
    bool bCancelWindow=false;
    int32 ActiveMontageInstanceID=INDEX_NONE;
    TSet<int32> OpenTraceHits;
    TMap<int32,FVector> LastTraceStart,LastTraceEnd;
    TMap<int32,double> TraceEndsAt;
    FVector StartLocation = FVector::ZeroVector, DodgeDirection = FVector::ZeroVector;
    double LastTickAt = 0;
    void AdvanceAction(double Now);
    bool ValidateAction(FString& Reason) const;
    bool CommitFirstEffect();
    bool EmitHit(int32 Index,bool ContinueTrace=false);
    void AdvanceEffects(double Now);
    void SyncEffectHandles();
    void ClearActionTags();
    void UnlockSkills();
    FGameplayAbilitySpecHandle GetOrGrantSkillAbility(const FICSkillRow& Row);
    void SyncCooldownEffects();
    void ApplySkillCooldown(const FICSkillRow& Row, double Duration);
};
