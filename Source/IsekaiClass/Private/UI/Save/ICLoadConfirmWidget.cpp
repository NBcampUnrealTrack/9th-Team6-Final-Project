#include "UI/Save/ICLoadConfirmWidget.h"
#include "UI/Framework/ICText.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Debug/ICLog.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Core/Controller/ICPlayerController.h"
#include "Blueprint/WidgetTree.h"
void UICLoadConfirmWidget::Configure(const FICSaveSlotInfo& InInfo) { IC_TRACE_FUNCTION(); Info = InInfo; }
void UICLoadConfirmWidget::ConfigurePrompt(const FText& Text) { IC_TRACE_FUNCTION();Prompt=Text; }
void UICLoadConfirmWidget::NativeOnInitialized()
{
    IC_TRACE_FUNCTION();
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    if (IC_LoadConfirm_Confirm) IC_LoadConfirm_Confirm->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
    if (IC_LoadConfirm_Cancel) IC_LoadConfirm_Cancel->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancel);
}
void UICLoadConfirmWidget::NativeConstruct()
{
    IC_TRACE_FUNCTION();
    Super::NativeConstruct();
    if (IC_LoadConfirm_Summary) {IC_LoadConfirm_Summary->SetAutoWrapText(true);IC_LoadConfirm_Summary->SetText(FText::Format(ICText::Get(TEXT("Save_ConfirmSummary")),Prompt,Info.bValid?ICSavePreviewUI::Summary(Info):FText::GetEmpty()));}
    WidgetTree->ForEachWidget([this](UWidget* Widget)
    {
        auto* Label=Cast<UTextBlock>(Widget);if(!Label||Label==IC_LoadConfirm_Summary)return;
        for(auto* Parent=Widget->GetParent();Parent;Parent=Parent->GetParent())if(Parent==IC_LoadConfirm_Confirm)
        {Label->SetText(ICText::Get(TEXT("Native_468266d639b5")));break;}
    });
}
FReply UICLoadConfirmWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent)
{
    IC_TRACE_FUNCTION();
    if((KeyEvent.GetKey()==EKeys::G||KeyEvent.GetKey()==EKeys::F||KeyEvent.GetKey()==EKeys::Enter)&&HasFocusedDescendants())return Super::NativeOnPreviewKeyDown(Geometry,KeyEvent);
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))if(PC->HandleSessionKey(KeyEvent.GetKey(),KeyEvent.IsRepeat()))return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(Geometry,KeyEvent);
}
void UICLoadConfirmWidget::HandleConfirm() { IC_TRACE_FUNCTION(); UE_LOG(LogICUI, Verbose, TEXT("[LoadConfirm.Accepted] Slot=%d"),Info.Slot);OnConfirmed.Broadcast(); }
void UICLoadConfirmWidget::HandleCancel() { IC_TRACE_FUNCTION(); UE_LOG(LogICUI, Verbose, TEXT("[LoadConfirm.Cancelled] Slot=%d"),Info.Slot);OnCancelled.Broadcast(); }
