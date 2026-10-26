#include "UI/Quest/ICQuestTrackerWidget.h"
#include "UI/Framework/ICText.h"

#include "Components/TextBlock.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Text/ICTextLibrary.h"
#include "Internationalization/StringTable.h"
#include "Quest/ICQuestSubsystem.h"
#include "UI/Quest/ICQuestDisplay.h"
#include "World/State/ICWorldStateSubsystem.h"

void UICQuestTrackerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsDesignTime() || !GetGameInstance()) return;

    if (UICWorldStateSubsystem* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>())
    {
        State->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshQuests);
        State->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::RefreshQuests);
    }
    RefreshQuests();
}

void UICQuestTrackerWidget::NativeDestruct()
{
    if (UICWorldStateSubsystem* State = GetGameInstance() ? GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>() : nullptr)
    {
        State->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshQuests);
        State->OnWorldRestored.RemoveDynamic(this, &ThisClass::RefreshQuests);
    }
    Super::NativeDestruct();
}

void UICQuestTrackerWidget::RefreshQuests()
{
    if (!GetGameInstance() || !QuestText) return;
    UICWorldStateSubsystem* State = GetGameInstance()->GetSubsystem<UICWorldStateSubsystem>();
    UICDefinitionSubsystem* Definitions = GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();
    UICQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
    UStringTable* Strings = LoadObject<UStringTable>(nullptr,
        TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Quest.ST_IC_Quest"));
    if (!State || !Definitions || !Quests || !Strings)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const FICWorldState Snapshot = State->ExportState();
    TArray<FName> QuestIDs;
    FICQuestDisplay::GetActiveQuestIDs(Snapshot, QuestIDs);

    TArray<FText> Lines;
    for (const FName QuestID : QuestIDs)
    {
        FICQuestRow Definition;
        if (!Definitions->GetQuest(QuestID, Definition).bSuccess)
        {
            continue;
        }

        const FICTextLookupResult Name = UICTextLibrary::ResolveText(Strings, Definition.NameKey);
        const FICQuestState& Quest = Snapshot.Quests.FindChecked(QuestID);
        Lines.Add(Name.bSuccess ? Name.Text : FText::FromName(QuestID));
        for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
        {
            for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
            {
                const FICTextLookupResult Description = UICTextLibrary::ResolveText(Strings, Objective.DescriptionKey);
                if (Description.bSuccess)
                {
                    Lines.Add(FText::Format(ICText::Get(TEXT("Native_4dfde083f2db")),
                        FICQuestDisplay::FormatObjective(Description.Text, Objective, Quest, Snapshot, Quests)));
                }
            }
        }
    }

    QuestText->SetText(FText::Join(FText::FromString(TEXT("\n")), Lines));
    SetVisibility(Lines.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
