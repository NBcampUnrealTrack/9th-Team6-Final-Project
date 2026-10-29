#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Framework/ICText.h"
#include "UI/Framework/ICUITheme.h"
#include "Debug/ICLog.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ButtonSlot.h"
#include "Engine/Texture2D.h"
void UICSaveSlotWidget::NativeConstruct()
{
    IC_TRACE_FUNCTION();
    Super::NativeConstruct();
    if (IC_SaveSlot_Label)
    {IC_SaveSlot_Label->SetText(Label);IC_SaveSlot_Label->SetAutoWrapText(false);IC_SaveSlot_Label->SetWrapTextAt(0);IC_SaveSlot_Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);}
    SetSelected(bIsSelected);
    ApplyData();
    if (IC_SaveSlot_Button) IC_SaveSlot_Button->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}
void UICSaveSlotWidget::Configure(int32 InSlot, const FText& InLabel)
{
    IC_TRACE_FUNCTION();
    SlotIndex = InSlot; Label = InLabel;
    if (IC_SaveSlot_Label)
    {IC_SaveSlot_Label->SetText(Label);IC_SaveSlot_Label->SetAutoWrapText(false);IC_SaveSlot_Label->SetWrapTextAt(0);IC_SaveSlot_Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);}
}
void UICSaveSlotWidget::SetSelected(bool bSelected)
{
    IC_TRACE_FUNCTION();
    bIsSelected = bSelected;
    ICUITheme::SetSelected(IC_SaveSlot_Button,bSelected);
    if(IC_SaveSlot_Button)
    {auto Style=IC_SaveSlot_Button->GetStyle();Style.Hovered=Style.Normal;Style.Pressed=Style.Normal;IC_SaveSlot_Button->SetStyle(Style);}
    OnSelectionChanged(bSelected);
}
void UICSaveSlotWidget::HandleClicked() { IC_TRACE_FUNCTION(); UE_LOG(LogICUI, Verbose, TEXT("[SaveSlot.Picked] Slot=%d Exists=%d Valid=%d Backup=%d"),SlotIndex,Info.bExists,Info.bValid,Info.bUsesBackup);OnSlotPicked.Broadcast(SlotIndex); }

void UICSaveSlotWidget::ConfigureData(const FICSaveSlotInfo& InInfo, const TMap<FName, TObjectPtr<UTexture2D>>& Portraits)
{
    IC_TRACE_FUNCTION();
    Info = InInfo; PortraitTextures = Portraits;
    FText Text = FText::Format(ICText::Get(TEXT("Native_03ef37db1622")), ICSavePreviewUI::SlotName(Info.Slot));
    if (Info.bExists && !Info.bValid) Text = FText::Format(ICText::Get(TEXT("Native_a5426b8e5330")), ICSavePreviewUI::SlotName(Info.Slot));
    else if (Info.bValid)
    {
        Text=FText::Format(NSLOCTEXT("ICSaveUI","CompactRow","{0}  ·  {1}  ·  {2}  ·  플레이 {3}  ·  저장 {4}"),
            ICSavePreviewUI::SlotName(Info.Slot),FText::FromString(Info.PlayerName),ICSavePreviewUI::Location(Info),ICSavePreviewUI::PlayTime(Info),ICSavePreviewUI::Date(Info));
        if(Info.bUsesBackup)Text=FText::Format(NSLOCTEXT("ICSaveUI","RecoveredRow","{0}  ·  {1}"),Text,ICSavePreviewUI::Recovery(Info));
    }
    FString SingleLine=Text.ToString();SingleLine.ReplaceInline(TEXT("\r"),TEXT(""));SingleLine.ReplaceInline(TEXT("\n"),TEXT("  ·  "));
    Text=FText::FromString(SingleLine);
    Configure(Info.Slot, Text); ApplyData();
}
void UICSaveSlotWidget::ApplyData()
{
    IC_TRACE_FUNCTION();
    UImage* Images[] = { IC_SaveSlot_Party0.Get(), IC_SaveSlot_Party1.Get(), IC_SaveSlot_Party2.Get(), IC_SaveSlot_Party3.Get() };
    for (int32 Index = 0; Index < 4; ++Index) ICSavePreviewUI::SetImage(Images[Index], Info.bValid ? ICSavePreviewUI::Portrait(Info, Index, PortraitTextures) : nullptr);
}
