#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Framework/ICText.h"
#include "Debug/ICLog.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ButtonSlot.h"
#include "Engine/Texture2D.h"
TSharedRef<SWidget> UICSaveSlotWidget::RebuildWidget()
{
    IC_TRACE_FUNCTION();
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
    IC_TRACE_FUNCTION();
    Super::NativeConstruct();
    if (IC_SaveSlot_Label) IC_SaveSlot_Label->SetText(Label);
    SetSelected(bIsSelected);
    ApplyData();
    if (IC_SaveSlot_Button) IC_SaveSlot_Button->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}
void UICSaveSlotWidget::Configure(int32 InSlot, const FText& InLabel)
{
    IC_TRACE_FUNCTION();
    SlotIndex = InSlot; Label = InLabel;
    if (IC_SaveSlot_Label) IC_SaveSlot_Label->SetText(Label);
}
void UICSaveSlotWidget::SetSelected(bool bSelected)
{
    IC_TRACE_FUNCTION();
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
        if(!NormalImage)ButtonStyle.Normal.TintColor=bSelected?FLinearColor(.18f,.27f,.40f):FLinearColor(.075f,.10f,.15f);
        IC_SaveSlot_Button->SetStyle(ButtonStyle);
        IC_SaveSlot_Button->SetBackgroundColor(FLinearColor::White);
    }
    OnSelectionChanged(bSelected);
}
void UICSaveSlotWidget::HandleClicked() { IC_TRACE_FUNCTION(); UE_LOG(LogICUI, Verbose, TEXT("[SaveSlot.Picked] Slot=%d Exists=%d Valid=%d Backup=%d"),SlotIndex,Info.bExists,Info.bValid,Info.bUsesBackup);OnSlotPicked.Broadcast(SlotIndex); }

void UICSaveSlotWidget::ConfigureData(const FICSaveSlotInfo& InInfo, const TMap<FName, TObjectPtr<UTexture2D>>& Portraits)
{
    IC_TRACE_FUNCTION();
    Info = InInfo; PortraitTextures = Portraits;
    FText Text = FText::Format(ICText::Get(TEXT("Native_03ef37db1622")), ICSavePreviewUI::SlotName(Info.Slot));
    if (Info.bExists && !Info.bValid) Text = FText::Format(ICText::Get(TEXT("Native_a5426b8e5330")), ICSavePreviewUI::SlotName(Info.Slot));
    else if (Info.bValid) Text = ICSavePreviewUI::Summary(Info);
    Configure(Info.Slot, Text); ApplyData();
}
void UICSaveSlotWidget::ApplyData()
{
    IC_TRACE_FUNCTION();
    UImage* Images[] = { IC_SaveSlot_Party0.Get(), IC_SaveSlot_Party1.Get(), IC_SaveSlot_Party2.Get(), IC_SaveSlot_Party3.Get() };
    for (int32 Index = 0; Index < 4; ++Index) ICSavePreviewUI::SetImage(Images[Index], Info.bValid ? ICSavePreviewUI::Portrait(Info, Index, PortraitTextures) : nullptr);
}
