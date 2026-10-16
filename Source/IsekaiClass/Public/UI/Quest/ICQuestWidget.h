#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICQuestWidget.generated.h"

class UButton;
class UVerticalBox;
class UICQuestListWidget;
class UStringTable;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuestWidget : public UUserWidget
{
    GENERATED_BODY()

public:
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UVerticalBox> QuestEntries;

    UPROPERTY(EditDefaultsOnly, Category="IC|UI")
    TSoftClassPtr<UICQuestListWidget> QuestItemWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="IC|UI")
    TSoftObjectPtr<UStringTable> QuestStringTable;

    UPROPERTY(EditDefaultsOnly, Category="IC|UI")
    FText EmptyQuestMessage;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UButton> IC_Quest_Close;

    void RefreshQuests();

    UFUNCTION()
    void CloseQuest();
};
