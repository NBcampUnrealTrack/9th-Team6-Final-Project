#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICSettingsWidget.generated.h"

class USlider;
class UCheckBox;
class UComboBoxString;

/** Runtime behavior for WBP_IC_Settings; the Widget Blueprint owns the layout. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICSettingsWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    void RefreshLocalizedQualityOptions();
    UPROPERTY(Transient) TObjectPtr<USlider> Sensitivity;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> Fullscreen;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> Quality;
    UFUNCTION() void Apply();
    UFUNCTION() void Close();
};
