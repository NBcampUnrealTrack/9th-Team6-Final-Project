#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "UI/CharacterCreate/ICNameComposer.h"
#include "ICCharacterCreateWidget.generated.h"
class UButton;
class UEditableTextBox;
class UICNameKeyButton;
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
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual void GoBack() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;

private:
    friend struct FICKeyboardTestAccess;
    int32 Step = 0;
    bool bStartingNewGame = false;
    bool bKorean=true,bShift=false,bUpdatingName=false;
    int32 KeyboardIndex=0;
    FICNameComposer NameComposer;
    UPROPERTY(Transient) TArray<TObjectPtr<UICNameKeyButton>> NameKeys;
    void RefreshKeyboard();
    UFUNCTION() void ChooseKey(int32 Index);
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> NameInput;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ChoiceButtons;
    void StoreName();
    void StartGame();
    UFUNCTION() void Choice0();
    UFUNCTION() void Choice1();
    UFUNCTION() void Choice2();
    UFUNCTION() void Choice3();
    UFUNCTION() void NameChanged(const FText& Value);
};
