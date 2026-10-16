#include "Data/Validation/ICTextValidationLibrary.h"
#include "Debug/ICLog.h"
#include "Data/Text/ICTextLibrary.h"
#include "NPC/Dialogue/ICDialogueLibrary.h"
#include "Engine/DataTable.h"
#include "Internationalization/StringTable.h"

TArray<FICDefinitionIssue> UICTextValidationLibrary::ValidateDialogueText(UDataTable* DialogueTable, UStringTable* Strings)
{
    IC_TRACE_FUNCTION();
    TArray<FICDefinitionIssue> Issues;
    auto AddIssue = [&](FName Row, const FString& Field, const FString& Reason)
    {
        IC_TRACE_FUNCTION();
        FICDefinitionIssue Issue;
        Issue.Table = IsValid(DialogueTable) ? DialogueTable->GetPathName() : TEXT("<missing dialogue table>");
        Issue.Row = Row; Issue.Field = Field; Issue.Reason = Reason;
        UE_LOG(LogICData, Log, TEXT("[DialogueText.Validation.Issue] Table=%s Row=%s Field=%s Reason=%s"),*Issue.Table,*Row.ToString(),*Field,*Reason);
        Issues.Add(MoveTemp(Issue));
    };
    if (!IsValid(DialogueTable) || !DialogueTable->GetRowStruct() || !DialogueTable->GetRowStruct()->IsChildOf(FICDialogueRow::StaticStruct()))
    { AddIssue(NAME_None, TEXT("RowStruct"), TEXT("Expected FICDialogueRow.")); return Issues; }
    if (!IsValid(Strings))
        AddIssue(NAME_None, TEXT("StringTable"), TEXT("Dialogue String Table is missing."));
    FICDefinitionTable Binding;
    Binding.Type = EICDefinitionType::Dialogue; Binding.Table = DialogueTable;
    // Partial mode does not require NPCs and other definitions to be complete to check dialogue authoring.
    Issues.Append(UICDefinitionLibrary::GetValidationIssues({Binding}, false));
    for (const FName RowName : DialogueTable->GetRowNames())
    {
        const FICDialogueRow* Row = DialogueTable->FindRow<FICDialogueRow>(RowName, TEXT("IC dialogue text validation"), false);
        if (!Row || Row->bDeprecated) continue;
        auto CheckKey = [&](const FString& Key, const FString& Field)
        {
            IC_TRACE_FUNCTION();
            const FICTextLookupResult Text = UICTextLibrary::ResolveText(Strings, Key);
            if (!Text.bSuccess) AddIssue(RowName, Field, Text.FailureReason);
        };
        for (int32 Index = 0; Index < Row->Nodes.Num(); ++Index)
        {
            const FICDialogueNode& Node = Row->Nodes[Index];
            CheckKey(Node.TextKey, FString::Printf(TEXT("Nodes[%d].TextKey"), Index));
            for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
                CheckKey(Node.Choices[ChoiceIndex].TextKey, FString::Printf(TEXT("Nodes[%d].Choices[%d].TextKey"), Index, ChoiceIndex));
        }
    }
    UE_LOG(LogICData, Log, TEXT("[DialogueText.Validation.Complete] Table=%s StringTable=%s Issues=%d"),*GetPathNameSafe(DialogueTable),*GetPathNameSafe(Strings),Issues.Num());
    return Issues;
}
