#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICCharacterCreateWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
class UButton;

UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICCharacterCreateWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void RefreshState();
    UFUNCTION(BlueprintCallable, Category="Character Creation") void SelectPreset(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Character Creation") void SelectJob(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Character Creation") void CreateCharacter();
    UFUNCTION(BlueprintCallable, Category="Character Creation") void OpenLoadScreen();
    UFUNCTION(BlueprintCallable, Category="Character Creation") void OpenQuitScreen();
    UFUNCTION(BlueprintPure, Category="Character Creation") int32 GetSelectedPreset() const;
    UFUNCTION(BlueprintPure, Category="Character Creation") int32 GetSelectedJob() const;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UEditableTextBox> IC_Create_Name;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_Create_Confirm;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_Create_Notice;
    // Style selected gender/job buttons in Blueprint using this event.
    UFUNCTION(BlueprintImplementableEvent, Category="Character Creation")
    void OnCreationStateChanged(int32 Preset, int32 Job, bool bCanEdit);
private:
    UFUNCTION() void HandleNameChanged(const FText& Text);
};
