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
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
    UPROPERTY(Transient) TObjectPtr<UButton> StartButton;
    int32 Selected=0;
    UFUNCTION() void Start();
    UFUNCTION() void Continue();
    UFUNCTION() void SaveList();
    UFUNCTION() void Settings();
    UFUNCTION() void Quit();
};
