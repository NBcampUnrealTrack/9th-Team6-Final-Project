#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Combat/GAS/Abilities/ICGameplayAbility.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Combat/Actors/ICProjectile.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"

FICCombatResult UICAbilitySystemComponent::RequestAction(FName Action,FName SkillID,AICCharacterBase* Target)
{
    IC_TRACE_FUNCTION();FICCombatResult Result;
    if(Action==TEXT("BlockEnd")){SetGuarding(false);Result.bSuccess=true;return Result;}
    if(Action!=TEXT("Attack")&&Action!=TEXT("BlockStart")&&Action!=TEXT("Dodge")&&Action!=TEXT("Skill")){Result.Reason=TEXT("지원하지 않는 전투 요청입니다.");return Result;}
    if(!IsAlive()||HasMatchingGameplayTag(ICTags::Returning)||HasMatchingGameplayTag(ICTags::Staggered)){Result.Reason=TEXT("현재 상태에서는 행동할 수 없습니다.");return Result;}
    if(Action==TEXT("Attack")&&RunningAbility&&RequestedAction==TEXT("Attack"))
    {FICDefinitionResult R;const auto* Settings=Definitions()?Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;BufferedAttackUntil=PlaySecond()+(Settings?Settings->InputBufferPlaySeconds:.2);Result.bSuccess=true;Result.ActionID=CurrentID;return Result;}
    if(RunningAbility)
    {
        if(Action!=TEXT("BlockStart")&&Action!=TEXT("Dodge")){Result.Reason=TEXT("현재 행동이 끝나지 않았습니다.");return Result;}
        if(RequestedAction==TEXT("Skill")&&((Action==TEXT("BlockStart")&&!Skill.CancelRules.bCancelOnGuard)||(Action==TEXT("Dodge")&&!Skill.CancelRules.bCancelOnDodge)))
        {Result.Reason=TEXT("이 스킬은 해당 입력으로 취소할 수 없습니다.");return Result;}
        if(RequestedAction==TEXT("Attack")&&!Skill.Montage.IsNull()&&!bCancelWindow){Result.Reason=TEXT("공격 취소 가능 구간이 아닙니다.");return Result;}
        CancelAction();
    }
    bCommitted=false;RequestedAction=Action;RequestedSkill=SkillID;TargetActor=Target;Skill=FICSkillRow();FICDefinitionResult R;
    if(Action==TEXT("Skill"))
    {const auto* Row=Definitions()?Definitions()->Find<FICSkillRow>(SkillID,R):nullptr;if(!Row){Result.Reason=R.Reason;return Result;}Skill=*Row;}
    if(!ValidateAction(Result.Reason))return Result;
    if(Action==TEXT("BlockStart")){SetGuarding(true);Result.bSuccess=true;return Result;}
    SetGuarding(false);
    if(!AbilityHandle.IsValid()||!TryActivateAbility(AbilityHandle)){Result.Reason=TEXT("GAS 능력 활성화에 실패했습니다.");return Result;}
    Result.bSuccess=CurrentID.IsValid();Result.ActionID=CurrentID;if(!Result.bSuccess)Result.Reason=TEXT("행동 준비에 실패했습니다.");return Result;
}
bool UICAbilitySystemComponent::ValidateAction(FString& Reason) const
{
    IC_TRACE_FUNCTION();Reason.Empty();if(!IsAlive()||!GetAvatarActor()){Reason=TEXT("실행 캐릭터가 준비되지 않았습니다.");return false;}
    if(RequestedAction==TEXT("Dodge")&&PlaySecond()<DodgeReadyAt){Reason=TEXT("회피 재사용 대기 중입니다.");return false;}
    if(RequestedAction==TEXT("BlockStart")||RequestedAction==TEXT("Dodge"))return true;
    auto* D=Definitions();if(!D){Reason=TEXT("정의 카탈로그가 없습니다.");return false;}FICDefinitionResult R;
    if(RequestedAction==TEXT("Skill"))
    {
        if(!bCommitted&&GetSkillCooldownRemaining(Skill.ID)>0){Reason=TEXT("스킬 재사용 대기 중입니다.");return false;}
        if(Primary&&!Runtime.UnlockedSkillIDs.Contains(Skill.ID)){Reason=TEXT("해금되지 않은 스킬입니다.");return false;}
        if(!Combat||Combat->GetMana()<Skill.MPCost){Reason=TEXT("MP가 부족합니다.");return false;}
        auto* Target=TargetActor.Get();auto* ASC=Target?Target->GetICAbilitySystem():nullptr;
        if(Skill.TargetRules.bRequiresTarget&&!ASC){Reason=TEXT("유효한 대상이 없습니다.");return false;}
        if(ASC)
        {
            const bool Down=ASC->Runtime.IsDowned();
            if((Down&&Skill.TargetRules.TargetState==EICCombatTargetState::Living)||(!Down&&Skill.TargetRules.TargetState==EICCombatTargetState::Downed)||(!Down&&!ASC->IsAlive()))
            {Reason=TEXT("스킬 대상의 생존 상태가 맞지 않습니다.");return false;}
            auto* C=Cast<AICCharacterBase>(GetAvatarActor());const bool Hostile=C->IsHostileTo(Target);
            if((Skill.TargetRules.Target==EICCombatTarget::Enemy&&!Hostile)||(Skill.TargetRules.Target==EICCombatTarget::Self&&Target!=C)
                ||((Skill.TargetRules.Target==EICCombatTarget::PartyMember||Skill.TargetRules.Target==EICCombatTarget::LivingParty||Skill.TargetRules.Target==EICCombatTarget::DownedCompanion)&&Hostile))
            {Reason=TEXT("스킬의 아군·적군 대상 규칙이 맞지 않습니다.");return false;}
            if(Target==C&&!Skill.TargetRules.bAllowCaster&&Skill.TargetRules.Target!=EICCombatTarget::Self){Reason=TEXT("시전자 자신에게 사용할 수 없습니다.");return false;}
            if((Skill.TargetRules.Target==EICCombatTarget::PartyMember||Skill.TargetRules.Target==EICCombatTarget::LivingParty||Skill.TargetRules.Target==EICCombatTarget::DownedCompanion)
                && ASC->Runtime.Kind!=EICCharacterKind::Player&&(ASC->Runtime.Kind!=EICCharacterKind::Recruitable||ASC->Runtime.PartySlot<0||ASC->Runtime.Mode!=EICCharacterMode::Companion))
            {Reason=TEXT("파티에 참가한 동료가 아닙니다.");return false;}
            const double Distance=FVector::Distance(GetAvatarActor()->GetActorLocation(),Target->GetActorLocation());
            if(!(Down&&Skill.TargetRules.bIgnoreRangeForDowned)&&(Distance<Skill.TargetRules.MinimumRangeCm||(Skill.TargetRules.MaximumRangeCm>0&&Distance>Skill.TargetRules.MaximumRangeCm)))
            {Reason=TEXT("스킬 사거리 밖입니다.");return false;}
            if(Skill.TargetRules.bRequireLineOfSight&&Target!=C)
            {FHitResult Hit;FCollisionQueryParams Params;Params.AddIgnoredActor(C);Params.AddIgnoredActor(Target);if(GetWorld()->LineTraceSingleByChannel(Hit,C->GetActorLocation(),Target->GetActorLocation(),ECC_Visibility,Params)){Reason=TEXT("대상이 가려져 있습니다.");return false;}}
        }
        if(!Conditions(Skill.TargetRules.Conditions,ASC)){Reason=TEXT("스킬 대상 조건이 맞지 않습니다.");return false;}
        for(const auto& H:Skill.Hits)
        {
            const auto* E=D->Find<FICSkillEffectRow>(H.EffectID,R);
            if(!E||E->Kind==EICSkillEffectKind::Custom){Reason=E?TEXT("사용자 효과 실행기가 등록되지 않았습니다."):R.Reason;return false;}
            for(FName Status:E->AppliedStatusEffectIDs)if(!D->Find<FICStatusEffectRow>(Status,R)){Reason=R.Reason;return false;}
            if(H.Shape==EICHitShape::Projectile&&!H.ProjectileClass.IsNull()) {auto* Type=H.ProjectileClass.LoadSynchronous();if(!Type||!Type->IsChildOf(AICProjectile::StaticClass())){Reason=TEXT("투사체 BP는 ICProjectile을 상속해야 합니다.");return false;}}
        }
        if(Skill.Hits.IsEmpty()){Reason=TEXT("스킬 효과 정의가 비어 있습니다.");return false;}
        if(Skill.MPCost<0||!FMath::IsFinite(Skill.CastPlaySeconds)||Skill.CastPlaySeconds<0||!FMath::IsFinite(Skill.RecoveryPlaySeconds)||Skill.RecoveryPlaySeconds<0||!FMath::IsFinite(Skill.CooldownPlaySeconds)||Skill.CooldownPlaySeconds<0){Reason=TEXT("스킬 비용·시전·쿨타임 값이 올바르지 않습니다.");return false;}
        for(const auto& H:Skill.Hits){const auto* E=D->Find<FICSkillEffectRow>(H.EffectID,R);if(H.RepeatCount<1||H.MaximumTargets<1||!FMath::IsFinite(H.StartOffsetPlaySeconds)||H.StartOffsetPlaySeconds<0||!FMath::IsFinite(H.RepeatIntervalPlaySeconds)||H.RepeatIntervalPlaySeconds<0||(H.RepeatCount>1&&H.RepeatIntervalPlaySeconds<=0)){Reason=TEXT("스킬 타격 횟수·시간이 올바르지 않습니다.");return false;}if(E&&E->Kind==EICSkillEffectKind::Revive){if(!GetWorld()->GetSubsystem<UICSpawnSubsystem>()->CanReviveAtCurrentPlacement(TargetActor.Get())){Reason=TEXT("안전하게 부활할 공간이 없습니다.");return false;}}}
    }
    const bool NeedWeapon=RequestedAction==TEXT("Attack")||(RequestedAction==TEXT("Skill")&&Skill.bRequiresEquippedWeapon);
    if(NeedWeapon&&Primary)
    {
        const auto Items=Runtime.Kind==EICCharacterKind::Player&&GetOwner()->FindComponentByClass<UICInventoryComponent>()?GetOwner()->FindComponentByClass<UICInventoryComponent>()->GetSnapshot().Equipment:Runtime.Equipment;
        bool Found=false;for(const auto& Item:Items)
        {const auto* I=D->Find<FICItemRow>(Item.ItemID,R);const auto* E=I?D->Find<FICEquipmentRow>(I->EquipmentID,R):nullptr;
         if(E&&E->Slot==EPlayerEquipSlot::Weapon&&(RequestedAction!=TEXT("Skill")||Skill.RequiredWeaponFamily==EICWeaponFamily::None||E->WeaponFamily==Skill.RequiredWeaponFamily))Found=true;}
        if(!Found){Reason=TEXT("필요한 무기가 장착되지 않았습니다.");return false;}
    }
    return true;
}
bool UICAbilitySystemComponent::BeginRequestedAction(UICGameplayAbility* Ability)
{
    IC_TRACE_FUNCTION();FString Reason;if(!ValidateAction(Reason))return false;FICDefinitionResult R;
    auto* D=Definitions();auto* Character=Cast<AICCharacterBase>(GetAvatarActor());if(!Character)return false;
    if(RequestedAction==TEXT("Attack")&&Primary)
    {
        const FICClassRow* Class=nullptr;TArray<FName> Classes;if(D)D->GetDefinitionIDs(EICDefinitionType::Class,Classes);
        for(FName ID:Classes)if(const auto* C=D->Find<FICClassRow>(ID,R))
        {
            if(Runtime.Kind==EICCharacterKind::Player)
             {if(C->WeaponFamily==GetEquippedWeaponFamily())Class=C;}
            else if(const auto* NPC=D->Find<FICNPCRow>(Runtime.CharacterId,R))if(C->ID==NPC->ClassID)Class=C;
        }
        if(!Class||Class->NormalAttackStages.Num()!=3){UE_LOG(LogICData, Warning, TEXT("[Combat.MissingClass] ID=%s"),*Runtime.CharacterId.ToString());return false;}
        if(PlaySecond()>ComboReadyUntil)ComboIndex=0;const auto& Stage=Class->NormalAttackStages[ComboIndex];
        Skill.Hits={Stage.Hit};Skill.Montage=Stage.Montage;Skill.CastSection=Stage.MontageSection;
        Skill.RecoveryPlaySeconds=FMath::Max(.3,Stage.Hit.StartOffsetPlaySeconds+Stage.Hit.TraceDurationPlaySeconds+.2);Skill.FirstEffectOffsetPlaySeconds=Stage.Hit.StartOffsetPlaySeconds;

    }
    if(RequestedAction==TEXT("Attack")&&!Primary){FICSkillHitDefinition Hit;Hit.Shape=EICHitShape::Direct;Hit.StartOffsetPlaySeconds=.2;Skill.Hits={Hit};Skill.CastPlaySeconds=.2;Skill.RecoveryPlaySeconds=.6;}
    RunningAbility=Ability;CurrentID=FGuid::NewGuid();StartedAt=PlaySecond();StartLocation=Character->GetActorLocation();bCommitted=false;bCancelWindow=false;ActiveMontageInstanceID=INDEX_NONE;EmittedHits.Reset();HitReceipts.Reset();OpenTraceHits.Reset();LastTraceStart.Reset();LastTraceEnd.Reset();TraceEndsAt.Reset();
    FICCombatSettingsRow Defaults;const auto* Moves=D?D->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;if(!Moves)Moves=&Defaults;
    if(RequestedAction==TEXT("Dodge"))
    {DodgeReadyAt=StartedAt+Moves->DodgeCooldownPlaySeconds;ActionEndsAt=StartedAt+Moves->DodgeInvulnerabilityPlaySeconds;DodgeDirection=Character->GetLastMovementInputVector().GetSafeNormal2D();if(DodgeDirection.IsNearlyZero())DodgeDirection=Character->GetActorForwardVector();SetLooseGameplayTagCount(ICTags::Dodging,1);SetLooseGameplayTagCount(ICTags::Invulnerable,1);}
    else
    {const double Rate=RequestedAction==TEXT("Attack")?FMath::Max(.01,Stat(EICStat::AttackSpeed)):1;ActionEndsAt=StartedAt+FMath::Max(.01,Skill.CastPlaySeconds+Skill.RecoveryPlaySeconds)/Rate;SetLooseGameplayTagCount(RequestedAction==TEXT("Attack")?ICTags::Attacking:ICTags::Casting,1);
     if(auto* Montage=Skill.Montage.LoadSynchronous())if(auto* Anim=Character->GetMesh()->GetAnimInstance()){Anim->Montage_Play(Montage,RequestedAction==TEXT("Attack")?Stat(EICStat::AttackSpeed):1);FOnMontageEnded Ended;const FGuid ActionGUID=CurrentID;Ended.BindWeakLambda(this,[this,ActionGUID](UAnimMontage* EndedMontage,bool Interrupted){if(CurrentID==ActionGUID&&RunningAbility)RunningAbility->Finish(Interrupted);});Anim->Montage_SetEndDelegate(Ended,Montage);if(auto* Instance=Anim->GetActiveInstanceForMontage(Montage))ActiveMontageInstanceID=Instance->GetInstanceID();if(!Skill.CastSection.IsNone())Anim->Montage_JumpToSection(Skill.CastSection,Montage);}}
    UE_LOG(LogICCore, Log, TEXT("[Combat.Start] Entity=%s Action=%s Skill=%s ID=%s"),*Runtime.CharacterId.ToString(),*RequestedAction.ToString(),*RequestedSkill.ToString(),*CurrentID.ToString());
    OnStateChanged.Broadcast();return true;
}
bool UICAbilitySystemComponent::CommitFirstEffect()
{
    IC_TRACE_FUNCTION();if(bCommitted)return true;FString Reason;if(!ValidateAction(Reason))return false;
    if(RequestedAction==TEXT("Skill"))
    {if(Skill.MPCost>0&&!ChangeResource(TEXT("MP"),-Skill.MPCost))return false;Runtime.Combat.SkillReadyAtPlaySecond.Add(Skill.ID,PlaySecond()+Skill.CooldownPlaySeconds);}
    bCommitted=true;MarkCombat();UE_LOG(LogICCore, Verbose,TEXT("[Combat.Commit] ID=%s MP=%d CD=%.2f"),*CurrentID.ToString(),Skill.MPCost,Skill.CooldownPlaySeconds);return true;
}
FGuid UICAbilitySystemComponent::GetActionID() const { IC_TRACE_FUNCTION();return CurrentID; }
void UICAbilitySystemComponent::SetGuarding(bool Guarding)
{ IC_TRACE_FUNCTION();bGuarding=Guarding;SetLooseGameplayTagCount(ICTags::Guarding,Guarding?1:0);if(Guarding)ComboIndex=0;OnStateChanged.Broadcast(); }
void UICAbilitySystemComponent::ClearActionTags()
{ IC_TRACE_FUNCTION();SetLooseGameplayTagCount(ICTags::Attacking,0);SetLooseGameplayTagCount(ICTags::Casting,0);SetLooseGameplayTagCount(ICTags::Dodging,0);SetLooseGameplayTagCount(ICTags::Invulnerable,0); }
void UICAbilitySystemComponent::CancelAction()
{ IC_TRACE_FUNCTION();if(RunningAbility)RunningAbility->Finish(true);else ClearActionTags(); }
void UICAbilitySystemComponent::EndCurrentAction(bool Cancelled)
{
    IC_TRACE_FUNCTION();if(bEnding||!CurrentID.IsValid())return;TGuardValue<bool> Guard(bEnding,true);
    if(Cancelled&&RequestedAction==TEXT("Skill")&&!bCommitted)
        Runtime.Combat.SkillReadyAtPlaySecond.Add(Skill.ID,FMath::Max(Runtime.Combat.SkillReadyAtPlaySecond.FindRef(Skill.ID),PlaySecond()+Skill.BeforeFirstEffectRetryPlaySeconds));
    if(RequestedAction==TEXT("Attack")){FICDefinitionResult R;const auto* Settings=Definitions()?Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;ComboIndex=Cancelled?0:(ComboIndex+1)%3;ComboReadyUntil=PlaySecond()+(Settings?Settings->ComboWindowPlaySeconds:.8);}
    if(Cancelled)if(auto* C=Cast<AICCharacterBase>(GetAvatarActor()))if(auto* Anim=C->GetMesh()->GetAnimInstance())if(auto* Montage=Skill.Montage.Get())Anim->Montage_Stop(.1f,Montage);
    ClearActionTags();OpenTraceHits.Reset();ActiveMontageInstanceID=INDEX_NONE;CurrentID.Invalidate();RunningAbility=nullptr;OnStateChanged.Broadcast();
}
void UICAbilitySystemComponent::AdvanceAction(double Now)
{
    IC_TRACE_FUNCTION();if(!CurrentID.IsValid())return;auto* C=Cast<AICCharacterBase>(GetAvatarActor());if(!C){CancelAction();return;}
    if(RequestedAction==TEXT("Dodge"))
    {FICDefinitionResult R;FICCombatSettingsRow Defaults;const auto* Rules=Definitions()?Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R):nullptr;if(!Rules)Rules=&Defaults;
     const double Alpha=FMath::Clamp((Now-StartedAt)/FMath::Max(.01,Rules->DodgeInvulnerabilityPlaySeconds),0.,1.);C->SetActorLocation(StartLocation+DodgeDirection*Rules->DodgeDistanceCm*Alpha,true);}
    else
    {
        if(RequestedAction==TEXT("Skill")&&!bCommitted){FString Failure;if(!ValidateAction(Failure)){CancelAction();return;}}
        if(RequestedAction==TEXT("Skill")&&!bCommitted&&Skill.CancelRules.bCancelOnUserMovement&&FVector::DistSquared(StartLocation,C->GetActorLocation())>25){CancelAction();return;}
        if(RequestedAction==TEXT("Skill")&&!bCommitted&&Now>=StartedAt+Skill.FirstEffectOffsetPlaySeconds&&Skill.FirstEffectEvent==EICFirstEffectEvent::EffectCommit)
            if(!CommitFirstEffect()){CancelAction();return;}
        int32 Index=0;
        for(const auto& H:Skill.Hits)for(int32 Repeat=0;Repeat<H.RepeatCount;++Repeat,++Index)
            if(Now>=StartedAt+(H.StartOffsetPlaySeconds+Repeat*H.RepeatIntervalPlaySeconds)/(RequestedAction==TEXT("Attack")?FMath::Max(.01,Stat(EICStat::AttackSpeed)):1)&&!EmittedHits.Contains(Index))EmitHit(Index);
    }
    TArray<int32> Traces=OpenTraceHits.Array();for(int32 Index:Traces){if(Now>=TraceEndsAt.FindRef(Index))OpenTraceHits.Remove(Index);else EmitHit(Index,true);}
    if(Now>=ActionEndsAt&&RunningAbility)
    {
        const bool Buffered=RequestedAction==TEXT("Attack")&&BufferedAttackUntil>=Now;RunningAbility->Finish(false);
        if(Buffered){BufferedAttackUntil=0;RequestAction(TEXT("Attack"),NAME_None,TargetActor.Get());}
    }
}
bool UICAbilitySystemComponent::NotifyCombatEvent(FGuid ID,FName Event,int32 HitIndex)
{
    IC_TRACE_FUNCTION();if(!CurrentID.IsValid()||ID!=CurrentID)return false;
    if(Event==TEXT("AN_EffectCommit")||Event==TEXT("EffectCommit"))return CommitFirstEffect();
    if(Event==TEXT("AN_IFrameEnd")||Event==TEXT("IFrameEnd")){SetLooseGameplayTagCount(ICTags::Invulnerable,0);return true;}
    if(Event==TEXT("AN_IFrameStart")||Event==TEXT("IFrameStart")){if(RequestedAction!=TEXT("Dodge"))return false;return true;}
    if(Event==TEXT("AN_HitStart")||Event==TEXT("HitStart")||Event==TEXT("AN_ProjectileFire")||Event==TEXT("ProjectileFire"))return EmitHit(HitIndex);
    if(Event==TEXT("MontageInterrupted")){CancelAction();return true;}
    if(Event==TEXT("AN_HitEnd")||Event==TEXT("HitEnd")){OpenTraceHits.Remove(HitIndex);return true;}
    if(Event==TEXT("AN_CancelWindowStart")){bCancelWindow=true;return true;}if(Event==TEXT("AN_CancelWindowEnd")){bCancelWindow=false;return true;}return false;
}
bool UICAbilitySystemComponent::EmitHit(int32 Index,bool ContinueTrace)
{
    IC_TRACE_FUNCTION();if(Index<0||(!ContinueTrace&&EmittedHits.Contains(Index))||!CurrentID.IsValid())return false;
    const FICSkillHitDefinition* Hit=nullptr;int32 Cursor=0;
    for(const auto& H:Skill.Hits){if(Index>=Cursor&&Index<Cursor+H.RepeatCount){Hit=&H;break;}Cursor+=H.RepeatCount;}
    if(!Hit)return false;EmittedHits.Add(Index);FICDefinitionResult R;FICSkillEffectRow Normal;const FICSkillEffectRow* Effect=nullptr;
    if(RequestedAction==TEXT("Attack")&&!Primary){Normal.Magnitude.Coefficient=1;Normal.Magnitude.SourceStat=EICStat::PhysicalAttack;Normal.Magnitude.bApplyPersonalCHA=false;Effect=&Normal;}
    else if(RequestedAction==TEXT("Attack"))
    {
        TArray<FName> Classes;Definitions()->GetDefinitionIDs(EICDefinitionType::Class,Classes);
        for(FName ID:Classes)if(const auto* Class=Definitions()->Find<FICClassRow>(ID,R))
        {
            bool Match=false;
            if(Runtime.Kind==EICCharacterKind::Player)Match=Class->WeaponFamily==GetEquippedWeaponFamily();
            else if(const auto* NPC=Definitions()->Find<FICNPCRow>(Runtime.CharacterId,R))Match=NPC->ClassID==ID;
            if(Match&&Class->NormalAttackStages.IsValidIndex(ComboIndex)){const auto& S=Class->NormalAttackStages[ComboIndex];Normal.DamageType=Class->NormalAttackDamageType;Normal.Magnitude.SourceStat=Normal.DamageType==EICDamageType::Magical?EICStat::MagicAttack:EICStat::PhysicalAttack;Normal.Magnitude.Coefficient=S.DamageCoefficient;Normal.Magnitude.FlatAmount=S.FixedDamage;Normal.Magnitude.bApplyPersonalCHA=false;Effect=&Normal;break;}
        }
    }
    else Effect=Definitions()?Definitions()->Find<FICSkillEffectRow>(Hit->EffectID,R):nullptr;
    if(!Effect)return false;
    auto* C=Cast<AICCharacterBase>(GetAvatarActor());if(!C)return false;
    if(Hit->Shape==EICHitShape::Projectile)
    {
        if(!CommitFirstEffect())return false;UClass* Type=Hit->ProjectileClass.IsNull()?AICProjectile::StaticClass():Hit->ProjectileClass.LoadSynchronous();if(!Type||!Type->IsChildOf(AICProjectile::StaticClass()))return false;
        FVector Origin=C->GetMesh()->DoesSocketExist(TEXT("muzzle"))?C->GetMesh()->GetSocketLocation(TEXT("muzzle")):C->GetActorLocation()+C->GetActorForwardVector()*50;
        FVector Direction=TargetActor.IsValid()?TargetActor->GetActorLocation()-Origin:C->GetActorForwardVector();
        FHitResult Wall;FCollisionQueryParams Q;Q.AddIgnoredActor(C);if(GetWorld()->LineTraceSingleByChannel(Wall,C->GetActorLocation(),Origin,ECC_Visibility,Q))return true;
        auto* P=GetWorld()->SpawnActorDeferred<AICProjectile>(Type,FTransform(Direction.Rotation(),Origin),C,C,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!P)return false;P->InitializeShot(this,*Effect,*Hit,CaptureAttack(Effect->Magnitude,Effect->DamageType,RequestedAction!=TEXT("Attack")),Direction);P->FinishSpawning(FTransform(Direction.Rotation(),Origin));return true;
    }
    TArray<AICCharacterBase*> Targets;
    if(Hit->Shape==EICHitShape::Direct){if(TargetActor.IsValid())Targets.Add(TargetActor.Get());else if(Skill.TargetRules.Target==EICCombatTarget::Self)Targets.Add(C);}
    else if(Hit->Shape==EICHitShape::WeaponTrace)
    {
        USceneComponent* Start=nullptr;USceneComponent* End=nullptr;TArray<USceneComponent*> Components;C->GetComponents<USceneComponent>(Components);
        for(auto* Component:Components){if(Component->DoesSocketExist(Hit->TraceStartSocket))Start=Component;if(Component->DoesSocketExist(Hit->TraceEndSocket))End=Component;}
        if(!Start||!End){UE_LOG(LogICCore,Warning,TEXT("[Combat.WeaponSocketsMissing] %s Start=%s End=%s"),*C->GetName(),*Hit->TraceStartSocket.ToString(),*Hit->TraceEndSocket.ToString());return false;}
        const FVector A=Start->GetSocketLocation(Hit->TraceStartSocket),B=End->GetSocketLocation(Hit->TraceEndSocket);FCollisionQueryParams Q;Q.AddIgnoredActor(C);FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_Pawn);Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
        auto Sweep=[&](FVector From,FVector To){TArray<FHitResult> Results;GetWorld()->SweepMultiByObjectType(Results,From,To,FQuat::Identity,Objects,FCollisionShape::MakeSphere(FMath::Max(1.,Hit->TraceRadiusCm)),Q);Results.Sort([](const FHitResult& X,const FHitResult& Y){return X.Time<Y.Time;});for(const auto& H:Results){auto* Target=Cast<AICCharacterBase>(H.GetActor());if(Target&&C->IsHostileTo(Target))Targets.AddUnique(Target);else if(!Target&&H.bBlockingHit&&Hit->bStopAtWorldCollision)break;}};
        Sweep(A,B);if(LastTraceStart.Contains(Index)){Sweep(LastTraceStart[Index],A);Sweep(LastTraceEnd[Index],B);}LastTraceStart.Add(Index,A);LastTraceEnd.Add(Index,B);
        if(!ContinueTrace){OpenTraceHits.Add(Index);TraceEndsAt.Add(Index,PlaySecond()+Hit->TraceDurationPlaySeconds);}
    }
    else for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)
    {
        AICCharacterBase* T=*It;auto* ASC=T->GetICAbilitySystem();if(!ASC)continue;
        const bool FriendlyArea=RequestedAction==TEXT("Skill")&&(Skill.TargetRules.Target==EICCombatTarget::LivingParty||Skill.TargetRules.Target==EICCombatTarget::PartyMember||Skill.TargetRules.Target==EICCombatTarget::Self);
        if(FriendlyArea)
        {if(C->IsHostileTo(T)||!ASC->IsAlive()||(T==C&&!Skill.TargetRules.bAllowCaster&&Skill.TargetRules.Target!=EICCombatTarget::Self)
            ||(ASC->Runtime.Kind!=EICCharacterKind::Player&&ASC->Runtime.PartySlot<0))continue;}
        else if(T==C||!C->IsHostileTo(T))continue;
        const FVector Center=Hit->Shape==EICHitShape::GroundArea&&TargetActor.IsValid()?TargetActor->GetActorLocation():C->GetActorLocation();
        const FVector Delta=T->GetActorLocation()-Center;const double Radius=Hit->RadiusCm>0?Hit->RadiusCm:150;
        bool Inside=Delta.SizeSquared()<=FMath::Square(Radius);
        if(Hit->Shape==EICHitShape::Cone)Inside&=FVector::DotProduct(Delta.GetSafeNormal(),C->GetActorForwardVector())>=FMath::Cos(FMath::DegreesToRadians(Hit->ConeAngleDegrees*.5));
        if(Hit->Shape==EICHitShape::Box){const FVector Local=C->GetActorTransform().InverseTransformPosition(T->GetActorLocation());Inside=FMath::Abs(Local.X)<=Hit->BoxHalfExtentCm.X&&FMath::Abs(Local.Y)<=Hit->BoxHalfExtentCm.Y&&FMath::Abs(Local.Z)<=Hit->BoxHalfExtentCm.Z;}
        if(Inside)Targets.Add(T);
    }
    int32 Applied=0;const FString Prefix=FString::Printf(TEXT("%d:"),Index);int32 Previous=0;for(const auto& Receipt:HitReceipts)if(Receipt.StartsWith(Prefix))++Previous;
    for(auto* Target:Targets)
    {
        if(!Target||!Target->GetICAbilitySystem()||Previous+Applied>=Hit->MaximumTargets)continue;
        const FString Receipt=FString::Printf(TEXT("%d:%u"),Index,Target->GetUniqueID());if(HitReceipts.Contains(Receipt))continue;
        FHitResult Wall;FCollisionQueryParams Q;Q.AddIgnoredActor(C);Q.AddIgnoredActor(Target);
        if(Hit->bStopAtWorldCollision&&GetWorld()->LineTraceSingleByChannel(Wall,C->GetActorLocation(),Target->GetActorLocation(),ECC_Visibility,Q))continue;
        if(!CommitFirstEffect())return false;HitReceipts.Add(Receipt);
        ExecuteEffect(*Effect,Target->GetICAbilitySystem());if(++Applied>=Hit->MaximumTargets)break;
    }
    return Applied>0;
}

bool UICAbilitySystemComponent::NotifyFromMontage(int32 InstanceID,FName Event,int32 HitIndex)
{ IC_TRACE_FUNCTION();return InstanceID!=INDEX_NONE&&InstanceID==ActiveMontageInstanceID&&NotifyCombatEvent(CurrentID,Event,HitIndex); }

FICCombatResult UICAbilitySystemComponent::RequestSkillByID(FName SkillID,FName TargetID)
{
    IC_TRACE_FUNCTION();FICCombatResult Result;auto* Spawn=GetWorld()?GetWorld()->GetSubsystem<UICSpawnSubsystem>():nullptr;if(!Spawn){Result.Reason=TEXT("스폰 시스템이 없습니다.");return Result;}
    auto* Target=Spawn->FindCharacter(TargetID);
    if(!Target)for(TActorIterator<AICCharacterBase> It(GetWorld());It;++It)if(It->GetCharacterID()==TargetID){Target=*It;break;}
    FICDefinitionResult R;const auto* Row=Definitions()?Definitions()->Find<FICSkillRow>(SkillID,R):nullptr;if(!Row){Result.Reason=R.Reason;return Result;}
    if(!Target&&Row->TargetRules.TargetState==EICCombatTargetState::Downed)Target=Spawn->ReserveReviveTarget(TargetID,this,Result.Reason);
    if(!Target){if(Result.Reason.IsEmpty())Result.Reason=TEXT("활성 대상이 없습니다.");return Result;}
    return RequestAction(TEXT("Skill"),SkillID,Target);
}

void UICAbilitySystemComponent::NotifyEquipmentChanged()
{ IC_TRACE_FUNCTION();if(CurrentID.IsValid()&&(RequestedAction==TEXT("Attack")||Skill.CancelRules.bCancelOnSuccessfulEquipmentChange))CancelAction(); }
