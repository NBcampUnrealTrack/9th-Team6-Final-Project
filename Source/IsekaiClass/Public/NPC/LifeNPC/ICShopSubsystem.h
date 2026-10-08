#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ICShopSubsystem.generated.h"

class AICPlayerController;
struct FICServiceRow;

USTRUCT(BlueprintType, meta=(ToolTip="상점 UI에 표시할 판매 항목 한 줄입니다. 가격은 Item DT의 BuyPrice에 서비스의 호감도 할인을 적용한 값이며 거래 시점에 다시 계산됩니다.", ShortToolTip="상점 UI에 표시할 판매 항목입니다."))
struct ISEKAICLASS_API FICShopOffer
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop", meta=(ToolTip="판매 아이템의 Item DT ID입니다.\n입력 자료형: 이름/ID.\nBP에서는 조회 전용입니다.")) FName ItemID;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop", meta=(ToolTip="Item DT의 구매 정가입니다. 할인 전 금액이며 골드 단위입니다.\n입력 자료형: 정수.\nBP에서는 조회 전용입니다.")) int32 BasePrice=0;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop", meta=(ToolTip="현재 호감도 할인을 적용한 실제 구매 가격입니다. 정가가 0보다 크면 최소 1입니다.\n입력 자료형: 정수.\nBP에서는 조회 전용입니다.")) int32 FinalPrice=0;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop", meta=(ToolTip="플레이어가 이 아이템을 되팔 때 받는 금액입니다. 판매 불가 아이템이면 0입니다.\n입력 자료형: 정수.\nBP에서는 조회 전용입니다.")) int32 SellPrice=0;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop", meta=(ToolTip="현재 호감도로 적용 중인 할인율입니다. 0.1이면 10% 할인이며 UI의 할인 표시에 사용합니다.\n입력 자료형: 소수 0~1.\nBP에서는 조회 전용입니다.")) float DiscountRatio=0.f;
};

UCLASS(meta=(ToolTip="NPC 상점 서비스의 영업 가능 여부·판매 목록·호감도 할인·구매/판매를 처리합니다. 정적 정의(서비스·상점·아이템 DT)와 WorldState(호감도·골드·인벤토리)만 읽고 쓰며 자체 상태를 저장하지 않습니다. 구매·판매는 실패해도 골드와 아이템이 일부만 바뀌지 않도록 되돌립니다.", ShortToolTip="NPC 상점의 영업 조건·가격·구매/판매를 처리합니다."))
class ISEKAICLASS_API UICShopSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Shop", meta=(ToolTip="상점 서비스를 지금 이용할 수 있는지 검사합니다. 서비스 영업시간, 상점 주인의 근무 구간·위치·상태를 모두 확인합니다.\n@param ServiceID Service DT의 상점형 서비스 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Reason 이용할 수 없을 때의 사유입니다. 출력 핀. 자료형: 문자열.\n@return 이용할 수 있으면 true입니다.")) bool CanUseShop(FName ServiceID, FString& Reason) const;
    UFUNCTION(BlueprintCallable, Category="IC|Shop", meta=(ToolTip="상점의 판매 목록을 DisplayOrder 순으로 가격과 함께 조회합니다. 정의가 없거나 구매 가격이 0 이하인 항목은 제외하고 경고 로그를 남깁니다.\n@param ServiceID Service DT의 상점형 서비스 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Offers 판매 항목 목록입니다. 출력 핀. 자료형: 구조 배열 (FICShopOffer).\n@param Reason 실패 사유입니다. 출력 핀. 자료형: 문자열.\n@return 조회에 성공하면 true입니다.")) bool GetOffers(FName ServiceID, TArray<FICShopOffer>& Offers, FString& Reason) const;
    UFUNCTION(BlueprintCallable, Category="IC|Shop", meta=(ToolTip="상점의 아이템을 구매합니다. 영업·근무 조건과 판매 목록을 확인하고 골드를 먼저 차감한 뒤 아이템을 전부 지급하며, 가방 공간이 부족하면 골드를 환불하고 실패합니다.\n@param Player 구매하는 플레이어 컨트롤러입니다. 입력 핀. 자료형: 객체 참조.\n@param ServiceID 상점형 서비스 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param ItemID 구매할 아이템 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Count 구매 수량입니다. 1 이상. 입력 핀. 자료형: 정수.\n@param Reason 실패 사유입니다. 출력 핀. 자료형: 문자열.\n@return 구매가 완료되면 true입니다.")) bool Buy(AICPlayerController* Player, FName ServiceID, FName ItemID, int32 Count, FString& Reason);
    UFUNCTION(BlueprintCallable, Category="IC|Shop", meta=(ToolTip="가방의 아이템을 상점에 판매합니다. 영업·근무 조건과 판매 가능 여부·보유 수량을 확인하고 아이템을 소비한 뒤 골드를 지급하며, 골드 지급에 실패하면 아이템을 되돌립니다. 현재는 수량 단위로 거래되는 일반 아이템만 지원합니다.\n@param Player 판매하는 플레이어 컨트롤러입니다. 입력 핀. 자료형: 객체 참조.\n@param ServiceID 상점형 서비스 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param ItemID 판매할 아이템 ID입니다. 입력 핀. 자료형: 이름/ID.\n@param Count 판매 수량입니다. 1 이상. 입력 핀. 자료형: 정수.\n@param Reason 실패 사유입니다. 출력 핀. 자료형: 문자열.\n@return 판매가 완료되면 true입니다.")) bool Sell(AICPlayerController* Player, FName ServiceID, FName ItemID, int32 Count, FString& Reason);
private:
    bool ResolveService(FName ServiceID, const FICServiceRow*& Service, FString& Reason) const;
    double DiscountRatioFor(const FICServiceRow& Service) const;
};
