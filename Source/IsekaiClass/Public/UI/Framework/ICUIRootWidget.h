#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICUIRootWidget.generated.h"

class UOverlay;
class UCanvasPanel;
class UCommonActivatableWidgetStack;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICUIRootWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Mount(UUserWidget* Widget, int32 ZOrder);
    void Unmount(UUserWidget* Widget);
    void SetHUD(UUserWidget* Widget);

protected:
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY(Transient) TObjectPtr<UOverlay> HUDLayer;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> LegacyLayer;
    UPROPERTY(Transient) TObjectPtr<UCommonActivatableWidgetStack> ScreenLayer;
    UPROPERTY(Transient) TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;
    UPROPERTY(Transient) TObjectPtr<UCommonActivatableWidgetStack> LoadingLayer;
};
