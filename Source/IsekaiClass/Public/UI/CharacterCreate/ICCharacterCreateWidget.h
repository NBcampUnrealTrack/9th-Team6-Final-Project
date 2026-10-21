#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "ICCharacterCreateWidget.generated.h"
class UButton;
class UEditableTextBox;
UCLASS(Blueprintable)
class ISEKAICLASS_API UICCharacterCreateWidget : public UICScreenWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void RefreshState();
    UFUNCTION(BlueprintCallable) void SelectAnswer(int32 Index);
    UFUNCTION(BlueprintCallable) void Next();
    UFUNCTION(BlueprintCallable) void Back();
    UFUNCTION(BlueprintPure) int32 GetStep() const
    {
        return Step;
    }

protected:
    virtual void NativeOnInitialized() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual void GoBack() override;

private:
    int32 Step = 0;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> NameInput;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ChoiceButtons;
    void StoreName();
    void ConfirmStart();
    UFUNCTION() void Choice0();
    UFUNCTION() void Choice1();
    UFUNCTION() void Choice2();
    UFUNCTION() void Choice3();
    UFUNCTION() void NameChanged(const FText& Value);
};
