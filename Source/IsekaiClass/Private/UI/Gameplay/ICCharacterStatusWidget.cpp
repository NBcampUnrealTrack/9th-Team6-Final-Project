#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "World/State/Types/ICCharacterStateTypes.h"

void UICCharacterStatusWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshPresentation();
}

void UICCharacterStatusWidget::RefreshPresentation()
{
    if (auto* Name = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Name"))))
        Name->SetText(DisplayName);
    if (auto* Image = Cast<UImage>(GetWidgetFromName(TEXT("IMG_Portrait"))))
    {
        Image->SetBrushFromTexture(Portrait);
        Image->SetVisibility(Portrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    const TCHAR* Names[] = {TEXT("BAR_HP"), TEXT("BAR_MP"), TEXT("BAR_SP")};
    const float Values[] = {HPPercent, MPPercent, SPPercent};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
        if (auto* Bar = Cast<UProgressBar>(GetWidgetFromName(Names[Index])))
            Bar->SetPercent(FMath::IsFinite(Values[Index]) ? FMath::Clamp(Values[Index], 0.f, 1.f) : 0.f);
}

void UICCharacterStatusWidget::UpdateVitals(const FICCharacterState& State)
{
    const float Current[] = {State.HP, State.MP, State.SP};
    const float Maximum[] = {State.MaxHP, State.MaxMP, State.MaxSP};
    float* Percent[] = {&HPPercent, &MPPercent, &SPPercent};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Percent); ++Index)
    {
        const float Max = FMath::IsFinite(Maximum[Index]) ? FMath::Max(0.f, Maximum[Index]) : 0.f;
        const float Value = FMath::IsFinite(Current[Index]) ? FMath::Clamp(Current[Index], 0.f, Max) : 0.f;
        *Percent[Index] = Max > 0 ? Value / Max : 0.f;
    }
    RefreshPresentation();
}
