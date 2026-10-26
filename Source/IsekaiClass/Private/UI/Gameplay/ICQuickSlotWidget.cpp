#include "UI/Gameplay/ICQuickSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UICQuickSlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshPresentation();
}

void UICQuickSlotWidget::SetItem(FName InItemID, int32 InCount, UTexture2D* InIcon)
{
    const bool bVisible = !InItemID.IsNone() && InCount > 0 && InIcon;
    ItemID = bVisible ? InItemID : NAME_None;
    Count = bVisible ? InCount : 0;
    Icon = bVisible ? InIcon : nullptr;
    RefreshPresentation();
}

void UICQuickSlotWidget::RefreshPresentation()
{
    const bool bVisible = !ItemID.IsNone() && Count > 0 && Icon;
    if (auto* Image = Cast<UImage>(GetWidgetFromName(TEXT("IMG_ItemIcon"))))
        Image->SetBrushFromTexture(Icon);
    if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Count"))))
        Text->SetText(bVisible ? FText::AsNumber(Count) : FText::GetEmpty());
    SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
