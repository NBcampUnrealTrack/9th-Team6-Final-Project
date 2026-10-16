#include "UI/Quest/ICQuestWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Quest/ICQuestSubsystem.h"
#include "Debug/ICLog.h"
#include "InputCoreTypes.h"
#include "Internationalization/StringTable.h"
#include "UI/Quest/ICQuestListWidget.h"
#include "UI/Quest/ICQuestDisplay.h"
#include "World/State/ICWorldStateSubsystem.h"

void UICQuestWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    IC_Quest_Close->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseQuest);
    if (UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>())
    {
        Quests->RefreshAvailableQuests();
    }
    RefreshQuests();
    if (UICWorldStateSubsystem* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>())
    {
        State->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshQuests);
        State->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::RefreshQuests);
    }
}

void UICQuestWidget::NativeDestruct()
{
    if (UICWorldStateSubsystem* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>())
    {
        State->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshQuests);
        State->OnWorldRestored.RemoveDynamic(this, &ThisClass::RefreshQuests);
    }
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

    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    const FICWorldState Snapshot = State->ExportState();
    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UStringTable* Strings = QuestStringTable.LoadSynchronous();
    UClass* ItemClass = QuestItemWidgetClass.LoadSynchronous();
    TArray<FName> QuestIDs;
    FICQuestDisplay::GetActiveQuestIDs(Snapshot, QuestIDs);

    for (FName QuestID : QuestIDs)
    {
        FICQuestRow Definition;
        if (!Definitions || !Definitions->GetQuest(QuestID, Definition).bSuccess)
        {
            UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.DefinitionMissing] Quest=%s"), *QuestID.ToString());
            continue;
        }
        if (ItemClass && Quests)
        {
            UICQuestListWidget* Item = CreateWidget<UICQuestListWidget>(GetOwningPlayer(), ItemClass);
            if (Item)
            {
                Item->SetQuestData(Definition, Snapshot.Quests.FindChecked(QuestID), Snapshot, Quests, Strings);
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
