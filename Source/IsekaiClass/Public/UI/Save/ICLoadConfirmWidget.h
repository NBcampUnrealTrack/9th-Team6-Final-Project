#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICLoadConfirmWidget.generated.h"
class UButton;
class UTextBlock;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICLoadConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICLoadCancelled);
UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICLoadConfirmWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICLoadConfirmed OnConfirmed;
    UPROPERTY(BlueprintAssignable) FICLoadCancelled OnCancelled;
    void Configure(const FICSaveSlotInfo& InInfo);
    void ConfigurePrompt(const FText& Text);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent) override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_LoadConfirm_Confirm;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_LoadConfirm_Cancel;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_LoadConfirm_Summary;
private:
    FText Prompt;
    UPROPERTY() FICSaveSlotInfo Info;
    UFUNCTION() void HandleConfirm();
    UFUNCTION() void HandleCancel();
};
