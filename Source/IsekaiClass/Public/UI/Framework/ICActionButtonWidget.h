#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ICActionButtonWidget.generated.h"
class UTextBlock;

/** Repeated list entries keep their button, caption, spacing and style in their WBP. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICActionButtonWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnButtonClickedEvent OnClicked;
    void SetCaption(const FText& Text);
    void SetSelected(bool Value);
protected:
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> BTN_Select;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TXT_Label;
    virtual void NativeOnInitialized() override;
    virtual void NativeOnAddedToFocusPath(const FFocusEvent& Event) override;
    virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& Event) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    UFUNCTION() void Clicked(){OnClicked.Broadcast();}
};
