#include "UI/Gameplay/ICCharacterStatusWidget.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
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

void UICCharacterStatusWidget::EnsureSkillCooldownOverlay()
{
    if (!WidgetTree || GetWidgetFromName(TEXT("PortraitCooldownOverlay"))) return;
    auto* Image = Cast<UImage>(GetWidgetFromName(TEXT("IMG_Portrait")));
    auto* PortraitBox = Image ? Cast<USizeBox>(Image->GetParent()) : nullptr;
    if (!PortraitBox) return;

    auto* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PortraitCooldownOverlay"));
    PortraitBox->SetContent(Overlay);
    auto Fill = [](UOverlaySlot* OverlaySlot)
    {
        OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
        OverlaySlot->SetVerticalAlignment(VAlign_Fill);
    };
    Fill(Overlay->AddChildToOverlay(Image));

    auto* Remaining = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_SkillCooldown"));
    Remaining->SetFont(ICFrontendLayout::Font(24, true));
    Remaining->SetJustification(ETextJustify::Center);
    Remaining->SetColorAndOpacity(FLinearColor::White);
    Remaining->SetShadowColorAndOpacity(FLinearColor::Black);
    Remaining->SetShadowOffset(FVector2D(2.f, 2.f));
    Remaining->SetVisibility(ESlateVisibility::Hidden);
    auto* RemainingSlot = Overlay->AddChildToOverlay(Remaining);
    RemainingSlot->SetHorizontalAlignment(HAlign_Fill);
    RemainingSlot->SetVerticalAlignment(VAlign_Center);

    auto* Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_SkillKey"));
    Key->SetText(FText::FromString(TEXT("Q")));
    Key->SetFont(ICFrontendLayout::Font(16, true));
    Key->SetColorAndOpacity(FLinearColor(1.f, .77f, .3f));
    Key->SetShadowColorAndOpacity(FLinearColor::Black);
    Key->SetShadowOffset(FVector2D(1.f, 1.f));
    Key->SetVisibility(ESlateVisibility::Collapsed);
    auto* KeySlot = Overlay->AddChildToOverlay(Key);
    KeySlot->SetHorizontalAlignment(HAlign_Left);
    KeySlot->SetVerticalAlignment(VAlign_Bottom);
    KeySlot->SetPadding(FMargin(3.f));
}

void UICCharacterStatusWidget::UpdateSkillCooldown(bool bHasSkill, float Progress, double RemainingSeconds)
{
    EnsureSkillCooldownOverlay();
    const float SafeProgress = FMath::IsFinite(Progress) ? FMath::Clamp(Progress, 0.f, 1.f) : 1.f;
    const double SafeRemaining = FMath::IsFinite(RemainingSeconds) ? FMath::Max(0., RemainingSeconds) : 0.;
    const bool bCoolingDown = bHasSkill && SafeRemaining > KINDA_SMALL_NUMBER;
    const float Brightness = bCoolingDown ? FMath::Lerp(.3f, 1.f, SafeProgress) : 1.f;

    if (auto* Image = Cast<UImage>(GetWidgetFromName(TEXT("IMG_Portrait"))))
        Image->SetColorAndOpacity(FLinearColor(Brightness, Brightness, Brightness, 1.f));
    if (auto* Remaining = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SkillCooldown"))))
    {
        Remaining->SetText(FText::AsNumber(FMath::CeilToInt(SafeRemaining)));
        Remaining->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    if (auto* Key = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SkillKey"))))
        Key->SetVisibility(bHasSkill ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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
