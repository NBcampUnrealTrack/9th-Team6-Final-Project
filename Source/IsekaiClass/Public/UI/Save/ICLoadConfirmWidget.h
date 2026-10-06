#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICLoadConfirmWidget.generated.h"
class UButton;
class UTextBlock;
class UImage;
class UTexture2D;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICLoadConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICLoadCancelled);
UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICLoadConfirmWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICLoadConfirmed OnConfirmed;
    UPROPERTY(BlueprintAssignable) FICLoadCancelled OnCancelled;
    void Configure(const FICSaveSlotInfo& InInfo, UTexture2D* InThumbnail);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_LoadConfirm_Confirm;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_LoadConfirm_Cancel;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_LoadConfirm_Thumbnail;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_LoadConfirm_Summary;
private:
    UPROPERTY() FICSaveSlotInfo Info;
    UPROPERTY() TObjectPtr<UTexture2D> Thumbnail;
    UFUNCTION() void HandleConfirm();
    UFUNCTION() void HandleCancel();
};
