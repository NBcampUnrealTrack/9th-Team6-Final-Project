#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICNPCDialogueWidget.generated.h"

class AICNPCBase;
class UICDialogueEntryWidget;
class UStringTable;
class UTextBlock;
class UVerticalBox;
class UICAffinityHeartsWidget;
class UICWorldStateSubsystem;
class UICNPCGiftWidget;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICNPCDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void OpenForNPC(AICNPCBase* InNPC);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> SpeakerText;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UICAffinityHeartsWidget> AffinityHearts;

    UPROPERTY(Transient)
    TObjectPtr<UICWorldStateSubsystem> BoundState;

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
    FGuid DialogueEventID;
    bool bShowingInteractionOptions = false;
    bool bRecruitEntryShown = false;
    bool bDismissEntryShown = false;
    bool bUpdatingOptions = false;
    bool bEntryFocusPending = false;
    TWeakObjectPtr<UICDialogueEntryWidget> SelectedEntry;
    UPROPERTY(Transient) TObjectPtr<UICNPCGiftWidget> GiftPicker;

    void ShowInteractionOptions();
    void ShowInitialDialogue();
    void ShowGiftPicker();
    UFUNCTION() void GiftPickerClosed(const FText& Message);
    void ShowQuestList();
    void AddQuestEntries();
    void ShowQuest(FName QuestID);
    void ShowNode(FName DialogueID, FName NodeID = NAME_None);
    void AddEntry(FName EntryID, const FText& Label, bool IsEnabled = true);
    void ClearEntries();
    void SelectEntry(UICDialogueEntryWidget* Entry, bool RequestFocus = true);
    void MoveSelection(int32 Direction);
    void CloseDialogue();
    void CompleteDialogue(bool IsTerminalChoice = false);

    UFUNCTION() void RefreshAffinity();
    UFUNCTION() void CharacterChanged(FName CharacterID);

    UFUNCTION()
    void HandleEntry(FName EntryID);
    UFUNCTION() void HandleEntryFocused(FName EntryID);
};
