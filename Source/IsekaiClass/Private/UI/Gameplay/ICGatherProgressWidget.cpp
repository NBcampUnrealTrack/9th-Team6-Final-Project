#include "UI/Gameplay/ICGatherProgressWidget.h"
#include "UI/Gameplay/ICGameplayLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UICGatherProgressWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (WidgetTree && !WidgetTree->RootWidget) ICGameplayLayout::Build(WidgetTree, TEXT("GatherProgress"));
    SetIsFocusable(false);
}

void UICGatherProgressWidget::SetGatherProgress(const FText& ResourceName, float Value)
{
    Progress = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0.f;
    if (auto* Bar = Cast<UProgressBar>(GetWidgetFromName(TEXT("BAR_GatherProgress")))) Bar->SetPercent(Progress);
    if (auto* Name = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_GatherName"))))
        Name->SetText(FText::Format(NSLOCTEXT("ICLife", "Gathering", "{0} 채집 중"), ResourceName));
}
