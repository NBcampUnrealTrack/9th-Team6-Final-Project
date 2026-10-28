#pragma once
#include "CoreMinimal.h"

/** Korean source text stays intact. English translations retain FText culture switching. */
namespace ICLocalization
{
    ISEKAICLASS_API void ApplyLanguage(int32 Language);
    ISEKAICLASS_API FText Localize(const FText& Text);
}
