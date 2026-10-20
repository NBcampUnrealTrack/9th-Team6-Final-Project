#pragma once
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
class UWidgetTree;
namespace ICFrontendLayout
{
ISEKAICLASS_API void Build(UWidgetTree* Tree, FName Kind);
ISEKAICLASS_API FText Text(const TCHAR* Key);
ISEKAICLASS_API FSlateFontInfo Font(int32 Size, bool Bold = false);
}
