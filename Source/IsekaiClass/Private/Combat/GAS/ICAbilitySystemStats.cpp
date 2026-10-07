#include "World/State/ICWorldStateSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Combat/GAS/ICGameplayTags.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"

namespace
{
double Curve(double S, double K) { IC_TRACE_FUNCTION(); return FMath::Max(0.,S-1)/(FMath::Max(0.,S-1)+FMath::Max(1.,K)); }
double PrimaryBase(const FICMainStats& S, EICStat Stat)
{ IC_TRACE_FUNCTION(); switch(Stat) { case EICStat::STR:return S.STR;case EICStat::DEX:return S.DEX;case EICStat::INT:return S.INT;case EICStat::CON:return S.CON;case EICStat::CHA:return S.CHA;case EICStat::LUK:return S.LUK;default:return 0; } }
bool Compare(double A, double B, EICComparison Op)
{ IC_TRACE_FUNCTION(); switch(Op) {case EICComparison::Equal:return A==B;case EICComparison::NotEqual:return A!=B;case EICComparison::Greater:return A>B;case EICComparison::GreaterOrEqual:return A>=B;case EICComparison::Less:return A<B;case EICComparison::LessOrEqual:return A<=B;default:return false;} }
}
bool UICAbilitySystemComponent::Conditions(const FICConditionGroup& Group, const UICAbilitySystemComponent* Target, bool BaseOnly) const
{
    IC_TRACE_FUNCTION(); if (Group.Conditions.IsEmpty()) return true;
    const auto* GI = GetWorld()?GetWorld()->GetGameInstance():nullptr; const auto* W = GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    const FICWorldState World = W?W->ExportState():FICWorldState();
    const auto* Subject = Target ? Target : this;
    bool Result = Group.Policy == EICConditionPolicy::All;
    for (const auto& C : Group.Conditions)
    {
        bool Pass = false; double Value = 0; bool Numeric = true;
        switch(C.Type)
        {
            case EICConditionType::None:Pass=true;Numeric=false;break;
            case EICConditionType::Stat:Value=BaseOnly||!C.bUseEffectiveStats?PrimaryBase(Subject->Runtime.MainStats,C.Stat):Subject->Stat(C.Stat);break;
            case EICConditionType::Affinity:if(const auto* S=World.Characters.Find(C.Target.ID))Value=S->Affinity;break;
            case EICConditionType::CurrentHPRatio:Value=Subject->Combat&&Subject->Combat->GetMaxHealth()>0?Subject->Combat->GetHealth()/Subject->Combat->GetMaxHealth():0;break;
            case EICConditionType::CurrentMPRatio:Value=Subject->Combat&&Subject->Combat->GetMaxMana()>0?Subject->Combat->GetMana()/Subject->Combat->GetMaxMana():0;break;
            case EICConditionType::TargetExists:Value=Target!=nullptr;break;
            case EICConditionType::InCombat:Value=Subject->HasRecentCombat();break;
            case EICConditionType::CharacterMode:Value=static_cast<int32>(Subject->Runtime.Mode);break;
            case EICConditionType::Flag:Value=World.Flags.FindRef(C.Flag);break;
            case EICConditionType::BossDefeated:Value=World.DefeatedBosses.Contains(C.Target.ID);break;
            case EICConditionType::VisitedAnchor:Value=World.DiscoveredAnchorIDs.Contains(C.Target.ID);break;
            case EICConditionType::WarpUnlocked:Value=World.UnlockedWarps.Contains(C.Target.ID);break;
            case EICConditionType::RecipeUnlocked:Value=World.UnlockedRecipeIDs.Contains(C.Target.ID);break;
            case EICConditionType::HasGameplayTag:Pass=Subject->HasAllMatchingGameplayTags(C.RequiredTags);Numeric=false;break;
            case EICConditionType::HasStatusEffect:Value=Subject->Runtime.Combat.Effects.ContainsByPredicate([&C](const auto& E){return E.EffectID==C.Target.ID;});break;
            case EICConditionType::ItemInBag:for(const auto& I:World.Inventory.Bag.Slots)if(I.ItemID==C.Target.ID)Value+=I.Count;break;
            case EICConditionType::QuestAccepted:case EICConditionType::QuestCompleted: { const auto* Q=World.Quests.Find(C.Target.ID); Value=Q&&(C.Type==EICConditionType::QuestAccepted?Q->bAccepted:Q->bCompleted);break; }
            default:return false;
        }
        if (Numeric) Pass=Compare(Value,C.Value,C.Compare); if(C.bNegate)Pass=!Pass;
        if(Group.Policy==EICConditionPolicy::All)Result&=Pass;else Result|=Pass;
    }
    return Result;
}
void UICAbilitySystemComponent::RebuildDerivedStats()
{
    IC_TRACE_FUNCTION(); if (!Combat || !bReady) return;
    FICDefinitionResult Result; FICStatSettingsRow Defaults; FICCombatSettingsRow MovementDefaults;
    const auto* Rules=Definitions()?Definitions()->Find<FICStatSettingsRow>(TEXT("CFG_STAT_001"),Result):nullptr;if(!Rules)Rules=&Defaults;
    const auto* Moves=Definitions()?Definitions()->Find<FICCombatSettingsRow>(TEXT("CFG_COMBAT_001"),Result):nullptr;if(!Moves)Moves=&MovementDefaults;
    TArray<FICStatModifier> Mods;
    if(auto* Inventory=GetOwner()->FindComponentByClass<UICInventoryComponent>())Mods=Inventory->GetEquipmentStats().Modifiers;
    else if(Definitions()&&!Runtime.Equipment.IsEmpty())
    {
        auto* Staging=NewObject<UICInventoryComponent>(GetTransientPackage());FICDefinitionResult Lookup;const auto* NPC=Definitions()->Find<FICNPCRow>(Runtime.CharacterId,Lookup);const auto* Class=NPC?Definitions()->Find<FICClassRow>(NPC->ClassID,Lookup):nullptr;Staging->BeginStaging(Definitions(),Class?static_cast<int32>(Class->CharacterClass)-1:0);
        FICInventoryState I;I.Bag.Slots.SetNum(20);I.Equipment=Runtime.Equipment;
        if(Staging->ApplySnapshot(I))Mods=Staging->GetEquipmentStats().Modifiers;
    }
    for(const auto& E:Runtime.Combat.Effects)if(const auto* R=Definitions()?Definitions()->Find<FICStatusEffectRow>(E.EffectID,Result):nullptr)
        for(auto M:R->StatModifiers){const double Scale=E.CapturedMagnitudes.Contains(TEXT("PersonalCHA"))?E.CapturedMagnitudes.FindRef(TEXT("PersonalCHA")):1.;M.Flat*=Scale*E.Stacks;M.Percent*=Scale*E.Stacks;Mods.Add(M);}
    // Strongest active combination per stat; membership includes Downed companions.
    auto* W=GetWorld()&&GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    FICWorldState World=W?W->ExportState():FICWorldState();TArray<FName> Synergies;TMap<EICStat,TArray<FICStatModifier>> SynergyMods;
    if(Definitions()&&Runtime.HasStats()&&!Runtime.IsDowned()&& (Runtime.Kind==EICCharacterKind::Player||Runtime.PartySlot>=0))
    {
        Definitions()->GetDefinitionIDs(EICDefinitionType::Synergy,Synergies);
        for(FName ID:Synergies)if(const auto* R=Definitions()->Find<FICSynergyRow>(ID,Result))
        {
            bool Match=Conditions(R->ActivationConditions);
            for(FName Member:R->MemberNPCIDs){const auto* S=World.Characters.Find(Member);Match&=S&&S->PartySlot>=0;}
            if(Match)for(const auto& M:R->StatModifiers)SynergyMods.FindOrAdd(M.Stat).Add(M);
        }

    }
    auto Modified=[&](EICStat S,double Base){IC_TRACE_FUNCTION();double Flat=0,Percent=0;for(const auto& M:Mods)if(M.Stat==S){Flat+=M.Flat;Percent+=M.Percent;}double Value=(Base+Flat)*(1+Percent);if(const auto* Candidates=SynergyMods.Find(S)){double Best=-DBL_MAX;for(const auto& M:*Candidates)Best=FMath::Max(Best,(Base+Flat+M.Flat)*(1+Percent+M.Percent));if(Best>-DBL_MAX)Value=Best;}return Value;};
    double STR=1,DEX=1,INT=1,CON=1,CHA=1,LUK=1;
    if(Primary)
    {
        auto SetPrimary=[&](EICStat S,FGameplayAttribute A){IC_TRACE_FUNCTION();const double V=FMath::RoundToDouble(FMath::Clamp(Modified(S,PrimaryBase(Runtime.MainStats,S)),1.,double(Rules->MainStatCap)));SetNumericAttributeBase(A,V);return V;};
        STR=SetPrimary(EICStat::STR,UICPrimaryAttributeSet::GetSTRAttribute());DEX=SetPrimary(EICStat::DEX,UICPrimaryAttributeSet::GetDEXAttribute());
        INT=SetPrimary(EICStat::INT,UICPrimaryAttributeSet::GetINTAttribute());CON=SetPrimary(EICStat::CON,UICPrimaryAttributeSet::GetCONAttribute());
        CHA=SetPrimary(EICStat::CHA,UICPrimaryAttributeSet::GetCHAAttribute());LUK=SetPrimary(EICStat::LUK,UICPrimaryAttributeSet::GetLUKAttribute());
    }
    double BaseHP=Rules->BaseMaxHP+Rules->HPPerCON*(CON-1),BaseMP=Rules->BaseMaxMP+Rules->MPPerINT*(INT-1),BaseSP=Rules->BaseMaxSP+Rules->SPPerCON*(CON-1);
    // Player creation values are an explicit project contract. Growth uses configured increments from this baseline.
    if(Runtime.Kind==EICCharacterKind::Player)if(const auto* New=Definitions()?Definitions()->Find<FICNewGameRow>(TEXT("CFG_NEWGAME_001"),Result):nullptr)
    {BaseHP=New->InitialMaxHP+Rules->HPPerCON*(CON-New->InitialStats.CON);BaseMP=New->InitialMaxMP+Rules->MPPerINT*(INT-New->InitialStats.INT);BaseSP=New->InitialMaxSP+Rules->SPPerCON*(CON-New->InitialStats.CON);}
    double CompanionCHA=1;
    if(Runtime.Kind==EICCharacterKind::Recruitable&&Runtime.PartySlot>=0)
    {
        const auto* P=World.Characters.Find(UICWorldStateSubsystem::PlayerId());
        double EffectiveCHA=P?P->MainStats.CHA:1;
        if(P)if(auto* Player=Cast<AICCharacterBase>(UGameplayStatics::GetPlayerPawn(GetWorld(),0)))if(auto* ASC=Player->GetICAbilitySystem())EffectiveCHA=ASC->Stat(EICStat::CHA);
        CompanionCHA+=Rules->CHABonus*Curve(EffectiveCHA,Rules->CHACurveK);
    }
    auto Base=[&](EICStat S,double V){IC_TRACE_FUNCTION();return Primary?V:MonsterBase.FindRef(S);};
    auto Final=[&](EICStat S,double V,bool Round){IC_TRACE_FUNCTION();double N=Modified(S,Base(S,V));if(Rules->CompanionCHAAffectedStats.Contains(S))N*=CompanionCHA;N=FMath::Max(0.,N);return Round?FMath::RoundToDouble(N):N;};
    const float HP=Combat->GetHealth(),MP=Combat->GetMana(),SP=Survival?Survival->GetSP():0;
    SetNumericAttributeBase(UICCombatAttributeSet::GetMaxHealthAttribute(),FMath::Max(1.,Final(EICStat::MaxHP,BaseHP,true)));
    SetNumericAttributeBase(UICCombatAttributeSet::GetMaxManaAttribute(),Final(EICStat::MaxMP,BaseMP,true));
    if(Survival)SetNumericAttributeBase(UICSurvivalAttributeSet::GetMaxSPAttribute(),Final(EICStat::MaxSP,BaseSP,true));
    SetNumericAttributeBase(UICCombatAttributeSet::GetPhysicalAttackAttribute(),Final(EICStat::PhysicalAttack,Rules->BaseAttack+Rules->AttackPerMainStat*(STR-1),true));
    SetNumericAttributeBase(UICCombatAttributeSet::GetMagicAttackAttribute(),Final(EICStat::MagicAttack,Rules->BaseAttack+Rules->AttackPerMainStat*(INT-1),true));
    SetNumericAttributeBase(UICCombatAttributeSet::GetDefenseAttribute(),Final(EICStat::Defense,Rules->BaseDefense+Rules->DefensePerCON*(CON-1),true));
    SetNumericAttributeBase(UICCombatAttributeSet::GetAttackSpeedAttribute(),FMath::Min(Rules->AttackSpeedMultiplierCap,Final(EICStat::AttackSpeed,1+Rules->DEXAttackSpeedBonus*Curve(DEX,Rules->DEXSpeedCurveK),false)));
    SetNumericAttributeBase(UICCombatAttributeSet::GetMoveSpeedAttribute(),FMath::Min(Moves->RunSpeedCmPerSecond*Rules->MoveSpeedMultiplierCap,Final(EICStat::MoveSpeed,Moves->RunSpeedCmPerSecond*(1+Rules->DEXMoveSpeedBonus*Curve(DEX,Rules->DEXSpeedCurveK)),false)));
    SetNumericAttributeBase(UICCombatAttributeSet::GetCriticalChanceAttribute(),FMath::Min(Rules->CriticalChanceCap,Final(EICStat::CriticalChance,Rules->BaseCriticalChance+Rules->DEXCriticalBonus*Curve(DEX,Rules->DEXCriticalCurveK),false)));
    SetNumericAttributeBase(UICCombatAttributeSet::GetCriticalMultiplierAttribute(),FMath::Min(Rules->CriticalMultiplierCap,Final(EICStat::CriticalMultiplier,Rules->BaseCriticalMultiplier+Rules->LUKCriticalBonus*Curve(LUK,Rules->LUKCurveK),false)));
    SetNumericAttributeBase(UICCombatAttributeSet::GetAutoDodgeChanceAttribute(),FMath::Min(Rules->AutoDodgeChanceCap,Final(EICStat::AutoDodgeChance,Rules->DEXDodgeBonus*Curve(DEX,Rules->DEXDodgeCurveK),false)));
    SetVitals(Runtime.IsDowned()?0:HP,MP,SP);UnlockSkills();
}
void UICAbilitySystemComponent::UnlockSkills()
{
    IC_TRACE_FUNCTION(); if(!Primary||!Definitions())return;TArray<FName> IDs;FICDefinitionResult R;Definitions()->GetDefinitionIDs(EICDefinitionType::Skill,IDs);
    for(int32 Pass=0;Pass<4;++Pass)for(FName ID:IDs)if(const auto* S=Definitions()->Find<FICSkillRow>(ID,R))
    {
        bool Owner= S->OwnerType==EICSkillOwner::RecruitUnique ? S->NPCID==Runtime.CharacterId : false;
        if(S->OwnerType==EICSkillOwner::CharacterClass)if(const auto* C=Definitions()->Find<FICClassRow>(S->ClassID,R))
        { if(Runtime.Kind==EICCharacterKind::Player)Owner=static_cast<int32>(C->CharacterClass)-1==GetWorld()->GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>()->ExportState().Job;
          else if(const auto* NPC=Definitions()->Find<FICNPCRow>(Runtime.CharacterId,R))Owner=NPC->ClassID==S->ClassID; }
        if(Owner&&(S->bInitiallyUnlocked||((S->PreviousSkillID.IsNone()||Runtime.UnlockedSkillIDs.Contains(S->PreviousSkillID))&&Conditions(S->UnlockConditions,nullptr,true))))Runtime.UnlockedSkillIDs.Add(ID);
    }
}
