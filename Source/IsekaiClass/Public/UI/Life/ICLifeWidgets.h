#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "UI/Framework/ICActionButtonWidget.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICLifeWidgets.generated.h"
class UWidgetTree;
class AICPlayerController;
class AICLifeActor;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICLifeChoice,int32,Index);

UCLASS()
class ISEKAICLASS_API UICLifeChoiceButton : public UICActionButtonWidget
{
    GENERATED_BODY()
public:
    int32 Choice=0;
    UPROPERTY() FICLifeChoice OnChoice;
    void Configure(int32 Index,FText Caption);
    UFUNCTION() void Choose(){OnChoice.Broadcast(Choice);}
};
UCLASS(Blueprintable)
class ISEKAICLASS_API UICSleepFadeWidget : public UUserWidget
{
    GENERATED_BODY()
public: static bool BuildLayout(UWidgetTree* Tree);
protected: virtual void NativeOnInitialized() override;
};
UCLASS(Blueprintable)
class ISEKAICLASS_API UICSeedPickerWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static bool Open(AICPlayerController* Player,AICLifeActor* Plot);
    static bool BuildLayout(UWidgetTree* Tree);
    UFUNCTION(BlueprintCallable) void PickSeed(int32 Index);
    UFUNCTION(BlueprintCallable) void ClosePicker();
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    TWeakObjectPtr<AICLifeActor> Target;
    TArray<FName> CropIDs;
    int32 Selected=0;
    void Refresh();
};
UCLASS(Blueprintable)
class ISEKAICLASS_API UICShopWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static bool Open(AICPlayerController* Player,FName ServiceID);
    static bool BuildLayout(UWidgetTree* Tree);
    UFUNCTION(BlueprintCallable) void SelectTab(bool Sell);
    UFUNCTION(BlueprintCallable) void ItemPicked(int32 Index);
    UFUNCTION(BlueprintCallable) bool SubmitCart();
    UFUNCTION(BlueprintCallable) void CloseShop();
    UPROPERTY(BlueprintReadOnly) TArray<FICItemAmount> Cart;
    UPROPERTY(BlueprintReadOnly) bool bSelling=false;
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    FName Service;
    TArray<FName> Items;
    TMap<FName,int32> Prices;
    int32 Selected=0;
    bool bSubmitting=false;
    void Refresh();
    UFUNCTION() void BuyTab(){SelectTab(false);}
    UFUNCTION() void SellTab(){SelectTab(true);}
    UFUNCTION() void Checkout(){SubmitCart();}
};
