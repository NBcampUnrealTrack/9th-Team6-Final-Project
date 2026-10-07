#include "Combat/GAS/Simulation/ICCombatSimulation.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Debug/ICLog.h"
namespace
{
TArray<FICStatModifier> Modifiers(const FICCharacterState& C,const FICWorldState& World,UICDefinitionSubsystem* D,double At)
{
    IC_TRACE_FUNCTION();TArray<FICStatModifier> Mods;FICDefinitionResult R;
    const auto& Equipment=C.Kind==EICCharacterKind::Player?World.Inventory.Equipment:C.Equipment;
    if(!Equipment.IsEmpty())
    {
        const auto* NPC=D->Find<FICNPCRow>(C.CharacterId,R);const auto* Class=NPC?D->Find<FICClassRow>(NPC->ClassID,R):nullptr;
        auto* Staging=NewObject<UICInventoryComponent>(GetTransientPackage());Staging->BeginStaging(D,Class?static_cast<int32>(Class->CharacterClass)-1:World.Job);auto I=Staging->GetSnapshot();I.Equipment=Equipment;if(Staging->ApplySnapshot(I))Mods=Staging->GetEquipmentStats().Modifiers;
    }
    for(const auto& E:C.Combat.Effects)if(E.bInfinite||E.ExpiresAtPlaySecond>=At)if(const auto* Rule=D->Find<FICStatusEffectRow>(E.EffectID,R))for(auto M:Rule->StatModifiers)
    {const double Scale=E.CapturedMagnitudes.Contains(TEXT("PersonalCHA"))?E.CapturedMagnitudes.FindRef(TEXT("PersonalCHA")):1;M.Flat*=Scale*E.Stacks;M.Percent*=Scale*E.Stacks;Mods.Add(M);}
    return Mods;
}
double Modified(EICStat Stat,double Base,const TArray<FICStatModifier>& Mods)
{ IC_TRACE_FUNCTION();double Flat=0,Percent=0;for(const auto& M:Mods)if(M.Stat==Stat){Flat+=M.Flat;Percent+=M.Percent;}return FMath::Max(0.,(Base+Flat)*(1+Percent)); }
void Rebuild(FICCharacterState& C,const FICWorldState& World,UICDefinitionSubsystem* D,double At,double& Defense)
{
    IC_TRACE_FUNCTION();FICDefinitionResult R;FICStatSettingsRow Defaults;const auto* Rule=D->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),R);if(!Rule)Rule=&Defaults;
    const auto Mods=Modifiers(C,World,D,At);const double CON=FMath::Clamp(FMath::RoundToDouble(Modified(EICStat::CON,C.MainStats.CON,Mods)),1.,double(Rule->MainStatCap));const double INT=FMath::Clamp(FMath::RoundToDouble(Modified(EICStat::INT,C.MainStats.INT,Mods)),1.,double(Rule->MainStatCap));
    double Bonus=1;if(C.PartySlot>=0)if(const auto* P=World.Characters.Find(TEXT("CHR_PLAYER"))){const auto PMods=Modifiers(*P,World,D,At);const double CHA=FMath::Max(1.,Modified(EICStat::CHA,P->MainStats.CHA,PMods));Bonus+=Rule->CHABonus*(CHA-1)/(CHA-1+Rule->CHACurveK);}
    C.MaxHP=FMath::Max(1.,FMath::RoundToDouble(Modified(EICStat::MaxHP,Rule->BaseMaxHP+Rule->HPPerCON*(CON-1),Mods)*(Rule->CompanionCHAAffectedStats.Contains(EICStat::MaxHP)?Bonus:1.)));
    C.MaxMP=FMath::RoundToDouble(Modified(EICStat::MaxMP,Rule->BaseMaxMP+Rule->MPPerINT*(INT-1),Mods)*(Rule->CompanionCHAAffectedStats.Contains(EICStat::MaxMP)?Bonus:1.));
    Defense=FMath::RoundToDouble(Modified(EICStat::Defense,Rule->BaseDefense+Rule->DefensePerCON*(CON-1),Mods)*(Rule->CompanionCHAAffectedStats.Contains(EICStat::Defense)?Bonus:1.));C.HP=FMath::Min(C.HP,C.MaxHP);C.MP=FMath::Min(C.MP,C.MaxMP);
}
}
bool ICCombatSimulation::Advance(FICCharacterState& C,const FICWorldState& World,UICDefinitionSubsystem* D,double Now,double GameMinute)
{
    IC_TRACE_FUNCTION();if(!D||!FMath::IsFinite(Now)||Now<C.LastSimPlaySecond)return false;FICDefinitionResult R;
    for(const auto& E:C.Combat.Effects)if(!D->Find<FICStatusEffectRow>(E.EffectID,R))return false;
    if(C.Kind==EICCharacterKind::Recruitable)
    {
        FICCombatSettingsRow Defaults;const auto* Combat=D->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),R);if(!Combat)Combat=&Defaults;
        while(true)
        {
            int32 Index=INDEX_NONE;double At=DBL_MAX;
            for(int32 I=0;I<C.Combat.Effects.Num();++I){const auto& E=C.Combat.Effects[I];const auto* Rule=D->Find<FICStatusEffectRow>(E.EffectID,R);
                if(Rule&&Rule->TickIntervalPlaySeconds>0&&(Rule->Kind==EICStatusKind::DamageOverTime||Rule->Kind==EICStatusKind::HealOverTime)&&E.NextTickPlaySecond<=Now&&(E.bInfinite||E.NextTickPlaySecond<=E.ExpiresAtPlaySecond)&&E.NextTickPlaySecond<At){At=E.NextTickPlaySecond;Index=I;}}
            if(Index==INDEX_NONE)break;auto& E=C.Combat.Effects[Index];const auto* Rule=D->Find<FICStatusEffectRow>(E.EffectID,R);E.NextTickPlaySecond+=Rule->TickIntervalPlaySeconds;if(C.IsDowned())continue;
            double Defense=0;Rebuild(C,World,D,At,Defense);double Received=1;bool Immune=false;
            for(const auto& Active:C.Combat.Effects)if(Active.bInfinite||Active.ExpiresAtPlaySecond>=At)if(const auto* AR=D->Find<FICStatusEffectRow>(Active.EffectID,R)){Received*=Rule->Kind==EICStatusKind::DamageOverTime?AR->DamageReceivedMultiplier:AR->HealingReceivedMultiplier;Immune|=AR->GrantedStateTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("IC.State.Invulnerable"),false));}
            double Amount=FMath::Max(0.,E.CapturedMagnitudes.FindRef(TEXT("Magnitude"))*E.Stacks)*Received;
            if(Rule->Kind==EICStatusKind::DamageOverTime)
            {
                if(Immune)continue;Amount*=Combat->DefenseFormulaK/(Combat->DefenseFormulaK+Defense);Amount*=E.CapturedMagnitudes.Contains(TEXT("Output"))?E.CapturedMagnitudes.FindRef(TEXT("Output")):1;
                FRandomStream Random(HashCombine(GetTypeHash(E.InstanceID),GetTypeHash(At)));if(Rule->bPeriodicCanCritical&&Random.FRand()<E.CapturedMagnitudes.FindRef(TEXT("CritChance")))Amount*=E.CapturedMagnitudes.FindRef(TEXT("CritMultiplier"));Amount=Amount>0?FMath::Max(1.,FMath::RoundToDouble(Amount)):0;
                TArray<int32> Shields;for(int32 I=0;I<C.Combat.Effects.Num();++I)if(C.Combat.Effects[I].RemainingShield>0&&(C.Combat.Effects[I].bInfinite||C.Combat.Effects[I].ExpiresAtPlaySecond>=At))Shields.Add(I);
                Shields.Sort([&C](int32 A,int32 B){return (C.Combat.Effects[A].bInfinite?DBL_MAX:C.Combat.Effects[A].ExpiresAtPlaySecond)<(C.Combat.Effects[B].bInfinite?DBL_MAX:C.Combat.Effects[B].ExpiresAtPlaySecond);});
                for(int32 I:Shields){const double Used=FMath::Min(C.Combat.Effects[I].RemainingShield,Amount);C.Combat.Effects[I].RemainingShield-=Used;Amount-=Used;}
                C.HP=FMath::Max(C.PartySlot>=0?0.f:1.f,C.HP-float(Amount));if(C.HP<=0)C.LifeState=EICCharacterLifeState::Downed;
            }
            else C.HP=FMath::Min(C.MaxHP,C.HP+float(FMath::RoundToDouble(Amount)));
        }
        C.Combat.Effects.RemoveAll([Now](const auto& E){return !E.bInfinite&&E.ExpiresAtPlaySecond<=Now;});double Defense=0;Rebuild(C,World,D,Now,Defense);
    }
    if(C.Mode==EICCharacterMode::Scheduled)
    {
        const auto* NPC=D->Find<FICNPCRow>(C.CharacterId,R);const auto* Schedule=NPC?D->Find<FICScheduleRow>(NPC->ScheduleID,R):nullptr;const double Minute=FMath::Fmod(GameMinute,1440.);
        if(Schedule)for(const auto& Segment:Schedule->Segments)if(Minute>=Segment.StartGameMinute&&Minute<Segment.EndGameMinute)
        {const auto* End=D->Find<FICAnchorRow>(Segment.DestinationAnchorID,R);if(End){C.CurrentAnchorID=End->ID;C.Position=End->WorldTransform.GetLocation();C.Rotation=End->WorldTransform.Rotator();if(Segment.Activity==EICScheduleActivity::Move)if(const auto* Start=D->Find<FICAnchorRow>(Segment.DepartureAnchorID,R))C.Position=FMath::Lerp(Start->WorldTransform.GetLocation(),C.Position,(Minute-Segment.StartGameMinute)/FMath::Max(1,Segment.EndGameMinute-Segment.StartGameMinute));}break;}
    }
    C.LastSimPlaySecond=C.Combat.LastCapturedPlaySecond=Now;C.LastSimGameMinute=GameMinute;return true;
}
bool ICCombatSimulation::PrepareItemStatus(FICCharacterState& C,FName ID,UICDefinitionSubsystem* D,FString& Reason)
{
    IC_TRACE_FUNCTION();FICDefinitionResult R;const auto* Rule=D?D->Find<FICStatusEffectRow>(ID,R):nullptr;Reason.Empty();if(!Rule){Reason=TEXT("상태 효과 정의가 없습니다.");return false;}
    if(Rule->Kind==EICStatusKind::Custom||(!Rule->bPermanent&&(!FMath::IsFinite(Rule->DurationPlaySeconds)||Rule->DurationPlaySeconds<=0))){Reason=TEXT("상태 효과 종류·지속 시간이 올바르지 않습니다.");return false;}
    if((C.IsDowned()&&Rule->TargetState==EICCombatTargetState::Living)||(!C.IsDowned()&&Rule->TargetState==EICCombatTargetState::Downed)||Rule->TargetSide==EICEffectTargetSide::Hostile){Reason=TEXT("아이템 상태 효과의 대상 규칙이 맞지 않습니다.");return false;}
    FGameplayTagContainer Tags;
    for(const auto& E:C.Combat.Effects)if(const auto* Active=D->Find<FICStatusEffectRow>(E.EffectID,R))
    {Tags.AppendTags(Active->GrantedStateTags);if(Active->GrantedStatusImmunityIDs.Contains(ID)||Active->GrantedStatusImmunityTags.HasAny(Rule->Tags)){Reason=TEXT("이 상태 효과에 면역인 대상입니다.");return false;}}
    if(Tags.HasAny(Rule->BlockedByTargetTags)||(Rule->bBlockedByInvulnerability&&Tags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("IC.State.Invulnerable"),false)))){Reason=TEXT("현재 상태에서는 이 효과를 적용할 수 없습니다.");return false;}
    // Item preparation has no transient targeting/casting context. Refuse such conditions explicitly.
    if(!Rule->ApplicationConditions.Conditions.IsEmpty()){Reason=TEXT("조건부 상태 효과 아이템은 별도의 사용 조건 실행 경로가 필요합니다.");return false;}
    FICActiveEffectState E;E.InstanceID=FGuid::NewGuid();E.EffectID=ID;E.SourceEntityID=C.CharacterId;E.bInfinite=Rule->bPermanent;E.bHarmful=Rule->bHarmful;E.ExpiresAtPlaySecond=C.LastSimPlaySecond+Rule->DurationPlaySeconds;E.NextTickPlaySecond=C.LastSimPlaySecond+(Rule->bTickImmediatelyOnApplication?0:Rule->TickIntervalPlaySeconds);E.CapturedMagnitudes.Add(TEXT("Strength"),Rule->Strength);E.CapturedMagnitudes.Add(TEXT("PersonalCHA"),1);E.CapturedMagnitudes.Add(TEXT("Output"),1);
    auto Magnitude=[&](const FICCombatMagnitude& M)
    {IC_TRACE_FUNCTION();double Stat=0;FICStatSettingsRow Defaults;const auto* S=D->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),R);if(!S)S=&Defaults;switch(M.SourceStat){case EICStat::PhysicalAttack:Stat=S->BaseAttack+S->AttackPerMainStat*(C.MainStats.STR-1);break;case EICStat::MagicAttack:Stat=S->BaseAttack+S->AttackPerMainStat*(C.MainStats.INT-1);break;case EICStat::Defense:Stat=S->BaseDefense+S->DefensePerCON*(C.MainStats.CON-1);break;case EICStat::MaxHP:Stat=C.MaxHP;break;case EICStat::MaxMP:Stat=C.MaxMP;break;case EICStat::MaxSP:Stat=C.MaxSP;break;default:if(M.Coefficient!=0)return -1.;}
     return FMath::Max(0.,Stat*M.Coefficient+M.FlatAmount);};
    if(Rule->Kind==EICStatusKind::Shield){E.RemainingShield=Magnitude(Rule->InitialShieldMagnitude);if(E.RemainingShield<0){Reason=TEXT("아이템 보호막의 기준 능력치를 계산할 수 없습니다.");return false;}}
    if(Rule->Kind==EICStatusKind::DamageOverTime||Rule->Kind==EICStatusKind::HealOverTime){const double Amount=Magnitude(Rule->PeriodicMagnitude);if(Amount<0||!FMath::IsFinite(Rule->TickIntervalPlaySeconds)||Rule->TickIntervalPlaySeconds<=0){Reason=TEXT("주기 효과의 기준 능력치·주기가 올바르지 않습니다.");return false;}E.CapturedMagnitudes.Add(TEXT("Magnitude"),Amount);}
    auto* Existing=C.Combat.Effects.FindByPredicate([ID](const auto& X){return X.EffectID==ID;});
    if(Existing&&Rule->StackingPolicy!=EICStackingPolicy::Independent)
    {if(Rule->StackingPolicy==EICStackingPolicy::KeepStrongerRefreshDuration&&Existing->CapturedMagnitudes.FindRef(TEXT("Strength"))>Rule->Strength){Existing->ExpiresAtPlaySecond=E.ExpiresAtPlaySecond;Existing->RemainingShield=FMath::Max(Existing->RemainingShield,E.RemainingShield);return true;}
     if(Rule->StackingPolicy==EICStackingPolicy::AddStacks)E.Stacks=FMath::Min(Rule->MaximumStacks,Existing->Stacks+1);E.InstanceID=Existing->InstanceID;E.RemainingShield=FMath::Max(Existing->RemainingShield,E.RemainingShield);*Existing=E;}
    else C.Combat.Effects.Add(E);return true;
}
