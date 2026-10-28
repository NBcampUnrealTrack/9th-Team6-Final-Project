#pragma once
#include "CoreMinimal.h"

/** Two-set Hangul composition, independent of the OS IME and physical keyboard. */
class ISEKAICLASS_API FICNameComposer
{
public:
    FString Text;
    bool Append(TCHAR Character);
    void Backspace();
    void Clear() { Text.Empty(); History.Reset(); }
private:
    TArray<FString> History;
};
