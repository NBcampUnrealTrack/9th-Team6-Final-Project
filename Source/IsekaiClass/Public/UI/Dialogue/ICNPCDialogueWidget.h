#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICNPCDialogueWidget.generated.h"

class AICNPCBase;
class UICDialogueEntryWidget;
class UStringTable;
class UTextBlock;
class UVerticalBox;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICNPCDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void OpenForNPC(AICNPCBase* InNPC);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> SpeakerText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> DialogueText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UVerticalBox> Entries;

    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue")
    TSubclassOf<UICDialogueEntryWidget> EntryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text")
    TObjectPtr<UStringTable> NPCStringTable;

    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text")
    TObjectPtr<UStringTable> QuestStringTable;

    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text")
    TObjectPtr<UStringTable> DialogueStringTable;

    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText QuestPromptText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText QuestEntryFormatText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText CloseText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText BackText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText NextText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText QuestsText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText AcceptText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText TurnInText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText ReadyStatusText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText ActiveStatusText;
    UPROPERTY(EditDefaultsOnly, Category="IC|Dialogue|Text") FText OfferStatusText;

    UPROPERTY(Transient)
    TObjectPtr<AICNPCBase> NPC;

    FName SelectedQuestID;
    FName SelectedDialogueID;
    FName SelectedNodeID;
    bool IsShowingInteractionOptions = false;

    void ShowInteractionOptions();
    void ShowInitialDialogue();
    void ShowQuestList();
    void AddQuestEntries(bool IsOnlyActionable);
    void ShowQuest(FName QuestID);
    void ShowNode(FName DialogueID, FName NodeID = NAME_None);
    void AddEntry(FName EntryID, const FText& Label, bool IsEnabled = true);
    void CloseDialogue();

    UFUNCTION()
    void HandleEntry(FName EntryID);
};
