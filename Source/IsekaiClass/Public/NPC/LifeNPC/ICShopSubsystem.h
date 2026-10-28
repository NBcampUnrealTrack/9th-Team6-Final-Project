#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICShopSubsystem.generated.h"

class AICPlayerController;
struct FICServiceRow;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICShopOffer
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop") FName ItemID;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop") int32 BasePrice=0;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop") int32 FinalPrice=0;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop") int32 SellPrice=0;
    UPROPERTY(BlueprintReadOnly, Category="IC|Shop") float DiscountRatio=0.f;
};

UCLASS()
class ISEKAICLASS_API UICShopSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Shop") bool CanUseShop(FName ServiceID, FString& Reason) const;
    UFUNCTION(BlueprintCallable, Category="IC|Shop") bool GetOffers(FName ServiceID, TArray<FICShopOffer>& Offers, FString& Reason) const;
    UFUNCTION(BlueprintCallable, Category="IC|Shop") bool Buy(AICPlayerController* Player, FName ServiceID, FName ItemID, int32 Count, FString& Reason);
    UFUNCTION(BlueprintCallable, Category="IC|Shop") bool Sell(AICPlayerController* Player, FName ServiceID, FName ItemID, int32 Count, FString& Reason);
    UFUNCTION(BlueprintCallable, Category="IC|Shop") bool Checkout(AICPlayerController* Player,FName ServiceID,const TArray<FICItemAmount>& Cart,bool bSell,FText& Reason);
    UFUNCTION(BlueprintPure, Category="IC|Shop") int32 GetSellableCount(AICPlayerController* Player,FName ItemID) const;
private:
    bool ResolveService(FName ServiceID, const FICServiceRow*& Service, FString& Reason) const;
    double DiscountRatioFor(const FICServiceRow& Service) const;
};
