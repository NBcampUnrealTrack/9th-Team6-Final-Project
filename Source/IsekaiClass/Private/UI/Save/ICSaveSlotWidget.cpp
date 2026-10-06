#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ButtonSlot.h"
#include "Engine/Texture2D.h"
TSharedRef<SWidget> UICSaveSlotWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        IC_SaveSlot_Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("IC_SaveSlot_Button"));
        IC_SaveSlot_Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IC_SaveSlot_Label"));
        WidgetTree->RootWidget = IC_SaveSlot_Button;
        UButtonSlot* ButtonContentSlot = Cast<UButtonSlot>(IC_SaveSlot_Button->AddChild(IC_SaveSlot_Label));
        if (ButtonContentSlot) ButtonContentSlot->SetPadding(FMargin(20.f, 16.f));
    }
    return Super::RebuildWidget();
}
void UICSaveSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IC_SaveSlot_Label) IC_SaveSlot_Label->SetText(Label);
    SetSelected(bIsSelected);
    ApplyData();
    if (IC_SaveSlot_Button) IC_SaveSlot_Button->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}
void UICSaveSlotWidget::Configure(int32 InSlot, const FText& InLabel)
{
    SlotIndex = InSlot; Label = InLabel;
    if (IC_SaveSlot_Label) IC_SaveSlot_Label->SetText(Label);
}
void UICSaveSlotWidget::SetSelected(bool bSelected)
{
    bIsSelected = bSelected;
    if (IC_SaveSlot_Button)
    {
        FButtonStyle ButtonStyle = IC_SaveSlot_Button->GetStyle();
        UTexture2D* NormalImage = bSelected ? IC_Load_Slot_Selected.Get() : IC_Load_Slot_Normal.Get();
        UTexture2D* HoverImage = bSelected ? IC_Load_Slot_Selected.Get() : IC_Load_Slot_Hover.Get();
        UTexture2D* PressedImage = bSelected ? IC_Load_Slot_Selected.Get() : IC_Load_Slot_Pressed.Get();
        if (NormalImage) ButtonStyle.Normal.SetResourceObject(NormalImage);
        if (HoverImage) ButtonStyle.Hovered.SetResourceObject(HoverImage);
        if (PressedImage) ButtonStyle.Pressed.SetResourceObject(PressedImage);
        IC_SaveSlot_Button->SetStyle(ButtonStyle);
        IC_SaveSlot_Button->SetBackgroundColor(FLinearColor::White);
    }
    OnSelectionChanged(bSelected);
}
void UICSaveSlotWidget::HandleClicked() { OnSlotPicked.Broadcast(SlotIndex); }

void UICSaveSlotWidget::ConfigureData(const FICSaveSlotInfo& InInfo, UTexture2D* InThumbnail, const TMap<FName, TObjectPtr<UTexture2D>>& Portraits)
{
    Info = InInfo; Thumbnail = InThumbnail; PortraitTextures = Portraits;
    FText Text = FText::Format(NSLOCTEXT("ICSaveUI", "CardEmpty", "{0}\n비어 있음"), ICSavePreviewUI::SlotName(Info.Slot));
    if (Info.bExists && !Info.bValid) Text = FText::Format(NSLOCTEXT("ICSaveUI", "CardInvalid", "{0}\n읽을 수 없는 저장"), ICSavePreviewUI::SlotName(Info.Slot));
    else if (Info.bValid) Text = FText::Format(NSLOCTEXT("ICSaveUI", "CardData", "{0}\n{1} · {2}\n{3}"), ICSavePreviewUI::SlotName(Info.Slot), FText::FromString(Info.PlayerName), ICSavePreviewUI::Date(Info), ICSavePreviewUI::Location(Info));
    Configure(Info.Slot, Text); ApplyData();
}
void UICSaveSlotWidget::ApplyData()
{
    ICSavePreviewUI::SetImage(IC_SaveSlot_Thumbnail, Info.bValid ? Thumbnail.Get() : nullptr);
    UImage* Images[] = { IC_SaveSlot_Party0.Get(), IC_SaveSlot_Party1.Get(), IC_SaveSlot_Party2.Get(), IC_SaveSlot_Party3.Get(), IC_SaveSlot_Party4.Get() };
    for (int32 Index = 0; Index < 5; ++Index) ICSavePreviewUI::SetImage(Images[Index], Info.bValid ? ICSavePreviewUI::Portrait(Info, Index, PortraitTextures) : nullptr);
}
