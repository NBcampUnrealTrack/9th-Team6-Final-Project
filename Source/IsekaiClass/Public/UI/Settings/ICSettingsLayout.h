#pragma once
#include "CoreMinimal.h"
class UWidgetTree;
enum class EICSettingControl : uint8 { Toggle,Slider,Choice };
struct FICSettingSpec
{
    FName ID;int32 Tab;FText Label,Help;EICSettingControl Control;
    double Min=0,Max=1,Step=.01;
    TArray<FText> Choices;TArray<double> Values;
};
namespace ICSettingsLayout
{
    const TArray<FICSettingSpec>& Specs();
    const TArray<FText>& Tabs();
    void Build(UWidgetTree* Tree);
}
