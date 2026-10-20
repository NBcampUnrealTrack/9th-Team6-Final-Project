#include "NPC/LifeNPC/ICShopSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "Data/Rows/NPC/ICServiceRow.h"
#include "Data/Rows/NPC/ICShopRow.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Debug/ICLog.h"

bool UICShopSubsystem::ResolveService(FName ServiceID,const FICServiceRow*& Service,FString& Reason) const
{
    IC_TRACE_FUNCTION();Service=nullptr;
    auto* GI=GetGameInstance();
    auto* D=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    auto* S=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    auto* T=GI?GI->GetSubsystem<UICTimeSubsystem>():nullptr;
    if(!D||!S||!T){Reason=TEXT("상점 시스템을 사용할 수 없습니다.");return false;}
    FICDefinitionResult R;
    const auto* Row=D->Find<FICServiceRow>(ServiceID,R);
    if(!Row){Reason=TEXT("서비스 정의를 찾을 수 없습니다.");return false;}
    if(Row->Type!=EICServiceType::Shop){Reason=TEXT("상점형 서비스가 아닙니다.");return false;}
    const int32 Minute=int32(FMath::FloorToDouble(T->GetClock().GameMinute))%1440;

    bool bOpen=Row->OpeningHours.IsEmpty();
    for(const auto& Hours:Row->OpeningHours)if(Minute>=Hours.OpenGameMinute&&Minute<Hours.CloseGameMinute)bOpen=true;
    if(!bOpen){Reason=TEXT("지금은 영업 시간이 아닙니다.");return false;}
    const auto NPC=S->GetSnapshot(Row->NPCID);
    if(NPC.IsDowned()||NPC.Mode!=EICCharacterMode::Scheduled){Reason=TEXT("상점 주인이 거래할 수 없는 상태입니다.");return false;}
    if(!Row->WorkAnchorID.IsNone()&&NPC.CurrentAnchorID!=Row->WorkAnchorID){Reason=TEXT("상점 주인이 근무 위치에 없습니다.");return false;}
    const auto* NPCRow=D->Find<FICNPCRow>(Row->NPCID,R);
    const auto* Schedule=NPCRow?D->Find<FICScheduleRow>(NPCRow->ScheduleID,R):nullptr;
    bool bWorking=false;
    if(Schedule)for(const auto& Segment:Schedule->Segments)
        if(Minute>=Segment.StartGameMinute&&Minute<Segment.EndGameMinute&&Segment.Activity==EICScheduleActivity::Work&&Segment.ServiceIDs.Contains(ServiceID))bWorking=true;
    if(!bWorking){Reason=TEXT("상점 주인이 지금 이 서비스를 근무 중이 아닙니다.");return false;}

    Service=Row;return true;
}

double UICShopSubsystem::DiscountRatioFor(const FICServiceRow& Service) const
{
    IC_TRACE_FUNCTION();
    auto* S=GetGameInstance()?GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    const int32 Affinity=S?S->GetSnapshot(Service.NPCID).Affinity:0;

    int32 BestTier=-1;double Ratio=0.;
    for(const auto& Tier:Service.AffinityDiscounts)if(Affinity>=Tier.MinimumAffinity&&Tier.MinimumAffinity>BestTier){BestTier=Tier.MinimumAffinity;Ratio=Tier.DiscountRatio;}
    return FMath::Clamp(Ratio,0.,1.);
}

bool UICShopSubsystem::CanUseShop(FName ServiceID,FString& Reason) const
{
    IC_TRACE_FUNCTION();
    const FICServiceRow* Service=nullptr;
    return ResolveService(ServiceID,Service,Reason);
}

bool UICShopSubsystem::GetOffers(FName ServiceID,TArray<FICShopOffer>& Offers,FString& Reason) const
{
    IC_TRACE_FUNCTION();Offers.Reset();
    const FICServiceRow* Service=nullptr;
    if(!ResolveService(ServiceID,Service,Reason))return false;
    auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult R;
    const auto* Shop=D->Find<FICShopRow>(Service->ShopID,R);
    if(!Shop||Shop->ServiceID!=ServiceID){Reason=TEXT("상점 정의가 없거나 이 서비스와 연결되어 있지 않습니다.");return false;}
    TArray<FICShopEntry> Entries=Shop->Entries;
    Entries.StableSort([](const FICShopEntry& A,const FICShopEntry& B){return A.DisplayOrder<B.DisplayOrder;});
    const double Ratio=DiscountRatioFor(*Service);
    for(const auto& Entry:Entries)
    {
        const auto* Item=D->Find<FICItemRow>(Entry.ItemID,R);
        if(!Item||Item->BuyPrice<=0)
        {UE_LOG(LogICData,Warning,TEXT("[Shop.EntrySkipped] Service=%s Item=%s Reason=아이템 정의가 없거나 BuyPrice가 0 이하입니다."),*ServiceID.ToString(),*Entry.ItemID.ToString());continue;}
        FICShopOffer Offer;Offer.ItemID=Entry.ItemID;Offer.BasePrice=Item->BuyPrice;
        Offer.FinalPrice=FMath::Max(1,FMath::FloorToInt(Item->BuyPrice*(1.-Ratio)));
        Offer.SellPrice=Item->bCanSell?FMath::Max(0,Item->SellPrice):0;Offer.DiscountRatio=float(Ratio);
        Offers.Add(Offer);
    }
    return true;
}

bool UICShopSubsystem::Buy(AICPlayerController* Player,FName ServiceID,FName ItemID,int32 Count,FString& Reason)
{
    IC_TRACE_FUNCTION();
    auto* Inventory=Player?Player->GetInventory():nullptr;
    if(!Inventory||Count<=0){Reason=TEXT("유효한 플레이어와 1 이상의 수량이 필요합니다.");return false;}
    TArray<FICShopOffer> Offers;
    if(!GetOffers(ServiceID,Offers,Reason))return false;
    const FICShopOffer* Offer=Offers.FindByPredicate([ItemID](const FICShopOffer& O){return O.ItemID==ItemID;});
    if(!Offer){Reason=TEXT("이 상점에서 판매하지 않는 아이템입니다.");return false;}
    const int64 Total=int64(Offer->FinalPrice)*Count;
    const int32 Before=Player->Gold;
    if(Total>Before){Reason=TEXT("골드가 부족합니다.");return false;}

    if(!Player->SetGold(Before-int32(Total))){Reason=TEXT("골드를 차감하지 못했습니다.");return false;}
    if(!Inventory->AddItem(ItemID,Count,false))
    {
        if(!Player->SetGold(Before))UE_LOG(LogICInventory,Error,TEXT("[Shop.RefundFailed] Service=%s Item=%s Gold=%d"),*ServiceID.ToString(),*ItemID.ToString(),Before);
        Reason=TEXT("가방에 공간이 부족합니다.");return false;
    }
    UE_LOG(LogICInventory,Log,TEXT("[Shop.Buy] Service=%s Item=%s Count=%d Total=%lld Gold=%d->%d"),*ServiceID.ToString(),*ItemID.ToString(),Count,Total,Before,Player->Gold);
    return true;
}

bool UICShopSubsystem::Sell(AICPlayerController* Player,FName ServiceID,FName ItemID,int32 Count,FString& Reason)
{
    IC_TRACE_FUNCTION();
    auto* Inventory=Player?Player->GetInventory():nullptr;
    if(!Inventory||Count<=0){Reason=TEXT("유효한 플레이어와 1 이상의 수량이 필요합니다.");return false;}
    const FICServiceRow* Service=nullptr;
    if(!ResolveService(ServiceID,Service,Reason))return false;
    const auto* Item=Inventory->Find(ItemID);
    if(!Item||!Item->bCanSell||Item->SellPrice<=0){Reason=TEXT("판매할 수 없는 아이템입니다.");return false;}
    if(Inventory->CountItem(ItemID)<Count){Reason=TEXT("보유 수량이 부족합니다.");return false;}
    const int64 Total=int64(Item->SellPrice)*Count;
    const int32 Before=Player->Gold;
    if(int64(Before)+Total>MAX_int32){Reason=TEXT("골드 보유 한도를 넘습니다.");return false;}

    if(!Inventory->Consume(ItemID,Count)){Reason=TEXT("아이템을 소비하지 못했습니다.");return false;}
    if(!Player->SetGold(Before+int32(Total)))
    {
        if(!Inventory->AddItem(ItemID,Count,false))UE_LOG(LogICInventory,Error,TEXT("[Shop.RollbackFailed] Service=%s Item=%s Count=%d"),*ServiceID.ToString(),*ItemID.ToString(),Count);
        Reason=TEXT("골드를 지급하지 못했습니다.");return false;
    }
    UE_LOG(LogICInventory,Log,TEXT("[Shop.Sell] Service=%s Item=%s Count=%d Total=%lld Gold=%d->%d"),*ServiceID.ToString(),*ItemID.ToString(),Count,Total,Before,Player->Gold);
    return true;
}
