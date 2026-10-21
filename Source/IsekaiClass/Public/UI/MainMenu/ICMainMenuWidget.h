#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "ICMainMenuWidget.generated.h"
class UButton;
UCLASS(Blueprintable)
class ISEKAICLASS_API UICMainMenuWidget : public UICScreenWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void FocusStartButton();

protected:
    virtual void NativeOnInitialized() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
    UPROPERTY(Transient) TObjectPtr<UButton> StartButton;
    UFUNCTION() void Start();
    UFUNCTION() void Quit();
};
