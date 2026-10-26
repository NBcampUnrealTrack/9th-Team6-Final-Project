#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "NPC/Character/ICNPCBase.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Combat/GAS/Effects/ICGameplayEffects.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Debug/ICLog.h"
#include "Enemy/Character/ICEnemyBase.h"

FICAttackCapture UICAbilitySystemComponent::CaptureAttack(const FICCombatMagnitude& M, EICDamageType Type, bool PersonalCHA) const
{
    IC_TRACE_FUNCTION(); FICAttackCapture Result;
    const EICStat S=M.SourceStat==EICStat::None?(Type==EICDamageType::Physical?EICStat::PhysicalAttack:EICStat::MagicAttack):M.SourceStat;
    Result.Magnitude=FMath::Max(0.,Stat(S)*M.Coefficient+M.FlatAmount);
    FICDefinitionResult R;FICStatSettingsRow Defaults;const auto* Rules=Definitions()?Definitions()->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),R):nullptr;if(!Rules)Rules=&Defaults;
    // PersonalCHA and the serialized magnitude flag remain readable for older assets.
    Result.CriticalChance=Stat(EICStat::CriticalChance);Result.CriticalMultiplier=Stat(EICStat::CriticalMultiplier);
    if(Survival&&Survival->GetSP()<=0)Result.OutputMultiplier*=Rules->ExhaustedDamageMultiplier;
    for(const auto& E:Runtime.Combat.Effects)if(const auto* Rule=Definitions()?Definitions()->Find<FICStatusEffectRow>(E.EffectID,R):nullptr)Result.OutputMultiplier*=Rule->DamageDealtMultiplier;
    return Result;
}
bool UICAbilitySystemComponent::ApplyDamage(const FICAttackCapture& Attack, UICAbilitySystemComponent* Source, bool Critical, bool Guardable, bool AutoDodge, bool CancelCasting)
{
    IC_TRACE_FUNCTION(); if(!IsAlive()||HasMatchingGameplayTag(ICTags::Invulnerable)||HasMatchingGameplayTag(ICTags::Returning))return false;
    if(AutoDodge&&FMath::FRand()<Stat(EICStat::AutoDodgeChance))return false;
    FICDefinitionResult R;FICCombatSettingsRow Defaults;const auto* Rules=Definitions()?Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;if(!Rules)Rules=&Defaults;
    double Guard=1;
    if(Guardable&&bGuarding&&Source&&GetAvatarActor()&&Source->GetAvatarActor())
    {const FVector Incoming=(Source->GetAvatarActor()->GetActorLocation()-GetAvatarActor()->GetActorLocation()).GetSafeNormal2D();
     if(FVector::DotProduct(GetAvatarActor()->GetActorForwardVector(),Incoming)>=FMath::Cos(FMath::DegreesToRadians(Rules->GuardArcDegrees*.5)))Guard=Rules->GuardDamageMultiplier;}
    double Received=1;for(const auto& E:Runtime.Combat.Effects)if(const auto* Def=Definitions()?Definitions()->Find<FICStatusEffectRow>(E.EffectID,R):nullptr)Received*=Def->DamageReceivedMultiplier;
    double Damage=Attack.Magnitude*Rules->DefenseFormulaK/(Rules->DefenseFormulaK+FMath::Max(0.,Stat(EICStat::Defense)))*Guard*Received*Attack.OutputMultiplier;
    if(Critical&&FMath::FRand()<Attack.CriticalChance)Damage*=Attack.CriticalMultiplier;
    if(Damage>0)Damage=FMath::Max(1.,FMath::RoundToDouble(Damage));
    TArray<int32> Shields;
    for(int32 I=0;I<Runtime.Combat.Effects.Num();++I)if(Runtime.Combat.Effects[I].RemainingShield>0)Shields.Add(I);
    Shields.Sort([this](int32 A,int32 B){IC_TRACE_FUNCTION();const auto& X=Runtime.Combat.Effects[A];const auto& Y=Runtime.Combat.Effects[B];return (X.bInfinite?DBL_MAX:X.ExpiresAtPlaySecond)<(Y.bInfinite?DBL_MAX:Y.ExpiresAtPlaySecond);});
    for(int32 I:Shields){auto& Shield=Runtime.Combat.Effects[I];const double Used=FMath::Min(Shield.RemainingShield,Damage);Shield.RemainingShield-=Used;Damage-=Used;if(Damage<=0)break;}
    const float PreviousHP=Combat->GetHealth();
    if(Damage>0)ChangeResource(TEXT("HP"),-Damage);
    const float AppliedDamage=PreviousHP-Combat->GetHealth();
    if(AppliedDamage>0)
    {
        if(auto* Enemy=Cast<AICEnemyBase>(GetAvatarActor()))Enemy->PlayHitReaction();
        if(auto* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr)
            if(auto* State=GI->GetSubsystem<UICWorldStateSubsystem>())State->OnCombatDamageReceived.Broadcast(GetAvatarActor(),AppliedDamage);
    }
    MarkCombat();if(Source)Source->MarkCombat();
    if(CancelCasting&&CurrentID.IsValid()&&(RequestedAction!=TEXT("Skill")||Skill.CancelRules.bCancelOnEffectiveHit))CancelAction();
    if(!bOrdinaryStaggerImmune&&PlaySecond()>=StaggerReadyAt)
    { StaggerReadyAt=PlaySecond()+Rules->HitStaggerIntervalPlaySeconds;StaggerUntil=PlaySecond()+Rules->HitStaggerPlaySeconds;SetLooseGameplayTagCount(ICTags::Staggered,1); }
    if(!bRestoring)OnStateChanged.Broadcast();return true;
}
bool UICAbilitySystemComponent::ExecuteEffect(const FICSkillEffectRow& Effect, UICAbilitySystemComponent* Target, const FICAttackCapture* Captured)
{
    IC_TRACE_FUNCTION(); if(!Target||!Conditions(Effect.ApplicationConditions,Target))return false;
    const FICAttackCapture Attack=Captured?*Captured:CaptureAttack(Effect.Magnitude,Effect.DamageType,true);
    switch(Effect.Kind)
    {
        case EICSkillEffectKind::Damage:return Target->ApplyDamage(Attack,this,Effect.bCanCritical,Effect.bCanBeGuarded,Effect.bCanBeAutoDodged,Effect.bCancelsCastingOnEffectiveHit);
        case EICSkillEffectKind::Heal:
        {
            if(!Target->IsAlive())return false;double Multiplier=1;FICDefinitionResult R;
            for(const auto& E:Target->Runtime.Combat.Effects)if(const auto* Def=Target->Definitions()?Target->Definitions()->Find<FICStatusEffectRow>(E.EffectID,R):nullptr)Multiplier*=Def->HealingReceivedMultiplier;
            return Target->ChangeResource(TEXT("HP"),FMath::RoundToDouble(Attack.Magnitude*Multiplier));
        }
        case EICSkillEffectKind::Shield:case EICSkillEffectKind::ApplyStatus:
        {
            if(Effect.AppliedStatusEffectIDs.IsEmpty())return false;
            for(FName ID:Effect.AppliedStatusEffectIDs)if(!Target->CanApplyStatus(ID,this))return false;
            const auto Before=Target->Capture();bool Applied=true;{TGuardValue<bool> Guard(Target->bRestoring,true);for(FName ID:Effect.AppliedStatusEffectIDs)if(!Target->ApplyStatus(ID,this)){Applied=false;break;}}
            if(!Applied){Target->Restore(Before);return false;}Target->OnStateChanged.Broadcast();
            return true;
        }
        case EICSkillEffectKind::Dispel:
        {
            FICDefinitionResult R;TArray<FGuid> Remove;
            for(const auto& E:Target->Runtime.Combat.Effects)if(const auto* Def=Target->Definitions()?Target->Definitions()->Find<FICStatusEffectRow>(E.EffectID,R):nullptr)
                if(Effect.DispellableStatusEffectIDs.Contains(E.EffectID)||Def->DispelTags.HasAny(Effect.DispellableEffectTags))Remove.Add(E.InstanceID);
            if(Effect.MaximumDispelCount>0&&Remove.Num()>Effect.MaximumDispelCount)Remove.SetNum(Effect.MaximumDispelCount);
            for(FGuid ID:Remove)Target->RemoveStatus(ID);return !Remove.IsEmpty();
        }
        case EICSkillEffectKind::Revive:
            if(!Target->Runtime.IsDowned()||!Target->Combat||!GetWorld()->GetSubsystem<UICSpawnSubsystem>()->CanReviveAtCurrentPlacement(Cast<AICCharacterBase>(Target->GetAvatarActor())))return false;
            Target->Runtime.LifeState=EICCharacterLifeState::Alive;Target->SetLooseGameplayTagCount(ICTags::Downed,0);
            Target->RebuildDerivedStats();Target->SetVitals(FMath::Max(1.f,Target->Combat->GetMaxHealth()*Effect.ReviveHPRatio),
                Effect.ReviveMPPolicy==EICReviveMPPolicy::Preserve?Target->Combat->GetMana():Effect.ReviveMPPolicy==EICReviveMPPolicy::AtLeastMaxMPRatio?FMath::Max<double>(Target->Combat->GetMana(),Target->Combat->GetMaxMana()*Effect.ReviveMPRatio):Target->Combat->GetMaxMana()*Effect.ReviveMPRatio,0);
            if(auto* NPC=Cast<AICNPCBase>(Target->GetAvatarActor())){NPC->SetActorEnableCollision(true);NPC->FlushToWorldState_Implementation(GetWorld()->GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>());NPC->RefreshExecution();}return true;
        case EICSkillEffectKind::Movement:
            if(!Target->GetAvatarActor())return false;
            return Target->GetAvatarActor()->SetActorLocation(Target->GetAvatarActor()->GetActorLocation()+Target->GetAvatarActor()->GetActorForwardVector()*Effect.MovementDistanceCm,true);
        default:UE_LOG(LogICCore, Warning, TEXT("[GAS.UnsupportedEffect] ID=%s Custom=%s"),*Effect.ID.ToString(),*Effect.CustomExecutionTag.ToString());return false;
    }
}
bool UICAbilitySystemComponent::CanApplyStatus(FName ID, UICAbilitySystemComponent* Source) const
{
    IC_TRACE_FUNCTION();FICDefinitionResult R;const auto* Rule=Definitions()?Definitions()->Find<FICStatusEffectRow>(ID,R):nullptr;
    if(!Rule||!Combat||HasMatchingGameplayTag(ICTags::Returning)||HasAnyMatchingGameplayTags(Rule->BlockedByTargetTags)
        ||(Rule->bBlockedByInvulnerability&&HasMatchingGameplayTag(ICTags::Invulnerable))||!Conditions(Rule->ApplicationConditions,this))return false;
    if(Rule->Kind==EICStatusKind::Custom)return false;
    if((Runtime.IsDowned()&&Rule->TargetState==EICCombatTargetState::Living)||(!Runtime.IsDowned()&&Rule->TargetState==EICCombatTargetState::Downed)||(!Runtime.IsDowned()&&!IsAlive()))return false;
    if(Source&&Source->GetAvatarActor()&&GetAvatarActor())
    {
        const bool Hostile=Cast<AICCharacterBase>(Source->GetAvatarActor())->IsHostileTo(Cast<AICCharacterBase>(GetAvatarActor()));
        if((Rule->TargetSide==EICEffectTargetSide::Friendly&&Hostile)||(Rule->TargetSide==EICEffectTargetSide::Hostile&&!Hostile))return false;
    }
    for(const auto& E:Runtime.Combat.Effects)if(const auto* Existing=Definitions()->Find<FICStatusEffectRow>(E.EffectID,R))
        if(Existing->GrantedStatusImmunityIDs.Contains(ID)||Rule->Tags.HasAny(Existing->GrantedStatusImmunityTags))return false;
    return true;
}
bool UICAbilitySystemComponent::ApplyStatus(FName ID,UICAbilitySystemComponent* Source)
{
    IC_TRACE_FUNCTION();if(!CanApplyStatus(ID,Source))return false;FICDefinitionResult R;const auto* Rule=Definitions()->Find<FICStatusEffectRow>(ID,R);
    FICActiveEffectState State;State.InstanceID=FGuid::NewGuid();State.EffectID=ID;State.SourceEntityID=Source?Source->Runtime.CharacterId:NAME_None;
    State.bInfinite=Rule->bPermanent;State.bHarmful=Rule->bHarmful;State.ExpiresAtPlaySecond=PlaySecond()+Rule->DurationPlaySeconds;
    State.NextTickPlaySecond=PlaySecond()+(Rule->bTickImmediatelyOnApplication?0:Rule->TickIntervalPlaySeconds);
    State.CapturedMagnitudes.Add(TEXT("Strength"),Rule->Strength);
    if(Source)
    {
        const auto Periodic=Source->CaptureAttack(Rule->PeriodicMagnitude,Rule->PeriodicDamageType,true);
        State.CapturedMagnitudes.Add(TEXT("Magnitude"),Periodic.Magnitude);State.CapturedMagnitudes.Add(TEXT("CritChance"),Periodic.CriticalChance);
        State.CapturedMagnitudes.Add(TEXT("CritMultiplier"),Periodic.CriticalMultiplier);State.CapturedMagnitudes.Add(TEXT("Output"),Periodic.OutputMultiplier);
        State.RemainingShield=Rule->Kind==EICStatusKind::Shield?Source->CaptureAttack(Rule->InitialShieldMagnitude,EICDamageType::Magical,true).Magnitude:0;
        if(Rule->Kind==EICStatusKind::Shield)State.CapturedMagnitudes[TEXT("Strength")]=State.RemainingShield;
        if(Rule->Kind==EICStatusKind::DamageOverTime||Rule->Kind==EICStatusKind::HealOverTime)State.CapturedMagnitudes[TEXT("Strength")]=Periodic.Magnitude;
    }
    auto* Existing=Runtime.Combat.Effects.FindByPredicate([ID](const auto& E){return E.EffectID==ID;});
    if(Existing&&Rule->StackingPolicy!=EICStackingPolicy::Independent)
    {
        if(Rule->StackingPolicy==EICStackingPolicy::KeepStrongerRefreshDuration)
        {
            if(Existing->CapturedMagnitudes.FindRef(TEXT("Strength"))>State.CapturedMagnitudes.FindRef(TEXT("Strength")))
            {Existing->ExpiresAtPlaySecond=State.ExpiresAtPlaySecond;Existing->RemainingShield=FMath::Max(Existing->RemainingShield,State.RemainingShield);SyncEffectHandles();return true;}
            State.RemainingShield=FMath::Max(Existing->RemainingShield,State.RemainingShield);
        }
        if(Rule->StackingPolicy==EICStackingPolicy::AddStacks)State.Stacks=FMath::Min(Rule->MaximumStacks,Existing->Stacks+1);
        State.InstanceID=Existing->InstanceID;*Existing=State;
    }
    else Runtime.Combat.Effects.Add(State);
    SyncEffectHandles();RebuildDerivedStats();if(!bRestoring)OnStateChanged.Broadcast();return true;
}
void UICAbilitySystemComponent::SyncEffectHandles()
{
    IC_TRACE_FUNCTION();TSet<FGuid> IDs;
    for(const auto& E:Runtime.Combat.Effects)
    {
        IDs.Add(E.InstanceID);if(EffectHandles.Contains(E.InstanceID))continue;
        auto Spec=MakeOutgoingSpec(StatusEffectClass,1,MakeEffectContext());FICDefinitionResult R;
        if(Spec.IsValid())if(const auto* Rule=Definitions()?Definitions()->Find<FICStatusEffectRow>(E.EffectID,R):nullptr)
        {Spec.Data->DynamicGrantedTags.AppendTags(Rule->GrantedStateTags);EffectHandles.Add(E.InstanceID,ApplyGameplayEffectSpecToSelf(*Spec.Data));}
    }
    for(auto It=EffectHandles.CreateIterator();It;++It)if(!IDs.Contains(It.Key())){RemoveActiveGameplayEffect(It.Value());It.RemoveCurrent();}
}
bool UICAbilitySystemComponent::RemoveStatus(FGuid ID)
{ IC_TRACE_FUNCTION();const int32 Removed=Runtime.Combat.Effects.RemoveAll([ID](const auto& E){return E.InstanceID==ID;});if(Removed){SyncEffectHandles();RebuildDerivedStats();if(!bRestoring)OnStateChanged.Broadcast();}return Removed>0; }
void UICAbilitySystemComponent::RemoveSleepEffects()
{ IC_TRACE_FUNCTION();if(!IsAlive())return;FICDefinitionResult R;TArray<FGuid> IDs;for(const auto& E:Runtime.Combat.Effects)if(const auto* Rule=Definitions()?Definitions()->Find<FICStatusEffectRow>(E.EffectID,R):nullptr)if(Rule->bRemovedByLivingTargetSleep)IDs.Add(E.InstanceID);for(FGuid ID:IDs)RemoveStatus(ID); }
void UICAbilitySystemComponent::AdvanceEffects(double Now)
{
    IC_TRACE_FUNCTION();FICDefinitionResult R;TArray<FGuid> Expired;

    TArray<FGuid> IDs;for(const auto& E:Runtime.Combat.Effects)IDs.Add(E.InstanceID);
    for(FGuid ID:IDs)
    {
        auto* E=Runtime.Combat.Effects.FindByPredicate([ID](const auto& X){return X.InstanceID==ID;});if(!E)continue;
        const auto* Rule=Definitions()?Definitions()->Find<FICStatusEffectRow>(E->EffectID,R):nullptr;if(!Rule)continue;
        const double Limit=E->bInfinite?Now:FMath::Min(Now,E->ExpiresAtPlaySecond);
        if(Rule->TickIntervalPlaySeconds>0&&(Rule->Kind==EICStatusKind::DamageOverTime||Rule->Kind==EICStatusKind::HealOverTime))
        {
            while(E->NextTickPlaySecond<=Limit)
            {
                FICAttackCapture C;C.Magnitude=E->CapturedMagnitudes.FindRef(TEXT("Magnitude"))*E->Stacks;C.CriticalChance=E->CapturedMagnitudes.FindRef(TEXT("CritChance"));C.CriticalMultiplier=E->CapturedMagnitudes.FindRef(TEXT("CritMultiplier"));C.OutputMultiplier=E->CapturedMagnitudes.Contains(TEXT("Output"))?E->CapturedMagnitudes.FindRef(TEXT("Output")):1;
                E->NextTickPlaySecond+=Rule->TickIntervalPlaySeconds;
                if(Rule->Kind==EICStatusKind::DamageOverTime)
                {
                    ApplyDamage(C,nullptr,Rule->bPeriodicCanCritical,Rule->bPeriodicCanBeGuarded,Rule->bPeriodicCanBeAutoDodged,Rule->bPeriodicCancelsCasting);
                    if(Runtime.Kind==EICCharacterKind::Recruitable&&Runtime.PartySlot==INDEX_NONE&&Combat->GetHealth()<1)SetVitals(1,Combat->GetMana(),0);
                }
                else if(IsAlive())ChangeResource(TEXT("HP"),C.Magnitude);
                E=Runtime.Combat.Effects.FindByPredicate([ID](const auto& X){return X.InstanceID==ID;});if(!E)break;
            }
        }
        if(E&&((!E->bInfinite&&Now>=E->ExpiresAtPlaySecond)||(E->bInfinite&&!Rule->PermanentEndConditions.Conditions.IsEmpty()&&Conditions(Rule->PermanentEndConditions,this))))Expired.Add(ID);
    }
    for(FGuid ID:Expired)RemoveStatus(ID);
    Runtime.Combat.LastCapturedPlaySecond=Now;
}
