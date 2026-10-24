#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICDialogueEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICDialogueEntryPressed, FName, EntryID);

UCLASS(Blueprintable)
class ISEKAICLASS_API UICDialogueEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetEntry(FName InEntryID, const FText& InLabel, bool IsEnabled = true);
    UFUNCTION(BlueprintPure, Category="IC|Dialogue") FText GetEntryText() const;
    UFUNCTION(BlueprintCallable, Category="IC|Dialogue") void FocusEntry();
    bool CanSelectEntry() const;
    bool HasEntryFocus() const;
    FName GetEntryID() const { return EntryID; }
    void ActivateEntry();
    void SetSelected(bool Selected);
    UPROPERTY(BlueprintReadOnly, Category="IC|Dialogue") bool bSelected = false;

    UPROPERTY(BlueprintAssignable, Category="IC|Dialogue")
    FICDialogueEntryPressed OnEntryPressed;
    UPROPERTY(BlueprintAssignable, Category="IC|Dialogue") FICDialogueEntryPressed OnEntryFocused;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnAddedToFocusPath(const FFocusEvent& FocusEvent) override;

private:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> EntryButton;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> EntryLabel;

    FName EntryID;

    UFUNCTION()
    void HandleClicked();
    UFUNCTION() void HandleHovered();
};
