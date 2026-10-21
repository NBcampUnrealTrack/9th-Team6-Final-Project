#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "ICLoadingWidget.generated.h"
UCLASS(Blueprintable)
class ISEKAICLASS_API UICLoadingWidget : public UICScreenWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
    UFUNCTION() void Refresh();
    UFUNCTION() void Retry();
    UFUNCTION() void Title();
};
