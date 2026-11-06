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
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual void NativeDestruct() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
    uint64 FirstPoseFrame=MAX_uint64;
    double WorldWarmupStarted=0.0;
    TWeakObjectPtr<class UAnimInstance> PreparedAnimation;
    UFUNCTION() void Refresh();
    UFUNCTION() void Retry();
    UFUNCTION() void Title();
};
