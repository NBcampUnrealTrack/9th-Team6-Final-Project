#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICSettingsWidget.generated.h"

class USlider;
class UCheckBox;
class UComboBoxString;

UCLASS()
class ISEKAICLASS_API UICSettingsWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<USlider> Sensitivity;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> Fullscreen;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> Quality;
    UFUNCTION() void Apply();
    UFUNCTION() void Close();
};
