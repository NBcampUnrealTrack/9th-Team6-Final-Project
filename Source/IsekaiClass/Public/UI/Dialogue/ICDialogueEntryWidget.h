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

    UPROPERTY(BlueprintAssignable, Category="IC|Dialogue")
    FICDialogueEntryPressed OnEntryPressed;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> EntryButton;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> EntryLabel;

    FName EntryID;

    UFUNCTION()
    void HandleClicked();
};
