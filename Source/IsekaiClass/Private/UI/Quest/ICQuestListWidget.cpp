#include "UI/Quest/ICQuestListWidget.h"
#include "Components/TextBlock.h"
#include "Data/Rows/Quest/ICQuestRow.h"
#include "Data/Text/ICTextLibrary.h"
#include "Debug/ICLog.h"
#include "Internationalization/StringTable.h"
#include "Quest/ICQuestSubsystem.h"
#include "UI/Quest/ICQuestDisplay.h"
#include "World/State/ICWorldStateSubsystem.h"

void UICQuestListWidget::SetQuestData(const FICQuestRow& Definition, const FICQuestState& Quest,
    const FICWorldState& World, const UICQuestSubsystem* Quests, UStringTable* Strings)
{
    const FICTextLookupResult NameResult = UICTextLibrary::ResolveText(Strings, Definition.NameKey);
    if (!NameResult.bSuccess)
    {
        UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.TextMissing] Quest=%s Field=Name Key=%s Reason=%s"),
            *Definition.ID.ToString(), *Definition.NameKey, *NameResult.FailureReason);
    }
    QuestNameText->SetText(NameResult.Text);

    const FICTextLookupResult DescriptionResult = UICTextLibrary::ResolveText(Strings, Definition.DescriptionKey);
    if (!DescriptionResult.bSuccess)
    {
        UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.TextMissing] Quest=%s Field=Description Key=%s Reason=%s"),
            *Definition.ID.ToString(), *Definition.DescriptionKey, *DescriptionResult.FailureReason);
    }
    QuestDescriptionText->SetText(DescriptionResult.Text);

    TArray<FText> ObjectiveLines;
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
        {
            const FICTextLookupResult Result = UICTextLibrary::ResolveText(Strings, Objective.DescriptionKey);
            if (!Result.bSuccess)
            {
                UE_LOG(LogICUI, Warning, TEXT("[Quest.UI.TextMissing] Quest=%s Objective=%s Key=%s Reason=%s"),
                    *Definition.ID.ToString(), *Objective.ObjectiveID.ToString(), *Objective.DescriptionKey, *Result.FailureReason);
                continue;
            }
            ObjectiveLines.Add(FICQuestDisplay::FormatObjective(Result.Text, Objective, Quest, World, Quests));
        }
    }
    QuestObjectivesText->SetText(FText::Join(FText::FromString(TEXT("\n")), ObjectiveLines));
}
