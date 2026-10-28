#include "UI/Gameplay/ICQuickSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "Core/Controller/ICPlayerController.h"
#include "Inventory/Components/ICInventoryComponent.h"

void UICQuickSlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshPresentation();
}

void UICQuickSlotWidget::SetItem(FName InItemID, int32 InCount, UTexture2D* InIcon)
{
    const bool bVisible = !InItemID.IsNone() && InCount > 0;
    ItemID = bVisible ? InItemID : NAME_None;
    Count = bVisible ? InCount : 0;
    Icon = bVisible ? InIcon : nullptr;
    RefreshPresentation();
}
void UICQuickSlotWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());const bool Gamepad=PC&&PC->IsUsingGamepad();
    if(Gamepad!=bGamepadHints){bGamepadHints=Gamepad;RefreshPresentation();}
    RefreshCooldown();
}

void UICQuickSlotWidget::RefreshPresentation()
{
    const bool bVisible = !ItemID.IsNone() && Count > 0;
    if (auto* Image = Cast<UImage>(GetWidgetFromName(TEXT("IMG_ItemIcon"))))
    {Image->SetBrushFromTexture(Icon);Image->SetVisibility(Icon?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);}
    if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_Count"))))
        Text->SetText(bVisible ? FText::AsNumber(Count) : FText::GetEmpty());
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_SlotKey"))))
    {
        const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());const TCHAR* Arrows[]={TEXT("↑"),TEXT("→"),TEXT("↓"),TEXT("←")};
        Text->SetText(PC&&PC->IsUsingGamepad()?FText::FromString(Arrows[FMath::Clamp(SlotNumber-1,0,3)]):FText::AsNumber(SlotNumber));
    }
    SetVisibility(ESlateVisibility::HitTestInvisible);
    RefreshCooldown();
}
void UICQuickSlotWidget::RefreshCooldown()
{
    const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    const double Remaining=PC?PC->GetItemCooldown(ItemID):0.;
    const auto* Item=PC&&PC->GetInventory()?PC->GetInventory()->Find(ItemID):nullptr;
    const double Duration=Item?Item->CooldownPlaySeconds:0.;
    const bool Cooling=!ItemID.IsNone()&&Remaining>KINDA_SMALL_NUMBER;
    if(auto* Image=Cast<UImage>(GetWidgetFromName(TEXT("IMG_ItemIcon"))))
        Image->SetColorAndOpacity(Cooling?FLinearColor(.35f,.35f,.35f,1):FLinearColor::White);
    if(auto* Bar=Cast<UProgressBar>(GetWidgetFromName(TEXT("BAR_ItemCooldown"))))
    {
        Bar->SetPercent(Duration>0?FMath::Clamp(float(Remaining/Duration),0.f,1.f):0.f);
        Bar->SetFillColorAndOpacity(FLinearColor(0,0,0,.65f));
        Bar->SetVisibility(Cooling?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    }
    if(auto* Text=Cast<UTextBlock>(GetWidgetFromName(TEXT("TXT_ItemCooldown"))))
    {
        Text->SetText(Cooling?FText::AsNumber(FMath::CeilToInt(Remaining)):FText::GetEmpty());
        Text->SetVisibility(Cooling?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    }
}
