#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Codex/ICCodexSubsystem.h"
namespace
{
const FICEnhancementOreRow* OreRule(FName ID)
{auto* T=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/Data/DataTable/Equipment/DT_IC_EnhancementOre.DT_IC_EnhancementOre"));if(T)for(const auto& Name:T->GetRowNames())if(const auto* R=T->FindRow<FICEnhancementOreRow>(Name,TEXT("Enhancement"),false);R&&R->ItemID==ID)return R;return nullptr;}
}
TArray<FICEnhancementTarget> UICInventoryComponent::GetEnhancementTargets() const
{
    TArray<FICEnhancementTarget> Out;TSet<FGuid> Seen;
    auto Add=[&](const FPlayerItemInstance& I,FName Owner,FText Location)
    {const auto* D=Find(I.ItemID);if(!I.IsEmpty()&&I.InstanceID.IsValid()&&D&&D->Kind==EPlayerItemKind::Equipment&&!Seen.Contains(I.InstanceID)){Seen.Add(I.InstanceID);FICEnhancementTarget T;T.Item=I;T.OwnerID=Owner;T.Location=Location;Out.Add(T);}};
    const auto S=GetSnapshot();for(const auto& I:S.Bag.Slots)Add(I,NAME_None,NSLOCTEXT("ICEnhance","Bag","가방"));
    for(const auto& I:S.Equipment)Add(I,UICWorldStateSubsystem::PlayerId(),NSLOCTEXT("ICEnhance","Player","플레이어 장착"));
    for(const auto& I:S.WeaponSlots)Add(I,UICWorldStateSubsystem::PlayerId(),NSLOCTEXT("ICEnhance","Loadout","플레이어 무기 슬롯"));
    auto* Names=GetWorld()&&GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UICCodexSubsystem>():nullptr;
    if(WorldState)for(const auto& P:WorldState->ExportState().Characters)if(P.Value.Kind==EICCharacterKind::Recruitable&&P.Value.Mode==EICCharacterMode::Companion&&P.Value.PartySlot>=0)
        for(const auto& I:P.Value.Equipment)Add(I,P.Key,Names?Names->CharacterName(P.Key):FText::FromName(P.Key));
    return Out;
}
bool UICInventoryComponent::GetEnhancementOffer(FGuid ID,FName OreID,double& Probability,int32& GoldCost,int32& OreCount,FText& Reason) const
{
    Probability=0;GoldCost=OreCount=0;Reason=FText::GetEmpty();const auto Targets=GetEnhancementTargets();const auto* T=Targets.FindByPredicate([ID](const auto& X){return X.Item.InstanceID==ID;});
    if(!T||T->Item.bLocked){Reason=NSLOCTEXT("ICEnhance","Unavailable","무기가 없거나 잠금 상태입니다.");return false;}
    FICDefinitionResult R;const auto* Item=Find(T->Item.ItemID);const auto* G=Item&&GetDefinitions()?GetDefinitions()->Find<FICEquipmentRow>(Item->EquipmentID,R):nullptr;const auto* Rules=G?GetDefinitions()->Find<FICEnhancementRow>(G->EnhancementID,R):nullptr;
    if(!Rules||T->Item.Upgrade>=Rules->MaximumLevel){Reason=NSLOCTEXT("ICEnhance","Max","최대 강화 단계이거나 강화 정보가 없습니다.");return false;}
    const auto* Step=Rules->Steps.FindByPredicate([&](const auto& S){return S.TargetLevel==T->Item.Upgrade+1;});if(!Step||Step->GoldCost<0||!FMath::IsFinite(Step->SuccessProbability))return false;
    Probability=FMath::Clamp(Step->SuccessProbability,0.,1.);GoldCost=Step->GoldCost;
    if(OreID.IsNone())return true;
    const auto* Ore=OreRule(OreID);if(!Ore||Ore->Count<1||!FMath::IsFinite(Ore->SuccessBonus)){Reason=NSLOCTEXT("ICEnhance","BadOre","강화용 광석 정보를 확인하세요.");return false;}
    Probability=FMath::Clamp(Probability+Ore->SuccessBonus,0.,1.);OreCount=Ore->Count;
    if(CountItem(OreID)<OreCount){Reason=NSLOCTEXT("ICEnhance","NeedOre","선택한 광석이 부족합니다.");return false;}
    return true;
}
bool UICInventoryComponent::TryEnhanceWeapon(FGuid ID,FName OreID,bool& Success,FText& Message)
{
    Success=false;Message=FText::GetEmpty();if(!WorldState||!WorldState->FlushParticipants()||OreID.IsNone())return false;
    double Probability=0;int32 GoldCost=0,OreCount=0;if(!GetEnhancementOffer(ID,OreID,Probability,GoldCost,OreCount,Message))return false;
    auto Next=WorldState->ExportState();if(Next.Gold<GoldCost){Message=NSLOCTEXT("ICEnhance","NeedGold","강화에 필요한 골드가 부족합니다.");return false;}
    FICInventoryState Consumed;if(!BuildConsumption(OreID,OreCount,Consumed)){Message=NSLOCTEXT("ICEnhance","OreLocked","광석이 잠겨 있거나 사용할 수 없습니다.");return false;}
    Next.Inventory=Consumed;const bool Roll=FMath::FRand()<Probability;int32 NewLevel=0;bool Found=false;
    auto Update=[&](TArray<FPlayerItemInstance>& Items){for(auto& I:Items)if(I.InstanceID==ID){if(Roll)++I.Upgrade;NewLevel=I.Upgrade;Found=true;}};
    Update(Next.Inventory.Bag.Slots);Update(Next.Inventory.Equipment);Update(Next.Inventory.WeaponSlots);
    for(auto& P:Next.Characters)if(P.Key==UICWorldStateSubsystem::PlayerId()||(P.Value.Mode==EICCharacterMode::Companion&&P.Value.PartySlot>=0))Update(P.Value.Equipment);
    if(!Found)return false;Next.Gold-=GoldCost;
    if(!ValidateSnapshot(Next.Inventory)||!WorldState->CommitGameplayTransaction(Next,FGuid::NewGuid())){Message=NSLOCTEXT("ICEnhance","CommitFailed","강화 상태가 변경되어 시도하지 못했습니다. 비용은 소비되지 않았습니다.");return false;}
    Success=Roll;
    Message=FText::Format(Roll?NSLOCTEXT("ICEnhance","Success","강화 성공! +{0} · 광석 {1}개 / {2}G 사용"):NSLOCTEXT("ICEnhance","Failure","강화 실패 · +{0} 유지 · 광석 {1}개 / {2}G 사용"),NewLevel,OreCount,GoldCost);
    OnMessage.Broadcast(Message);return true;
}
