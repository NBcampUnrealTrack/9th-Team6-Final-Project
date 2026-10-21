#include "Inventory/Use/ICItemUseService.h"
#include "Combat/GAS/Simulation/ICCombatSimulation.h"
#include "Debug/ICLog.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/Combat/ICStatusEffectRow.h"
#include "World/State/ICWorldStateSubsystem.h"

FICItemUseResult UICItemUseService::Prepare(UICInventoryComponent* Inventory,UICDefinitionSubsystem* Definitions,FName ItemID,const FICCharacterState& Target)
{
    IC_TRACE_FUNCTION();
    FICItemUseResult R;R.ItemID=ItemID;R.TargetID=Target.CharacterId;
    auto Fail=[&](const TCHAR* Field,const TCHAR* Reason){ IC_TRACE_FUNCTION();R.Field=Field;R.Reason=Reason;UE_LOG(LogICInventory, Verbose, TEXT("[Item.Prepare.Rejected] Item=%s Target=%s Field=%s Reason=%s"),*ItemID.ToString(),*Target.CharacterId.ToString(),Field,Reason);return R;};
    FICDefinitionResult Lookup;
    const auto* D=Definitions?Definitions->Find<FICItemRow>(ItemID,Lookup):nullptr;
    if(!Inventory || !D)return Fail(TEXT("ItemID"),TEXT("아이템 정의를 조회할 수 없습니다. 아이템은 소모하지 않았습니다."));
    if(D->Kind!=EPlayerItemKind::Consumable || !UICWorldStateSubsystem::IsCharacterIdValid(Target.CharacterId) || !Target.HasStats())
        return Fail(TEXT("Target"),TEXT("사용할 수 없는 아이템 또는 대상입니다."));
    if(Target.HP<=0&&!Target.IsDowned())return Fail(TEXT("Target.HP"),TEXT("사망한 대상에게는 이 아이템을 사용할 수 없습니다."));
    const bool Player=Target.Kind==EICCharacterKind::Player;
    const bool Party=Target.Kind==EICCharacterKind::Recruitable && Target.Mode==EICCharacterMode::Companion && Target.PartySlot>=0;
    const bool Outside=Target.Kind==EICCharacterKind::Recruitable && !Party;
    bool Allowed=false;
    switch(D->UseTarget)
    {
        case EICItemTarget::PlayerOnly:Allowed=Player&&!Target.IsDowned();break;
        case EICItemTarget::LivingParty:Allowed=(Player||Party||(Outside&&D->bAllowNonCompanionRecruitUse))&&!Target.IsDowned();break;
        case EICItemTarget::DownedCompanion:Allowed=Party&&Target.IsDowned();break;
        case EICItemTarget::NonCompanionRecruit:Allowed=Outside&&!Target.IsDowned();break;
        default:break;
    }
    if(!Allowed)return Fail(TEXT("UseTarget"),TEXT("이 아이템을 사용할 수 없는 대상입니다."));

    if(!D->StatusEffectIDs.IsEmpty())
    {
        for(FName ID:D->StatusEffectIDs)if(!Definitions->Find<FICStatusEffectRow>(ID,Lookup))
            return Fail(TEXT("StatusEffectIDs"),TEXT("효과 정의가 누락되었습니다. 변경하지 않았습니다."));
    }
    FICCharacterState C=Target;if(Player)C.Equipment.Reset();
    bool Changed=false;
    switch(D->UseEffect)
    {
        case EICItemEffect::Recovery:
        {
            const double Values[]={D->RestoreHP,D->RestoreMP,D->RestoreSP,D->RestoreHPRatio,D->RestoreMPRatio,D->RestoreSPRatio};
            for(double V:Values)if(!FMath::IsFinite(V)||V<0)return Fail(TEXT("Recovery"),TEXT("회복량 설정이 올바르지 않습니다."));
            if(D->RestoreHPRatio>1 || D->RestoreMPRatio>1 || D->RestoreSPRatio>1)return Fail(TEXT("RecoveryRatio"),TEXT("회복 비율은 0~1이어야 합니다."));
            const float HP=FMath::Min<double>(C.MaxHP,C.HP+D->RestoreHP+C.MaxHP*D->RestoreHPRatio);
            const float MP=FMath::Min<double>(C.MaxMP,C.MP+D->RestoreMP+C.MaxMP*D->RestoreMPRatio);
            const float SP=Player?FMath::Min<double>(C.MaxSP,C.SP+D->RestoreSP+C.MaxSP*D->RestoreSPRatio):0.f;
            Changed=HP!=C.HP||MP!=C.MP||SP!=C.SP;C.HP=HP;C.MP=MP;C.SP=SP;break;
        }
        case EICItemEffect::GrowthBook:
        {
            TArray<FName> Settings;
            if(!Definitions->GetDefinitionIDs(EICDefinitionType::Settings,Settings).bSuccess)return Fail(TEXT("StatSettings"),TEXT("능력치 규격을 조회할 수 없습니다."));
            const FICStatSettingsRow* Rules=nullptr;
            for(FName ID:Settings)if(const auto* Found=Definitions->Find<FICStatSettingsRow>(ID,Lookup)){if(Rules)return Fail(TEXT("StatSettings"),TEXT("능력치 규격이 중복되었습니다."));Rules=Found;}
            const int32 Index=static_cast<int32>(D->GrowthStat);
            if(!Rules || Rules->MainStatCap<1 || Rules->MainStatCap>10000 || Index<0 || Index>5 || D->GrowthAmount<=0)
                return Fail(TEXT("GrowthStat"),TEXT("성장 능력치·상한 설정을 확인하세요."));
            const int32 Values[]={C.MainStats.STR,C.MainStats.DEX,C.MainStats.INT,C.MainStats.CON,C.MainStats.CHA,C.MainStats.LUK};
            const int32 Before=Values[Index];const int32 After=static_cast<int32>(FMath::Min<int64>(Rules->MainStatCap,int64(Before)+D->GrowthAmount));
            if(After<Before)return Fail(TEXT("MainStatCap"),TEXT("현재 능력치보다 낮은 상한으로 변경할 수 없습니다."));
            Changed=After>Before;
            if(!Changed&&!D->bConsumeAtGrowthCap)return Fail(TEXT("MainStatCap"),TEXT("해당 능력치가 최대치입니다. 아이템은 소모하지 않았습니다."));
            C.MainStats.Set(static_cast<EICMainStat>(Index),After);R.GrowthStat=D->GrowthStat;R.GrowthAmount=After-Before;break;
        }
        case EICItemEffect::Revival:
            if(!Party||!C.IsDowned()||!FMath::IsFinite(D->RevivalHPRatio)||D->RevivalHPRatio<=0||D->RevivalHPRatio>1
                || !FMath::IsFinite(D->RevivalMinimumMPRatio)||D->RevivalMinimumMPRatio<0||D->RevivalMinimumMPRatio>1)
                return Fail(TEXT("Revival"),TEXT("부활 대상·회복 비율이 올바르지 않습니다."));
            C.LifeState=EICCharacterLifeState::Alive;C.HP=C.MaxHP*D->RevivalHPRatio;
            C.MP=FMath::Max<double>(C.MP,C.MaxMP*D->RevivalMinimumMPRatio);Changed=true;break;
        case EICItemEffect::Dispel:
        {
            TSet<FGuid> Remove;
            for(const auto& Effect:C.Combat.Effects)
            {
                const auto* Rule=Definitions->Find<FICStatusEffectRow>(Effect.EffectID,Lookup);
                if(!Rule)return Fail(TEXT("Combat.Effects"),TEXT("해제할 상태 효과의 정의가 누락되었습니다."));
                if(Effect.bHarmful && Rule->bHarmful && (D->DispelTags.IsEmpty() || Rule->Tags.HasAny(D->DispelTags) || Rule->GrantedStateTags.HasAny(D->DispelTags))
                    && (Rule->AllowedCureItemIDs.IsEmpty() || Rule->AllowedCureItemIDs.Contains(ItemID)))Remove.Add(Effect.InstanceID);
            }
            Changed=!Remove.IsEmpty();C.Combat.Effects.RemoveAll([&](const auto& E){ IC_TRACE_FUNCTION();return Remove.Contains(E.InstanceID);});break;
        }
        case EICItemEffect::None:if(D->StatusEffectIDs.IsEmpty())return Fail(TEXT("UseEffect"),TEXT("사용 효과가 설정되지 않았습니다."));break;
        default:return Fail(TEXT("UseEffect"),TEXT("사용 효과가 설정되지 않았습니다."));
    }
    for(FName ID:D->StatusEffectIDs){FString Reason;if(!ICCombatSimulation::PrepareItemStatus(C,ID,Definitions,Reason)){R.Field=TEXT("StatusEffectIDs");R.Reason=Reason;return R;}Changed=true;}
    const bool ConsumeAtCap=D->UseEffect==EICItemEffect::GrowthBook&&D->bConsumeAtGrowthCap;
    if(!Changed&&D->bRejectWhenNoEffect&&!ConsumeAtCap)return Fail(TEXT("UseEffect"),TEXT("이미 최대치이거나 적용할 효과가 없습니다. 아이템은 소모하지 않았습니다."));
    FICInventoryState Items;
    if(!Inventory->BuildConsumption(ItemID,1,Items))return Fail(TEXT("Inventory"),TEXT("아이템이 없거나 잠겨 있습니다."));
    R.Character=MoveTemp(C);R.Inventory=MoveTemp(Items);R.bSuccess=true;
    UE_LOG(LogICInventory, Verbose, TEXT("[Item.Prepared] Item=%s Target=%s HP=%.2f MP=%.2f SP=%.2f Growth=%d: 아직 원본에 반영하지 않음"),*ItemID.ToString(),*R.TargetID.ToString(),R.Character.HP,R.Character.MP,R.Character.SP,R.GrowthAmount);return R;
}
