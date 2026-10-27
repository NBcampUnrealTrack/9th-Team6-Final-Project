#include "Inventory/Use/ICItemUseService.h"
#include "UI/Framework/ICText.h"
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
    auto Fail=[&](const TCHAR* Field,const FString& Reason){ IC_TRACE_FUNCTION();R.Field=Field;R.Reason=Reason;UE_LOG(LogICInventory, Verbose, TEXT("[Item.Prepare.Rejected] Item=%s Target=%s Field=%s Reason=%s"),*ItemID.ToString(),*Target.CharacterId.ToString(),Field,*Reason);return R;};
    FICDefinitionResult Lookup;
    const auto* D=Definitions?Definitions->Find<FICItemRow>(ItemID,Lookup):nullptr;
    if(!Inventory || !D)return Fail(TEXT("ItemID"),ICText::Get(TEXT("Native_95077894d065")).ToString());
    if(D->Kind!=EPlayerItemKind::Consumable || !UICWorldStateSubsystem::IsCharacterIdValid(Target.CharacterId) || !Target.HasStats())
        return Fail(TEXT("Target"),ICText::Get(TEXT("Native_23a0ec358bbb")).ToString());
    if(Target.HP<=0&&!Target.IsDowned())return Fail(TEXT("Target.HP"),ICText::Get(TEXT("Native_ae947b06ab74")).ToString());
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
    if(!Allowed)return Fail(TEXT("UseTarget"),ICText::Get(TEXT("Native_2e83db18024b")).ToString());

    if(!D->StatusEffectIDs.IsEmpty())
    {
        for(FName ID:D->StatusEffectIDs)if(!Definitions->Find<FICStatusEffectRow>(ID,Lookup))
            return Fail(TEXT("StatusEffectIDs"),ICText::Get(TEXT("Native_34b7631c3281")).ToString());
    }
    FICCharacterState C=Target;if(Player)C.Equipment.Reset();
    bool Changed=false;
    switch(D->UseEffect)
    {
        case EICItemEffect::Recovery:
        {
            const double Values[]={D->RestoreHP,D->RestoreMP,D->RestoreSP,D->RestoreHPRatio,D->RestoreMPRatio,D->RestoreSPRatio};
            for(double V:Values)if(!FMath::IsFinite(V)||V<0)return Fail(TEXT("Recovery"),ICText::Get(TEXT("Native_0f62d5bb1ef1")).ToString());
            if(D->RestoreHPRatio>1 || D->RestoreMPRatio>1 || D->RestoreSPRatio>1)return Fail(TEXT("RecoveryRatio"),ICText::Get(TEXT("Native_f60eae1ba17b")).ToString());
            const float HP=FMath::Min<double>(C.MaxHP,C.HP+D->RestoreHP+C.MaxHP*D->RestoreHPRatio);
            const float MP=FMath::Min<double>(C.MaxMP,C.MP+D->RestoreMP+C.MaxMP*D->RestoreMPRatio);
            const float SP=Player?FMath::Min<double>(C.MaxSP,C.SP+D->RestoreSP+C.MaxSP*D->RestoreSPRatio):0.f;
            Changed=HP!=C.HP||MP!=C.MP||SP!=C.SP;C.HP=HP;C.MP=MP;C.SP=SP;break;
        }
        case EICItemEffect::GrowthBook:
        {
            TArray<FName> Settings;
            if(!Definitions->GetDefinitionIDs(EICDefinitionType::Settings,Settings).bSuccess)return Fail(TEXT("StatSettings"),ICText::Get(TEXT("Native_21f5a25f1253")).ToString());
            const FICStatSettingsRow* Rules=nullptr;
            for(FName ID:Settings)if(const auto* Found=Definitions->Find<FICStatSettingsRow>(ID,Lookup)){if(Rules)return Fail(TEXT("StatSettings"),ICText::Get(TEXT("Native_5ba8a50c7af9")).ToString());Rules=Found;}
            const int32 Index=static_cast<int32>(D->GrowthStat);
            if(!Rules || Rules->MainStatCap<1 || Rules->MainStatCap>10000 || Index<0 || Index>5 || Index==4 || D->GrowthAmount<=0)
                return Fail(TEXT("GrowthStat"),ICText::Get(TEXT("Native_fb0f8e10e9ad")).ToString());
            const int32 Values[]={C.MainStats.STR,C.MainStats.DEX,C.MainStats.INT,C.MainStats.CON,C.MainStats.CHA,C.MainStats.LUK};
            const int32 Before=Values[Index];const int32 After=static_cast<int32>(FMath::Min<int64>(Rules->MainStatCap,int64(Before)+D->GrowthAmount));
            if(After<Before)return Fail(TEXT("MainStatCap"),ICText::Get(TEXT("Native_3e3c9980e589")).ToString());
            Changed=After>Before;
            if(!Changed&&!D->bConsumeAtGrowthCap)return Fail(TEXT("MainStatCap"),ICText::Get(TEXT("Native_9b82835c52de")).ToString());
            C.MainStats.Set(static_cast<EICMainStat>(Index),After);R.GrowthStat=D->GrowthStat;R.GrowthAmount=After-Before;break;
        }
        case EICItemEffect::Revival:
            if(!Party||!C.IsDowned()||!FMath::IsFinite(D->RevivalHPRatio)||D->RevivalHPRatio<=0||D->RevivalHPRatio>1
                || !FMath::IsFinite(D->RevivalMinimumMPRatio)||D->RevivalMinimumMPRatio<0||D->RevivalMinimumMPRatio>1)
                return Fail(TEXT("Revival"),ICText::Get(TEXT("Native_4bd3821c5068")).ToString());
            C.LifeState=EICCharacterLifeState::Alive;C.HP=C.MaxHP*D->RevivalHPRatio;
            C.MP=FMath::Max<double>(C.MP,C.MaxMP*D->RevivalMinimumMPRatio);Changed=true;break;
        case EICItemEffect::Dispel:
        {
            TSet<FGuid> Remove;
            for(const auto& Effect:C.Combat.Effects)
            {
                const auto* Rule=Definitions->Find<FICStatusEffectRow>(Effect.EffectID,Lookup);
                if(!Rule)return Fail(TEXT("Combat.Effects"),ICText::Get(TEXT("Native_a782a10287b9")).ToString());
                if(Effect.bHarmful && Rule->bHarmful && (D->DispelTags.IsEmpty() || Rule->Tags.HasAny(D->DispelTags) || Rule->GrantedStateTags.HasAny(D->DispelTags))
                    && (Rule->AllowedCureItemIDs.IsEmpty() || Rule->AllowedCureItemIDs.Contains(ItemID)))Remove.Add(Effect.InstanceID);
            }
            Changed=!Remove.IsEmpty();C.Combat.Effects.RemoveAll([&](const auto& E){ IC_TRACE_FUNCTION();return Remove.Contains(E.InstanceID);});break;
        }
        case EICItemEffect::None:if(D->StatusEffectIDs.IsEmpty())return Fail(TEXT("UseEffect"),ICText::Get(TEXT("Native_74b370552341")).ToString());break;
        default:return Fail(TEXT("UseEffect"),ICText::Get(TEXT("Native_74b370552341")).ToString());
    }
    for(FName ID:D->StatusEffectIDs){FString Reason;if(!ICCombatSimulation::PrepareItemStatus(C,ID,Definitions,Reason)){R.Field=TEXT("StatusEffectIDs");R.Reason=Reason;return R;}Changed=true;}
    const bool ConsumeAtCap=D->UseEffect==EICItemEffect::GrowthBook&&D->bConsumeAtGrowthCap;
    if(!Changed&&D->bRejectWhenNoEffect&&!ConsumeAtCap)return Fail(TEXT("UseEffect"),ICText::Get(TEXT("Native_95e036acbacb")).ToString());
    FICInventoryState Items;
    if(!Inventory->BuildConsumption(ItemID,1,Items))return Fail(TEXT("Inventory"),ICText::Get(TEXT("Native_86d4e1888edc")).ToString());
    R.Character=MoveTemp(C);R.Inventory=MoveTemp(Items);R.bSuccess=true;
    UE_LOG(LogICInventory, Verbose, TEXT("[Item.Prepared] Item=%s Target=%s HP=%.2f MP=%.2f SP=%.2f Growth=%d: 아직 원본에 반영하지 않음"),*ItemID.ToString(),*R.TargetID.ToString(),R.Character.HP,R.Character.MP,R.Character.SP,R.GrowthAmount);return R;
}
