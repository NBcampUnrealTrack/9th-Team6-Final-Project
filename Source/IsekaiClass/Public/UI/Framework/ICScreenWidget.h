#pragma once
#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "ICScreenWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICScreenWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()
public:
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override
    {
        return TOptional<FUIInputConfig>();
    }

protected:
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual bool NativeOnHandleBackAction() override;
    virtual void GoBack();
};
