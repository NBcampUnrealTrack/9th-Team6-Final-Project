#include "UI/Quest/ICQuestWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Debug/ICLog.h"
#include "InputCoreTypes.h"
#include "Internationalization/StringTable.h"
#include "UI/Quest/ICQuestListWidget.h"
#include "World/State/ICWorldStateSubsystem.h"

void UICQuestWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    IC_Quest_Close->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseQuest);
    RefreshQuests();
}

void UICQuestWidget::NativeDestruct()
{
    IC_Quest_Close->OnClicked.RemoveDynamic(this, &ThisClass::CloseQuest);
    Super::NativeDestruct();
}

void UICQuestWidget::RefreshQuests()
{
    QuestEntries->ClearChildren();
    const AICPlayerController* Controller = Cast<AICPlayerController>(GetOwningPlayer());
    UICWorldStateSubsystem* State = Controller ? Controller->GetWorldState() : nullptr;
    if (!State || !GetGameInstance())
    {
        return;
    }

    const FICWorldState Snapshot = State->ExportState();
    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UStringTable* Strings = QuestStringTable.LoadSynchronous();
    UClass* ItemClass = QuestItemWidgetClass.LoadSynchronous();
    TArray<FName> QuestIDs;
    for (const TPair<FName, FICQuestState>& Entry : Snapshot.Quests)
    {
        if (Entry.Value.bAccepted && !Entry.Value.bCompleted)
        {
            QuestIDs.Add(Entry.Key);
        }
    }
    QuestIDs.Sort(FNameLexicalLess());

    for (FName QuestID : QuestIDs)
    {
        FICQuestRow Definition;
        if (!Definitions || !Definitions->GetQuest(QuestID, Definition).bSuccess)
        {
            UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.DefinitionMissing] Quest=%s"), *QuestID.ToString());
            continue;
        }
        if (ItemClass)
        {
            UICQuestListWidget* Item = CreateWidget<UICQuestListWidget>(GetOwningPlayer(), ItemClass);
            if (Item)
            {
                Item->SetQuestData(Definition, Strings);
                QuestEntries->AddChildToVerticalBox(Item);
            }
        }
    }

    if (QuestIDs.IsEmpty())
    {
        UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Empty->SetText(EmptyQuestMessage);
        QuestEntries->AddChildToVerticalBox(Empty);
    }
}

FReply UICQuestWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
    if (KeyEvent.GetKey() == EKeys::Escape)
    {
        CloseQuest();
        return FReply::Handled();
    }

    return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

void UICQuestWidget::CloseQuest()
{
    if (AICPlayerController* Controller = Cast<AICPlayerController>(GetOwningPlayer()))
    {
        Controller->SetMenu(TEXT(""));
    }
}
