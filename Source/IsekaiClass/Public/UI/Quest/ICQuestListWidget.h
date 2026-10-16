#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICQuestListWidget.generated.h"

class UStringTable;
class UTextBlock;
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
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> QuestNameText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> QuestDescriptionText;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> QuestObjectivesText;
};
