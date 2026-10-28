#include "Combat/Party/ICPartyStatus.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/Combat/ICSynergyRow.h"
#include "Data/Rows/Combat/ICStatusEffectRow.h"
#include "Data/Text/ICLocalization.h"
#include "World/State/ICWorldStateSubsystem.h"

namespace
{
FText Name(const FICDefinitionRow& R){return R.NameKey.IsEmpty()?FText::FromName(R.ID):ICLocalization::Localize(FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),R.NameKey));}
FText Modifiers(const TArray<FICStatModifier>& Mods)
{
    TArray<FText> Lines;
    for(const auto& M:Mods)
    {
        FText Stat=FText::FromString(StaticEnum<EICStat>()->GetNameStringByValue(int64(M.Stat)));
        if(M.Stat==EICStat::PhysicalAttack)Stat=NSLOCTEXT("ICPartyStatus","Physical","물리 공격력");
        if(M.Stat==EICStat::MagicAttack)Stat=NSLOCTEXT("ICPartyStatus","Magical","마법 공격력");
        if(M.Flat!=0)Lines.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Flat","{0} {1}{2}"),Stat,FText::FromString(M.Flat>0?TEXT("+"):TEXT("")),FText::AsNumber(M.Flat)));
        if(M.Percent!=0)Lines.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Percent","{0} {1}{2}%"),Stat,FText::FromString(M.Percent>0?TEXT("+"):TEXT("")),FText::AsNumber(M.Percent*100)));
    }
    return FText::Join(FText::FromString(TEXT(" / ")),Lines);
}
bool LivingMember(const FICCharacterState& S){return !S.IsDowned()&&S.HP>0&&(S.Kind==EICCharacterKind::Player||(S.Kind==EICCharacterKind::Recruitable&&S.Mode==EICCharacterMode::Companion&&S.PartySlot>=0));}
}
TArray<const FICSynergyRow*> ICPartyStatus::ActiveSynergies(const FICWorldState& W,UICDefinitionSubsystem* D,const UICAbilitySystemComponent* Recipient)
{
    TArray<const FICSynergyRow*> Result;if(!D)return Result;TArray<FName> IDs;D->GetDefinitionIDs(EICDefinitionType::Synergy,IDs);IDs.Sort(FNameLexicalLess());FICDefinitionResult Lookup;
    for(FName ID:IDs)if(const auto* R=D->Find<FICSynergyRow>(ID,Lookup);R&&!R->bDeprecated)
    {
        if(!R->ActivationConditions.Conditions.IsEmpty()&&(!Recipient||!Recipient->Conditions(R->ActivationConditions)))continue;
        int32 Count=0;
        for(const auto& Pair:W.Characters)if(LivingMember(Pair.Value))
        {
            const bool Player=Pair.Value.Kind==EICCharacterKind::Player;if(Player&&!R->bCountPlayer)continue;
            if(R->RequiredWeaponFamily!=EICWeaponFamily::None)
            {
                EICWeaponFamily Family=EICWeaponFamily::None;
                if(Player)
                {
                    Family=EICWeaponFamily(FMath::Clamp(W.Job+1,1,4));
                    if(!W.Inventory.Equipment.IsEmpty())if(const auto* Item=D->Find<FICItemRow>(W.Inventory.Equipment[0].ItemID,Lookup))if(const auto* Gear=D->Find<FICEquipmentRow>(Item->EquipmentID,Lookup))Family=Gear->WeaponFamily;
                }
                else if(const auto* NPC=D->Find<FICNPCRow>(Pair.Key,Lookup))if(const auto* C=D->Find<FICClassRow>(NPC->ClassID,Lookup))Family=C->WeaponFamily;
                if(Family==R->RequiredWeaponFamily)++Count;
            }
            else if(R->MemberNPCIDs.Contains(Pair.Key))++Count;
        }
        const int32 Required=R->MinimumMembers>0?R->MinimumMembers:R->MemberNPCIDs.Num();if(Required<=0||Count<Required)continue;
        if(!R->ExclusiveGroup.IsNone())
        {
            const int32 Existing=Result.IndexOfByPredicate([&](const auto* V){return V->ExclusiveGroup==R->ExclusiveGroup;});
            if(Existing!=INDEX_NONE){if(Result[Existing]->Priority<R->Priority)Result[Existing]=R;continue;}
        }
        Result.Add(R);
    }
    return Result;
}
uint32 ICPartyStatus::CompositionKey(const FICWorldState& W)
{
    uint32 H=GetTypeHash(W.Job);
    if(!W.Inventory.Equipment.IsEmpty())H=HashCombine(H,GetTypeHash(W.Inventory.Equipment[0].ItemID));
    for(const auto& P:W.Characters)if(P.Value.Kind==EICCharacterKind::Player||P.Value.PartySlot>=0)
    {H=HashCombine(H,GetTypeHash(P.Key));H=HashCombine(H,GetTypeHash(P.Value.PartySlot));H=HashCombine(H,GetTypeHash(LivingMember(P.Value)));}
    return H;
}
bool ICPartyStatus::HasBuff(const FICCharacterState& S,const FICWorldState& W,UICDefinitionSubsystem* D)
{
    if(!LivingMember(S))return false;
    const auto* Pet=W.Codex.CharacterPets.Find(S.CharacterId);
    if(!ActiveSynergies(W,D).IsEmpty()||(Pet&&W.Codex.PetEssence.FindRef(*Pet)>=100))return true;
    return S.Combat.Effects.ContainsByPredicate([](const auto& E){return !E.bHarmful;});
}
FText ICPartyStatus::Describe(const FICCharacterState& S,const FICWorldState& W,UICDefinitionSubsystem* D,double Now,const UICAbilitySystemComponent* Recipient)
{
    TArray<FText> Lines;if(!D)return FText::GetEmpty();FICDefinitionResult Lookup;
    if(S.IsDowned())Lines.Add(NSLOCTEXT("ICPartyStatus","Downed","전투불능 · 부활서로 소생 가능 · 퇴출 불가"));
    if(LivingMember(S))for(const auto* R:ActiveSynergies(W,D,Recipient))Lines.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Synergy","[파티 버프] {0}\n{1}"),Name(*R),Modifiers(R->StatModifiers)));
    if(const auto* Pet=W.Codex.CharacterPets.Find(S.CharacterId);Pet&&LivingMember(S)&&W.Codex.PetEssence.FindRef(*Pet)>=100)
    {
        TArray<FName> IDs;D->GetDefinitionIDs(EICDefinitionType::Monster,IDs);
        for(FName ID:IDs)if(const auto* M=D->Find<FICMonsterRow>(ID,Lookup);M&&M->PetCodexIndex==*Pet)Lines.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Pet","[펫 버프] {0}\n{1}"),M->PetName,Modifiers(M->PetBuffs)));
    }
    for(const auto& E:S.Combat.Effects)if(const auto* R=D->Find<FICStatusEffectRow>(E.EffectID,Lookup);R&&(E.bInfinite||E.ExpiresAtPlaySecond>Now))
    {
        auto Stacked=R->StatModifiers;for(auto& M:Stacked){M.Flat*=E.Stacks;M.Percent*=E.Stacks;}FText Detail=Modifiers(Stacked);TArray<FText> Extra;
        if(!Detail.IsEmpty())Extra.Add(Detail);
        if(R->DamageDealtMultiplier!=1)Extra.Add(FText::Format(NSLOCTEXT("ICPartyStatus","DamageDealt","주는 피해 ×{0}"),FText::AsNumber(R->DamageDealtMultiplier)));
        if(R->DamageReceivedMultiplier!=1)Extra.Add(FText::Format(NSLOCTEXT("ICPartyStatus","DamageReceived","받는 피해 ×{0}"),FText::AsNumber(R->DamageReceivedMultiplier)));
        if(R->HealingReceivedMultiplier!=1)Extra.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Healing","받는 회복 ×{0}"),FText::AsNumber(R->HealingReceivedMultiplier)));
        if(E.RemainingShield>0)Extra.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Shield","보호막 {0}"),FText::AsNumber(E.RemainingShield)));
        for(const auto& P:E.CapturedMagnitudes)if(P.Key==TEXT("Magnitude"))Extra.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Magnitude","효과량 {0}"),FText::AsNumber(P.Value)));
        const auto Duration=E.bInfinite?NSLOCTEXT("ICPartyStatus","Permanent","지속"):FText::Format(NSLOCTEXT("ICPartyStatus","Remaining","남은 {0}초"),FText::AsNumber(FMath::CeilToInt(E.ExpiresAtPlaySecond-Now)));
        Lines.Add(FText::Format(NSLOCTEXT("ICPartyStatus","Effect","[{0}] {1} · {2}중첩 · {3}\n{4}"),R->bHarmful?NSLOCTEXT("ICPartyStatus","Debuff","디버프"):NSLOCTEXT("ICPartyStatus","Buff","버프"),Name(*R),E.Stacks,Duration,FText::Join(FText::FromString(TEXT(" / ")),Extra)));
    }
    return Lines.IsEmpty()?NSLOCTEXT("ICPartyStatus","None","현재 적용 중인 버프 / 디버프가 없습니다."):FText::Join(FText::FromString(TEXT("\n\n")),Lines);
}
