#include "NPC/Dialogue/ICDialogueLibrary.h"
#include "Debug/ICLog.h"
#include "Data/Text/ICTextLibrary.h"
#include "Data/Validation/ICDefinitionLibrary.h"
#include "Engine/DataTable.h"

FICDialogueDefinitionResult UICDialogueLibrary::GetDialogueDefinition(UDataTable* Table, FName DialogueID, bool bAllowTestContent)
{
    IC_TRACE_FUNCTION();
    FICDialogueDefinitionResult Result;
    auto Finish=[&]()
    {
        IC_TRACE_FUNCTION();
        UE_LOG(LogICData, Verbose, TEXT("[Dialogue.Definition] Table=%s Dialogue=%s Success=%d Reason=%s"),
            *GetPathNameSafe(Table),*DialogueID.ToString(),Result.bSuccess,*Result.FailureReason);
        return Result;
    };
    if (!IsValid(Table) || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FICDialogueRow::StaticStruct()))
        Result.FailureReason = TEXT("Expected a DataTable using FICDialogueRow.");
    else if (!UICDefinitionLibrary::IsDefinitionIDValid(DialogueID))
        Result.FailureReason = TEXT("Dialogue ID is empty or invalid.");
    else
    {
        const FICDialogueRow* Row = Table->FindRow<FICDialogueRow>(DialogueID, TEXT("IC dialogue lookup"), false);
        if (!Row)
            Result.FailureReason = FString::Printf(TEXT("Dialogue '%s' is missing in '%s'."), *DialogueID.ToString(), *Table->GetPathName());
        else if (Row->ID != DialogueID)
            Result.FailureReason = TEXT("Dialogue RowName and ID do not match.");
        else if (Row->bDeprecated || (Row->bTestOnly && !bAllowTestContent))
            Result.FailureReason = TEXT("Dialogue is deprecated or test content is not allowed.");
        else if (Row->EntryNodeID.IsNone() || Row->Nodes.IsEmpty())
            Result.FailureReason = TEXT("Dialogue has no entry node or nodes.");
        else
        {
            TSet<FName> NodeIDs;
            for (const FICDialogueNode& Node : Row->Nodes)
            {
                if (Node.NodeID.IsNone() || NodeIDs.Contains(Node.NodeID))
                { Result.FailureReason = TEXT("Dialogue contains an empty or duplicate node ID."); return Finish(); }
                NodeIDs.Add(Node.NodeID);
            }
            if (!NodeIDs.Contains(Row->EntryNodeID))
            { Result.FailureReason = TEXT("Dialogue entry node does not exist."); return Finish(); }
            for (const FICDialogueNode& Node : Row->Nodes)
            {
                if (!Node.NextNodeID.IsNone() && !NodeIDs.Contains(Node.NextNodeID))
                { Result.FailureReason = TEXT("Dialogue contains a missing next node."); return Finish(); }
                TSet<FName> ChoiceIDs;
                for (const FICDialogueChoice& Choice : Node.Choices)
                {
                    if (Choice.ChoiceID.IsNone() || ChoiceIDs.Contains(Choice.ChoiceID))
                    { Result.FailureReason = TEXT("Dialogue contains an empty or duplicate choice ID."); return Finish(); }
                    if (!Choice.NextNodeID.IsNone() && !NodeIDs.Contains(Choice.NextNodeID))
                    { Result.FailureReason = TEXT("Dialogue choice targets a missing node."); return Finish(); }
                    ChoiceIDs.Add(Choice.ChoiceID);
                }
            }
            Result.Definition = *Row;
            Result.bSuccess = true;
        }
    }
    return Finish();
}

FICDialogueNodeResult UICDialogueLibrary::ResolveDialogueNode(UDataTable* Table, UStringTable* Strings, FName DialogueID, FName NodeID, bool bAllowTestContent)
{
    IC_TRACE_FUNCTION();
    FICDialogueNodeResult Failure;
    auto FinishFailure=[&]()
    {
        IC_TRACE_FUNCTION();
        UE_LOG(LogICData, Verbose, TEXT("[Dialogue.Node.Rejected] Table=%s Dialogue=%s Node=%s Reason=%s"),
            *GetPathNameSafe(Table),*DialogueID.ToString(),*NodeID.ToString(),*Failure.FailureReason);
        return Failure;
    };
    const FICDialogueDefinitionResult Definition = GetDialogueDefinition(Table, DialogueID, bAllowTestContent);
    if (!Definition.bSuccess) { Failure.FailureReason = Definition.FailureReason; return FinishFailure(); }
    const FName SelectedID = NodeID.IsNone() ? Definition.Definition.EntryNodeID : NodeID;
    const FICDialogueNode* Node = Definition.Definition.Nodes.FindByPredicate([SelectedID](const FICDialogueNode& Entry) { IC_TRACE_FUNCTION(); return Entry.NodeID == SelectedID; });
    if (!Node)
    { Failure.FailureReason = FString::Printf(TEXT("Node '%s' is missing in dialogue '%s'."), *SelectedID.ToString(), *DialogueID.ToString()); return FinishFailure(); }
    const FICTextLookupResult Line = UICTextLibrary::ResolveText(Strings, Node->TextKey);
    if (!Line.bSuccess) { Failure.FailureReason = Line.FailureReason; return FinishFailure(); }
    FICDialogueNodeResult Candidate;
    Candidate.DialogueID = DialogueID;
    Candidate.NodeID = Node->NodeID;
    Candidate.SpeakerNPCID = Node->SpeakerNPCID;
    Candidate.Text = Line.Text;
    Candidate.NextNodeID = Node->NextNodeID;
    for (const FICDialogueChoice& Choice : Node->Choices)
    {
        const FICTextLookupResult ChoiceText = UICTextLibrary::ResolveText(Strings, Choice.TextKey);
        if (!ChoiceText.bSuccess) { Failure.FailureReason = ChoiceText.FailureReason; return FinishFailure(); }
        FICResolvedDialogueChoice Resolved;
        Resolved.ChoiceID = Choice.ChoiceID;
        Resolved.Text = ChoiceText.Text;
        Resolved.NextNodeID = Choice.NextNodeID;
        Candidate.Choices.Add(MoveTemp(Resolved));
    }
    Candidate.bSuccess = true;
    UE_LOG(LogICData, Verbose, TEXT("[Dialogue.Node.Resolved] Dialogue=%s Node=%s Speaker=%s Choices=%d"),*DialogueID.ToString(),*Candidate.NodeID.ToString(),*Candidate.SpeakerNPCID.ToString(),Candidate.Choices.Num());
    return Candidate;
}
