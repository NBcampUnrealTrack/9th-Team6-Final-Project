#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "ICConfirmPopupWidget.generated.h"
UCLASS(Blueprintable)
class ISEKAICLASS_API UICConfirmPopupWidget : public UICScreenWidget
{
    GENERATED_BODY()
public:
    UICConfirmPopupWidget(const FObjectInitializer& Initializer);
    void Configure(const FText& Message, FSimpleDelegate Accept);

protected:
    virtual void NativeOnInitialized() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual void GoBack() override;

private:
    FSimpleDelegate Accepted;
    UFUNCTION() void Accept();
    UFUNCTION() void Cancel();
};
