#include "UI/Quest/ICQuestDisplay.h"
#include "UI/Framework/ICText.h"

#include "Quest/ICQuestSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

void FICQuestDisplay::GetActiveQuestIDs(const FICWorldState& World, TArray<FName>& QuestIDs)
{
    for (const TPair<FName, FICQuestState>& Entry : World.Quests)
    {
        if (Entry.Value.bAccepted && Entry.Value.bStarted && !Entry.Value.bCompleted
            && !Entry.Value.bAbandoned)
        {
            QuestIDs.Add(Entry.Key);
        }
    }
    QuestIDs.Sort(FNameLexicalLess());
}

FText FICQuestDisplay::FormatObjective(const FText& Description, const FICQuestObjectiveDefinition& Objective,
    const FICQuestState& Quest, const FICWorldState& World, const UICQuestSubsystem* Quests)
{
    int32 CurrentCount = 0;
    if (!Quests || !Quests->GetObjectiveProgress(Objective, Quest, World, CurrentCount))
    {
        return Description;
    }

    return FText::Format(ICText::Get(TEXT("Native_516e57ce9608")),
        Description, FText::AsNumber(CurrentCount), FText::AsNumber(Objective.RequiredCount));
}
