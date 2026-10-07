#include "UI/Save/ICLoadConfirmWidget.h"
#include "Debug/ICLog.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
void UICLoadConfirmWidget::Configure(const FICSaveSlotInfo& InInfo, UTexture2D* InThumbnail) { IC_TRACE_FUNCTION(); Info = InInfo; Thumbnail = InThumbnail; }
void UICLoadConfirmWidget::NativeOnInitialized()
{
    IC_TRACE_FUNCTION();
    Super::NativeOnInitialized();
    if (IC_LoadConfirm_Confirm) IC_LoadConfirm_Confirm->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
    if (IC_LoadConfirm_Cancel) IC_LoadConfirm_Cancel->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancel);
}
void UICLoadConfirmWidget::NativeConstruct()
{
    IC_TRACE_FUNCTION();
    Super::NativeConstruct();
    ICSavePreviewUI::SetImage(IC_LoadConfirm_Thumbnail, Thumbnail);
    if (IC_LoadConfirm_Summary) IC_LoadConfirm_Summary->SetText(FText::Format(NSLOCTEXT("ICSaveUI", "ConfirmSummary", "{0} · {1}\n{2}\n{3}"), ICSavePreviewUI::SlotName(Info.Slot), FText::FromString(Info.PlayerName), ICSavePreviewUI::Location(Info), ICSavePreviewUI::Date(Info)));
    if (APlayerController* PC = GetOwningPlayer())
    {
        FInputModeUIOnly Mode; Mode.SetWidgetToFocus(TakeWidget()); PC->SetInputMode(Mode);
        if (IC_LoadConfirm_Cancel) IC_LoadConfirm_Cancel->SetUserFocus(PC);
    }
}
void UICLoadConfirmWidget::HandleConfirm() { IC_TRACE_FUNCTION(); UE_LOG(LogICUI, Verbose, TEXT("[LoadConfirm.Accepted] Slot=%d"),Info.Slot);OnConfirmed.Broadcast(); }
void UICLoadConfirmWidget::HandleCancel() { IC_TRACE_FUNCTION(); UE_LOG(LogICUI, Verbose, TEXT("[LoadConfirm.Cancelled] Slot=%d"),Info.Slot);OnCancelled.Broadcast(); }
