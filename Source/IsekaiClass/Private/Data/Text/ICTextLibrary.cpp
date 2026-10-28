#include "Data/Text/ICTextLibrary.h"
#include "Data/Text/ICLocalization.h"
#include "Debug/ICLog.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"

bool UICTextLibrary::ReplaceTextSources(UStringTable* Table,const TMap<FString,FString>& Sources)
{
    IC_TRACE_FUNCTION();
#if WITH_EDITOR
    if(!IsValid(Table)){UE_LOG(LogICData, Verbose, TEXT("[Text.Replace.Rejected] String Table 누락"));return false;}
    for(const auto& Entry:Sources)if(Entry.Key.TrimStartAndEnd().IsEmpty()||Entry.Value.TrimStartAndEnd().IsEmpty()){UE_LOG(LogICData, Verbose, TEXT("[Text.Replace.Rejected] Table=%s Key=%s: 빈 키 또는 원문"),*GetPathNameSafe(Table),*Entry.Key);return false;}
    Table->Modify();const auto Texts=Table->GetMutableStringTable();Texts->ClearSourceStrings();
    for(const auto& Entry:Sources)Texts->SetSourceString(FTextKey(Entry.Key),Entry.Value,FString());
    Table->MarkPackageDirty();UE_LOG(LogICData, Log, TEXT("[Text.Replace] Table=%s Entries=%d"),*Table->GetPathName(),Sources.Num());return true;
#else
    return false;
#endif
}

FICTextLookupResult UICTextLibrary::ResolveText(UStringTable* Table, const FString& Key)
{
    IC_TRACE_FUNCTION();
    FICTextLookupResult Result;
    if (!IsValid(Table))
        Result.FailureReason = TEXT("String Table is missing.");
    else if (Key.IsEmpty())
        Result.FailureReason = TEXT("String Table key is empty.");
    else
    {
        FString Source;
        if (!Table->GetStringTable()->GetSourceString(FTextKey(Key), Source))
            Result.FailureReason = FString::Printf(TEXT("Key '%s' does not exist in '%s'."), *Key, *Table->GetPathName());
        else if (Source.TrimStartAndEnd().IsEmpty())
            Result.FailureReason = FString::Printf(TEXT("Key '%s' has an empty source string in '%s'."), *Key, *Table->GetPathName());
        else
        {
            Result.Text = ICLocalization::Localize(FText::FromStringTable(Table->GetStringTableId(), Key));
            Result.bSuccess = true;
        }
    }
    if(!Result.bSuccess) UE_LOG(LogICData, Verbose, TEXT("[Text.Rejected] Table=%s Key=%s Reason=%s"),*GetPathNameSafe(Table),*Key,*Result.FailureReason);
    return Result;
}

