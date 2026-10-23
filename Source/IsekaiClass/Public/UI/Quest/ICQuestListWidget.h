#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICQuestListWidget.generated.h"

class UStringTable;
class UTextBlock;
class UButton;
struct FICQuestRow;
struct FICQuestState;
struct FICWorldState;
class UICQuestSubsystem;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuestListWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetQuestData(const FICQuestRow& Definition, const FICQuestState& Quest, const FICWorldState& World,
        const UICQuestSubsystem* Quests, UStringTable* Strings);

private:
    FName DisplayedQuestID;
    bool bReaccept = false;

    // Existing BP layouts remain usable; a native button is added when absent.
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UButton> QuestAbandonButton;

    void UpdateAction(const FICQuestRow& Definition, const FICQuestState& Quest);
    UFUNCTION() void RequestQuestAction();

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> QuestNameText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> QuestDescriptionText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> QuestObjectivesText;
};
