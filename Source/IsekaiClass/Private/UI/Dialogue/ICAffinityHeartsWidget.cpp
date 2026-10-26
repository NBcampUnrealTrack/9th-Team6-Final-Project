#include "UI/Dialogue/ICAffinityHeartsWidget.h"
#include "Components/ProgressBar.h"

void UICAffinityHeartsWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshHearts();
}

void UICAffinityHeartsWidget::SetAffinity(int32 Value)
{
    Affinity = FMath::Clamp(Value, 0, 500);
    RefreshHearts();
}

void UICAffinityHeartsWidget::RefreshHearts()
{
    for (int32 Index = 0; Index < 5; ++Index)
        if (auto* Heart = Cast<UProgressBar>(GetWidgetFromName(FName(*FString::Printf(TEXT("AffinityHeart%d"), Index + 1)))))
            Heart->SetPercent(FMath::Clamp(Affinity / 100.f - Index, 0.f, 1.f));
}
