#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "UI/Framework/ICText.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Combat/GAS/Effects/ICGameplayEffects.h"
#include "Combat/GAS/Abilities/ICGameplayAbility.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"

UICAbilitySystemComponent::UICAbilitySystemComponent()
{
    IC_TRACE_FUNCTION();PrimaryComponentTick.bCanEverTick=true;SetIsReplicatedByDefault(false);
    ActionAbilityClass=UICGameplayAbility::StaticClass();HealthEffectClass=UICHealthEffect::StaticClass();ManaEffectClass=UICManaEffect::StaticClass();SPEffectClass=UICSPChangeEffect::StaticClass();StatusEffectClass=UICStatusGameplayEffect::StaticClass();
}
UICDefinitionSubsystem* UICAbilitySystemComponent::Definitions() const
{ IC_TRACE_FUNCTION(); auto* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr; return GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr; }
double UICAbilitySystemComponent::PlaySecond() const
{ IC_TRACE_FUNCTION(); auto* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr; auto* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr; return Time ? Time->GetClock().PlaySecond : 0; }
void UICAbilitySystemComponent::InitializeForCharacter(AICCharacterBase* Character, bool PrimaryStats, bool PlayerSP)
{
    IC_TRACE_FUNCTION(); if (!Character) return; InitAbilityActorInfo(Character, Character);
    if (!Combat) { Combat = NewObject<UICCombatAttributeSet>(Character); AddAttributeSetSubobject(Combat.Get()); }
    if (PrimaryStats && !Primary) { Primary = NewObject<UICPrimaryAttributeSet>(Character); AddAttributeSetSubobject(Primary.Get()); }
    if (PlayerSP && !Survival) { Survival = NewObject<UICSurvivalAttributeSet>(Character); AddAttributeSetSubobject(Survival.Get()); }
    if (!AbilityHandle.IsValid() && Character->HasAuthority()&&ActionAbilityClass&&ActionAbilityClass->IsChildOf(UICGameplayAbility::StaticClass())) AbilityHandle = GiveAbility(FGameplayAbilitySpec(ActionAbilityClass, 1));
}
bool UICAbilitySystemComponent::CanRestore(const FICCharacterState& State, FString& Reason) const
{
    IC_TRACE_FUNCTION(); Reason.Empty();
    if (!Combat || !FMath::IsFinite(State.HP) || !FMath::IsFinite(State.MP) || !FMath::IsFinite(State.SP)
        || !FMath::IsFinite(State.MaxHP) || !FMath::IsFinite(State.MaxMP) || !FMath::IsFinite(State.MaxSP)
        || State.MaxHP < 1 || State.MaxMP < 0 || State.MaxSP < 0 || State.HP < 0 || State.HP > State.MaxHP
        || State.MP < 0 || State.MP > State.MaxMP || State.SP < 0 || State.SP > State.MaxSP)
    { Reason = ICText::Get(TEXT("Native_cb3c4edccfa8")).ToString(); return false; }
    FICDefinitionResult Result;
    for (const auto& Effect : State.Combat.Effects)
        if (!Effect.InstanceID.IsValid() || !Definitions() || !Definitions()->Find<FICStatusEffectRow>(Effect.EffectID, Result))
        { Reason = ICText::Get(TEXT("Native_37891a2aeabd")).ToString(); return false; }
    for(const auto& Cooldown:State.Combat.SkillReadyAtPlaySecond)
        if(!FMath::IsFinite(Cooldown.Value)||Cooldown.Value<0||!Definitions()||!Definitions()->Find<FICSkillRow>(Cooldown.Key,Result))
        {Reason=ICText::Get(TEXT("Native_3f8aa571532a")).ToString();return false;}
    return true;
}
bool UICAbilitySystemComponent::Restore(const FICCharacterState& State)
{
    IC_TRACE_FUNCTION(); FString Reason; if (!CanRestore(State, Reason)) { UE_LOG(LogICWorldState, Warning, TEXT("[GAS.RestoreRejected] %s"), *Reason); return false; }
    const bool IdentityChanged = Runtime.CharacterId != State.CharacterId;

    if (IdentityChanged) CancelAction();
    for (const auto& Entry : CooldownEffectHandles) RemoveActiveGameplayEffect(Entry.Value);
    CooldownEffectHandles.Reset();
    { TGuardValue<bool> Guard(bRestoring, true); Runtime = State; bReady = true;
    SetNumericAttributeBase(UICCombatAttributeSet::GetMaxHealthAttribute(), State.MaxHP);
    SetNumericAttributeBase(UICCombatAttributeSet::GetMaxManaAttribute(), State.MaxMP);
    if (Survival) SetNumericAttributeBase(UICSurvivalAttributeSet::GetMaxSPAttribute(), State.MaxSP);
    RebuildDerivedStats(); SetVitals(State.HP, State.MP, State.SP); SyncEffectHandles(); SyncCooldownEffects();
    SetLooseGameplayTagCount(ICTags::Downed, State.IsDowned() ? 1 : 0);
    SetLooseGameplayTagCount(ICTags::Dead,State.HP<=0&&State.Kind!=EICCharacterKind::Recruitable?1:0);LastTickAt = PlaySecond(); }
    UpdateShouldTick();OnStateChanged.Broadcast(); return true;
}
FICCharacterState UICAbilitySystemComponent::Capture() const
{
    IC_TRACE_FUNCTION(); FICCharacterState State = Runtime;
    if (Combat) { State.HP = Combat->GetHealth(); State.MP = Combat->GetMana(); State.MaxHP = Combat->GetMaxHealth(); State.MaxMP = Combat->GetMaxMana(); }
    if (Survival) { State.SP = Survival->GetSP(); State.MaxSP = Survival->GetMaxSP(); }
    State.LastSimPlaySecond = State.Combat.LastCapturedPlaySecond = PlaySecond();
    auto* Time=GetWorld()&&GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UICTimeSubsystem>():nullptr;
    if(Time)State.LastSimGameMinute=Time->GetClock().GameMinute;
    if (GetAvatarActor()) { State.Position = GetAvatarActor()->GetActorLocation(); State.Rotation = GetAvatarActor()->GetActorRotation(); }
    return State;
}
void UICAbilitySystemComponent::SetVitals(float HP, float MP, float SP)
{
    IC_TRACE_FUNCTION(); if (!Combat) return;
    SetNumericAttributeBase(UICCombatAttributeSet::GetHealthAttribute(), HP);
    SetNumericAttributeBase(UICCombatAttributeSet::GetManaAttribute(), MP);
    if (Survival) SetNumericAttributeBase(UICSurvivalAttributeSet::GetSPAttribute(), SP);
    SetLooseGameplayTagCount(ICTags::Exhausted, Survival && Survival->GetSP() <= 0 ? 1 : 0); if(!bRestoring)OnStateChanged.Broadcast();
}
bool UICAbilitySystemComponent::ChangeResource(FName Resource, float Amount)
{
    IC_TRACE_FUNCTION(); if (!Combat || !FMath::IsFinite(Amount)) return false;
    TSubclassOf<UGameplayEffect> Type;
    if (Resource == TEXT("HP")) Type = HealthEffectClass;
    else if (Resource == TEXT("MP")) Type = ManaEffectClass;
    else if (Resource == TEXT("SP") && Survival) Type = SPEffectClass;
    else return false;
    auto Spec = MakeOutgoingSpec(Type, 1, MakeEffectContext()); if (!Spec.IsValid()) return false;
    Spec.Data->SetSetByCallerMagnitude(ICTags::DataAmount, Amount); ApplyGameplayEffectSpecToSelf(*Spec.Data);
    SetLooseGameplayTagCount(ICTags::Exhausted, Survival && Survival->GetSP() <= 0 ? 1 : 0); if(!bRestoring)OnStateChanged.Broadcast(); return true;
}
void UICAbilitySystemComponent::SetBaseStats(const FICMainStats& Stats)
{ IC_TRACE_FUNCTION(); if (Stats.IsValid()) { Runtime.MainStats = Stats; RebuildDerivedStats(); OnStateChanged.Broadcast(); } }
void UICAbilitySystemComponent::SetCombatBase(double HP, double MP, double Physical, double Magical, double Armor, double Speed, double AttackSpeed, double CritChance, double CritMultiplier)
{
    IC_TRACE_FUNCTION();
    MonsterBase = { {EICStat::MaxHP,HP},{EICStat::MaxMP,MP},{EICStat::PhysicalAttack,Physical},{EICStat::MagicAttack,Magical},
        {EICStat::Defense,Armor},{EICStat::MoveSpeed,Speed},{EICStat::AttackSpeed,AttackSpeed},{EICStat::CriticalChance,CritChance},{EICStat::CriticalMultiplier,CritMultiplier} };
}
double UICAbilitySystemComponent::Stat(EICStat Value) const
{
    IC_TRACE_FUNCTION(); if (!Combat) return 0;
    switch (Value)
    {
        case EICStat::MaxHP:return Combat->GetMaxHealth(); case EICStat::MaxMP:return Combat->GetMaxMana();
        case EICStat::MaxSP:return Survival ? Survival->GetMaxSP() : 0;
        case EICStat::PhysicalAttack:return Combat->GetPhysicalAttack(); case EICStat::MagicAttack:return Combat->GetMagicAttack();
        case EICStat::Defense:return Combat->GetDefense(); case EICStat::MoveSpeed:return Combat->GetMoveSpeed();
        case EICStat::AttackSpeed: { FICDefinitionResult R;const auto* Rule=Definitions()?Definitions()->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),R):nullptr;return Combat->GetAttackSpeed()*(Survival&&Survival->GetSP()<=0?(Rule?Rule->ExhaustedAttackSpeedMultiplier:.8):1.); } case EICStat::CriticalChance:return Combat->GetCriticalChance();
        case EICStat::CriticalMultiplier:return Combat->GetCriticalMultiplier(); case EICStat::AutoDodgeChance:return Combat->GetAutoDodgeChance();
        case EICStat::STR:return Primary?Primary->GetSTR():0; case EICStat::DEX:return Primary?Primary->GetDEX():0;
        case EICStat::INT:return Primary?Primary->GetINT():0; case EICStat::CON:return Primary?Primary->GetCON():0;
        case EICStat::CHA:return 0; case EICStat::LUK:return Primary?Primary->GetLUK():0;
        default:return 0;
    }
}
bool UICAbilitySystemComponent::IsAlive() const
{ IC_TRACE_FUNCTION(); return bReady && Combat && Combat->GetHealth() > 0 && !HasMatchingGameplayTag(ICTags::Downed) && !HasMatchingGameplayTag(ICTags::Dead); }
bool UICAbilitySystemComponent::GetShouldTick() const
{
    // GAS only knows its own tickable tasks. PlayClock actions, cooldowns and
    // status effects also need ticks while this character has a restored state.
    return bReady||Super::GetShouldTick();
}
bool UICAbilitySystemComponent::HasRecentCombat() const
{
    IC_TRACE_FUNCTION(); FICDefinitionResult R; auto* Rule = Definitions()?Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;
    return PlaySecond() - RecentCombatAt < (Rule ? Rule->RecentCombatPlaySeconds : 8);
}
void UICAbilitySystemComponent::MarkCombat() { IC_TRACE_FUNCTION(); RecentCombatAt = PlaySecond(); }
double UICAbilitySystemComponent::GetMovementCooldownRemaining() const
{
    return FMath::Max(0.,DodgeReadyAt-PlaySecond());
}
float UICAbilitySystemComponent::GetMovementCooldownProgress() const
{
    return MovementCooldownDuration>0?FMath::Clamp(1.-GetMovementCooldownRemaining()/MovementCooldownDuration,0.,1.):1.f;
}
void UICAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
    IC_TRACE_FUNCTION(); Super::TickComponent(DeltaTime, TickType, Function); if (!bReady || bRestoring) return;
    const double Now = PlaySecond(); if (Now > LastTickAt) { AdvanceEffects(Now); SyncCooldownEffects(); AdvanceAction(Now); LastTickAt = Now; }
    if (Now >= StaggerUntil) SetLooseGameplayTagCount(ICTags::Staggered, 0);
    if (Combat->GetHealth() <= 0)
    {
        if (Runtime.Kind == EICCharacterKind::Recruitable) { Runtime.LifeState = EICCharacterLifeState::Downed; SetLooseGameplayTagCount(ICTags::Downed, 1); }
        else SetLooseGameplayTagCount(ICTags::Dead, 1);
        CancelAction();SetGuarding(false); if (auto* Character = Cast<AICCharacterBase>(GetAvatarActor())) Character->HandleZeroHP();
    }
}
void UICAbilitySystemComponent::Shutdown()
{ IC_TRACE_FUNCTION(); CancelAction();SetGuarding(false); bReady = false; for (const auto& Entry : EffectHandles) RemoveActiveGameplayEffect(Entry.Value); EffectHandles.Reset(); for (const auto& Entry : CooldownEffectHandles) RemoveActiveGameplayEffect(Entry.Value); CooldownEffectHandles.Reset();UpdateShouldTick(); }
void UICAbilitySystemComponent::ResetExecutionForLoad()
{
    IC_TRACE_FUNCTION();CancelAction();SetGuarding(false);RecentCombatAt=-100;DodgeReadyAt=StaggerReadyAt=StaggerUntil=0;
    MovementCooldownDuration=MovementDuration=MovementDistance=MovementAlpha=0;
    ComboIndex=0;ComboReadyUntil=BufferedAttackUntil=0;SetLooseGameplayTagCount(ICTags::Staggered,0);
}
void UICAbilitySystemComponent::AdvanceNaturalSP(double Seconds)
{
    IC_TRACE_FUNCTION();if(!Survival||!IsAlive()||!FMath::IsFinite(Seconds)||Seconds<=0)return;
    Runtime.SPDrainElapsedPlaySeconds+=Seconds;
    const double Count=FMath::FloorToDouble(Runtime.SPDrainElapsedPlaySeconds/10.);
    Runtime.SPDrainElapsedPlaySeconds-=Count*10.;if(Count>0)ChangeResource(TEXT("SP"),-float(Count));
}
