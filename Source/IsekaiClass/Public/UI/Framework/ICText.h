#pragma once
#include "CoreMinimal.h"

/** One localization source for native UI labels and gameplay notices. */
namespace ICText
{
    inline FText Get(const TCHAR* Key)
    {
        return FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_UI.ST_IC_UI"), Key);
    }
    inline FFormatArgumentValue Argument(const TCHAR* Value) { return FText::FromString(Value); }
    inline FFormatArgumentValue Argument(const FString& Value) { return FText::FromString(Value); }
    inline FFormatArgumentValue Argument(const FText& Value) { return Value; }
    template<class T> FFormatArgumentValue Argument(T Value) { return FFormatArgumentValue(Value); }
    template<class... T> FText FormatText(const TCHAR* Key,T... Values)
    {
        const FFormatOrderedArguments Arguments{Argument(Values)...};
        return FText::Format(Get(Key),Arguments);
    }
    template<class... T> FString Format(const TCHAR* Key,T... Values)
    {
        return FormatText(Key,Values...).ToString();
    }
}
