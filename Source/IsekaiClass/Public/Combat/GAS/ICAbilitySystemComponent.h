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

USTRUCT(BlueprintType, meta=(ToolTip="공통 전투 요청의 결과입니다. 실패 시 능력·비용·쿨타임은 시작하지 않으며 Reason으로 누락 정의·상태·거리 등의 원인을 확인합니다."))
struct FICCombatResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="요청 검사와 GAS 활성화에 성공했는지 나타냅니다.")) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="현재 요청의 실패 이유입니다. 성공 결과에서는 비어 있습니다.")) FString Reason;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="이번 공격·시전의 고유 ID입니다. Notify와 타격 이벤트는 이 ID로 중복·늦은 수신을 거부합니다.")) FGuid ActionID;
};

USTRUCT(BlueprintType, meta=(ToolTip="발사·효과 시점 공격자 수치 캡처입니다. 발사 이후 장비·버프가 바뀌어도 유지하고 방어자 판정은 적중 시점에 수행합니다."))
struct FICAttackCapture
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="공격자 물리 또는 마법 공격력과 계수·고정 피해·개인 CHA를 반영한 수치입니다.")) double Magnitude = 0;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="발생 시점 치명타 확률입니다. 0~1 비율입니다.")) double CriticalChance = 0;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="발생 시점 치명타 배율입니다.")) double CriticalMultiplier = 1;
    UPROPERTY(BlueprintReadOnly, meta=(ToolTip="공격자 탈진·주는 피해 등의 최종 배율입니다.")) double OutputMultiplier = 1;
};

UCLASS(meta=(ToolTip="게임 공통 GAS 실행 컴포넌트입니다. DT 정의를 조회하여 입력·AI 요청을 실행하며 활성 수치·효과·쿨타임을 캐시합니다. WorldState 복원·반영과 PlayClock 시간 처리를 공통화합니다."))
class ISEKAICLASS_API UICAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()
public:
    UICAbilitySystemComponent();
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS",meta=(ToolTip="공통 행동 GAS 능력 클래스입니다. ICGameplayAbility를 상속하는 GA를 지정합니다. 입력·비용·쿨타임·GUID 검증은 공통 C++ 실행기가 담당하고 실제 수치는 DT에서 읽습니다. 초기화 후 변경하려면 캐릭터를 다시 생성하세요.")) TSubclassOf<UGameplayAbility> ActionAbilityClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS",meta=(ToolTip="HP 증감에 사용할 즉시 GameplayEffect입니다. ICHealthEffect를 상속하여 IC.Data.Amount SetByCaller와 Health 가산 수정자를 유지합니다. 치유·피해의 실제 양은 실행기가 지정합니다.")) TSubclassOf<UGameplayEffect> HealthEffectClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS",meta=(ToolTip="MP 증감에 사용할 즉시 GameplayEffect입니다. ICManaEffect를 상속하여 IC.Data.Amount SetByCaller와 Mana 가산 수정자를 유지합니다. 최초 효과 확정 전에 비용을 중복 지불하지 않습니다.")) TSubclassOf<UGameplayEffect> ManaEffectClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS",meta=(ToolTip="플레이어 SP 증감에 사용할 즉시 GameplayEffect입니다. ICSPChangeEffect를 상속합니다. SP 집합이 없는 NPC·몬스터에서는 적용하지 않습니다.")) TSubclassOf<UGameplayEffect> SPEffectClass;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|GAS",meta=(ToolTip="상태 효과의 GAS 태그를 보유하는 무한 지속 마커 GE입니다. ICStatusGameplayEffect를 상속하고 종료 시각·틱·보호막·중첩 수치는 WorldState의 PlayClock 기록으로 처리합니다. 마커에 자원 수정자를 중복 추가하지 않습니다.")) TSubclassOf<UGameplayEffect> StatusEffectClass;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
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
    UFUNCTION(BlueprintCallable, Category="IC|Combat", meta=(ToolTip="행동을 사전 검사하고 GAS 능력을 실행합니다. Action은 Attack·BlockStart·BlockEnd·Dodge·Skill이며 Skill은 정의 ID를 전달합니다.\n@param Action Attack·BlockStart·BlockEnd·Dodge·Skill 중 요청할 행동입니다.\n@param SkillID Skill DT의 RowName과 동일한 정의 ID입니다. 일반 동작은 None입니다.\n@param Target 스킬의 대상 규칙을 만족하는 실행 캐릭터입니다. 자기 대상 스킬은 시전자를 전달합니다.\n@return 현재 조회 결과입니다. 지속 원본을 직접 수정하는 반환값이 아닙니다.")) FICCombatResult RequestAction(FName Action, FName SkillID, AICCharacterBase* Target);
    UFUNCTION(BlueprintCallable,Category="IC|Combat",meta=(ToolTip="캐릭터 ID로 스킬 대상을 선택합니다. 액터가 없는 Downed 동료는 안전한 배치가 확인된 경우에만 부활 대상을 준비합니다. 준비 실패에는 MP·쿨타임·지속 상태를 변경하지 않습니다.\n@param SkillID 실행할 Skill DT의 정의 ID입니다.\n@param TargetID 대상 캐릭터의 지속 ID입니다.\n@return 성공 여부·실패 이유·행동 GUID입니다.")) FICCombatResult RequestSkillByID(FName SkillID,FName TargetID);
    UFUNCTION(BlueprintCallable, Category="IC|Combat", meta=(ToolTip="현재 행동을 취소합니다. 첫 효과 전에는 비용 없이 재시도 대기, 첫 효과 후에는 환불 없이 남은 타격을 정리합니다."))
    void CancelAction();
    UFUNCTION(BlueprintCallable, Category="IC|Combat", meta=(ToolTip="공격·시전 ID와 타격 번호를 확인하여 애니메이션 이벤트를 처리합니다. 지난 행동·중복 이벤트·잘못된 번호는 거부합니다.\n@param ActionID 시작 요청의 반환 결과에 포함된 행동 GUID입니다. 이전 행동의 알림과 구분합니다.\n@param Event AN_HitStart·AN_HitEnd·AN_ProjectileFire·AN_EffectCommit 등의 사건 키입니다.\n@param HitIndex 정의의 반복 타격을 펼친 0 기반 번호입니다.\n@return 성공하면 참이며 실패하면 상태를 적용하지 않습니다.")) bool NotifyCombatEvent(FGuid ActionID, FName Event, int32 HitIndex);
    UFUNCTION(BlueprintPure, Category="IC|Combat", meta=(ToolTip="현재 행동 ID입니다. 애니메이션 시작 시 저장한 ID를 Notify에 전달해야 이전 모션의 늦은 이벤트를 구분할 수 있습니다.\n@return 현재 조회 결과입니다. 지속 원본을 직접 수정하는 반환값이 아닙니다.")) FGuid GetActionID() const;
    UFUNCTION(BlueprintCallable, Category="IC|Combat", meta=(ToolTip="StatusEffect DT 정의를 검사하고 상태 효과를 적용합니다. 면역·무적·대상·중첩 규칙을 검사하며 누락 정의에서는 실패합니다.\n@param EffectID StatusEffect DT의 RowName과 동일한 정의 ID입니다.\n@param Source 수치를 캡처할 시전자 ASC입니다.\n@return 성공하면 참이며 실패하면 상태를 적용하지 않습니다.")) bool ApplyStatus(FName EffectID, UICAbilitySystemComponent* Source);
    bool CanApplyStatus(FName EffectID,UICAbilitySystemComponent* Source) const;
    UFUNCTION(BlueprintCallable, Category="IC|Combat", meta=(ToolTip="허용 검사 후 지정 효과 인스턴스를 제거하는 공통 경로입니다. 보정·태그·보호막을 해당 효과만 제거하고 파생값을 재계산합니다.\n@param InstanceID 적용 시 저장된 효과 인스턴스 GUID입니다. 효과 정의 ID와 구분합니다.\n@return 성공하면 참이며 실패하면 상태를 적용하지 않습니다.")) bool RemoveStatus(FGuid InstanceID);
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
    void SetOrdinaryStaggerImmunity(bool Immune) { bOrdinaryStaggerImmune=Immune; }
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="실행 자원·효과·행동 변경 이벤트입니다. 위젯·컨트롤러는 종료 시 구독을 해제하고 이 이벤트에서 지속 원본을 중복 저장하지 않습니다.")) FICGASStateChanged OnStateChanged;
private:
    UPROPERTY(Transient, meta=(ToolTip="전투 수치 집합의 실행 참조입니다. 저장에는 포함하지 않습니다.")) TObjectPtr<UICCombatAttributeSet> Combat;
    UPROPERTY(Transient, meta=(ToolTip="플레이어·영입 NPC의 유효 주 스탯 집합입니다. 몬스터에는 생성하지 않습니다.")) TObjectPtr<UICPrimaryAttributeSet> Primary;
    UPROPERTY(Transient, meta=(ToolTip="플레이어 전용 SP 집합입니다. NPC·몬스터에는 생성하지 않습니다.")) TObjectPtr<UICSurvivalAttributeSet> Survival;
    UPROPERTY(Transient, meta=(ToolTip="실행 중 공통 GAS 능력입니다. 저장할 수 없는 일시 실행 객체입니다.")) TObjectPtr<UICGameplayAbility> RunningAbility;
    FGameplayAbilitySpecHandle AbilityHandle;
    FICCharacterState Runtime;
    TMap<FGuid, FActiveGameplayEffectHandle> EffectHandles;
    TMap<EICStat, double> MonsterBase;
    TWeakObjectPtr<AICCharacterBase> TargetActor;
    FICSkillRow Skill;
    FName RequestedAction, RequestedSkill;
    FGuid CurrentID;
    TSet<int32> EmittedHits;
    TSet<FString> HitReceipts;
    double StartedAt = 0, RecentCombatAt = -100, RetryAt = 0, DodgeReadyAt = 0, StaggerReadyAt = 0, StaggerUntil = 0;
    double ActionEndsAt = 0;
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
};
