#include "UI/Save/ICSaveLoadWidget.h"
#include "UI/Framework/ICText.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Save/ICLoadConfirmWidget.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "Core/Controller/ICPlayerController.h"
#include "Save/ICSaveSubsystem.h"
#include "Debug/ICLog.h"
#include "Engine/LocalPlayer.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"

UICSaveSubsystem* UICSaveLoadWidget::GetSaves() const
{
    IC_TRACE_FUNCTION();
    const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());return PC?PC->GetSaves():nullptr;
}
void UICSaveLoadWidget::NativeOnInitialized()
{
    IC_TRACE_FUNCTION();Super::NativeOnInitialized();SetIsFocusable(true);
    if(IC_SaveLoad_Load)IC_SaveLoad_Load->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleLoadClicked);
    if(IC_SaveLoad_Save)IC_SaveLoad_Save->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleSaveClicked);
    if(IC_SaveLoad_Delete)IC_SaveLoad_Delete->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleDeleteClicked);
    if(IC_SaveLoad_Close)IC_SaveLoad_Close->OnClicked.AddUniqueDynamic(this,&ThisClass::HandleCloseClicked);
}
void UICSaveLoadWidget::Configure(bool bInAllowSave,TSoftObjectPtr<UWorld> InGameplayMap)
{
    IC_TRACE_FUNCTION();bAllowSave=bInAllowSave;GameplayMap=InGameplayMap;
}
void UICSaveLoadWidget::NativeConstruct()
{
    IC_TRACE_FUNCTION();Super::NativeConstruct();bClosing=false;SetIsEnabled(true);
    auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(!PC){SetNotice(ICText::Get(TEXT("Native_6fd24890b13a")));return;}
    PC->OnMenuChanged.AddUniqueDynamic(this,&ThisClass::HandleMenuChanged);
    PC->OnNoticeChanged.AddUniqueDynamic(this,&ThisClass::HandleNoticeChanged);
    PC->OnSaveSessionChanged.AddUniqueDynamic(this,&ThisClass::RefreshSlots);
    if(auto* Saves=GetSaves())
    {
        Saves->OnSaveStarted.AddUniqueDynamic(this,&ThisClass::HandleSaveStarted);
        Saves->OnSaveFinished.AddUniqueDynamic(this,&ThisClass::HandleSaveFinished);
    }
    PC->SetSessionGameplayMap(GameplayMap);
    const FString Mode=PC->GetSessionMenu();
    if(Mode!=TEXT("Save")&&Mode!=TEXT("Load"))
    {if(bAllowSave)PC->OpenSaveMenu();else PC->OpenLoadMenu();}
    else PC->RefreshSaveSlots();
    HandleMenuChanged(PC->Menu);HandleNoticeChanged(PC->Notice);
    UE_LOG(LogICUI,Log,TEXT("[SaveUI.Opened] Widget=%s SessionMenu=%s"),*GetName(),*PC->GetSessionMenu());
}
void UICSaveLoadWidget::NativeDestruct()
{
    IC_TRACE_FUNCTION();
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))
    {
        PC->OnMenuChanged.RemoveDynamic(this,&ThisClass::HandleMenuChanged);
        PC->OnNoticeChanged.RemoveDynamic(this,&ThisClass::HandleNoticeChanged);
        PC->OnSaveSessionChanged.RemoveDynamic(this,&ThisClass::RefreshSlots);
    }
    if(auto* Saves=GetSaves())
    {
        Saves->OnSaveStarted.RemoveDynamic(this,&ThisClass::HandleSaveStarted);
        Saves->OnSaveFinished.RemoveDynamic(this,&ThisClass::HandleSaveFinished);
    }
    CloseConfirmScreen();
    for(const auto& Row:SlotRows)if(Row)Row->OnSlotPicked.RemoveDynamic(this,&ThisClass::HandleSlotPicked);
    SlotRows.Reset();Super::NativeDestruct();
}
FReply UICSaveLoadWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent)
{
    IC_TRACE_FUNCTION();
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))
        if(PC->HandleSessionKey(KeyEvent.GetKey(),KeyEvent.IsRepeat()))return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(Geometry,KeyEvent);
}
void UICSaveLoadWidget::SetNotice(const FText& Text)
{
    IC_TRACE_FUNCTION();if(IC_SaveLoad_Notice)IC_SaveLoad_Notice->SetText(Text);
}
void UICSaveLoadWidget::HandleNoticeChanged(const FString& Notice)
{
    IC_TRACE_FUNCTION();
    if (const auto* PC=Cast<AICPlayerController>(GetOwningPlayer())) SetNotice(PC->NoticeDisplayText);
    else SetNotice(FText::FromString(Notice));
    UpdateSelection();
}
void UICSaveLoadWidget::RefreshSlots()
{
    IC_TRACE_FUNCTION();auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(!PC||!IC_SaveLoad_SlotList)return;
    DisplaySlots=PC->SaveSlots;
    for(const auto& Row:SlotRows)if(Row)Row->OnSlotPicked.RemoveDynamic(this,&ThisClass::HandleSlotPicked);
    IC_SaveLoad_SlotList->ClearChildren();SlotRows.Reset();
    for(int32 Index=0;Index<DisplaySlots.Num();++Index)
    {
        auto* Row=CreateWidget<UICSaveSlotWidget>(GetOwningPlayer(),SlotWidgetClass?SlotWidgetClass.Get():UICSaveSlotWidget::StaticClass());
        if(!Row)continue;
        Row->ConfigureData(DisplaySlots[Index],PortraitTextures);
        Row->OnSlotPicked.AddUniqueDynamic(this,&ThisClass::HandleSlotPicked);
        if(auto* Layout=IC_SaveLoad_SlotList->AddChildToVerticalBox(Row))Layout->SetPadding(FMargin(0,0,0,12));
        SlotRows.Add(Row);
    }
    UpdateSelection();
}
void UICSaveLoadWidget::HandleSlotPicked(int32 SlotIndex)
{
    IC_TRACE_FUNCTION();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->SelectSessionSlot(SlotIndex);
}
void UICSaveLoadWidget::UpdateSelection()
{
    IC_TRACE_FUNCTION();const auto* PC=Cast<AICPlayerController>(GetOwningPlayer());const auto* Saves=GetSaves();
    if(!PC)return;
    const bool bSaveMode=PC->GetSessionMenu()==TEXT("Save");
    const bool bBusy=!Saves||Saves->bSaving||PC->Menu==TEXT("Confirm");
    const int32 SelectedSlot=PC->GetSelectedSessionSlot();
    const auto* Selected=DisplaySlots.FindByPredicate([SelectedSlot](const FICSaveSlotInfo& Info){return Info.Slot==SelectedSlot;});
    for(const auto& Row:SlotRows)
    {
        Row->SetSelected(Row->GetSlotIndex()==SelectedSlot);
        Row->SetIsEnabled(!bBusy&&(!bSaveMode||Row->GetSlotIndex()!=0));
        if(Row->GetSlotIndex()==SelectedSlot)if(auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(TEXT("IC_SaveLoad_Scroll"))))Scroll->ScrollWidgetIntoView(Row,false);
    }
    if(IC_SaveLoad_Title)IC_SaveLoad_Title->SetText(bSaveMode?(PC->IsQuitAfterSave()?ICText::Get(TEXT("Native_f0a45ce4c600")):ICText::Get(TEXT("Native_1f1712acff27"))):ICText::Get(TEXT("Native_f01a5a522927")));
    if(IC_SaveLoad_Save){IC_SaveLoad_Save->SetVisibility(bSaveMode?ESlateVisibility::Visible:ESlateVisibility::Collapsed);IC_SaveLoad_Save->SetIsEnabled(!bBusy&&PC->bCreated&&SelectedSlot>=1&&SelectedSlot<=4);}
    if(IC_SaveLoad_Load){IC_SaveLoad_Load->SetVisibility(bSaveMode?ESlateVisibility::Collapsed:ESlateVisibility::Visible);IC_SaveLoad_Load->SetIsEnabled(!bBusy&&Selected&&Selected->bValid);}
    if(IC_SaveLoad_Delete)IC_SaveLoad_Delete->SetIsEnabled(!bBusy&&Selected&&Selected->bExists);
    if(IC_SaveLoad_Close)IC_SaveLoad_Close->SetIsEnabled(!bBusy);
    UImage* Images[]={IC_SaveLoad_Party0.Get(),IC_SaveLoad_Party1.Get(),IC_SaveLoad_Party2.Get(),IC_SaveLoad_Party3.Get()};
    for(int32 Index=0;Index<4;++Index)ICSavePreviewUI::SetImage(Images[Index],Selected&&Selected->bValid?ICSavePreviewUI::Portrait(*Selected,Index,PortraitTextures):nullptr);
    if(IC_SaveLoad_Detail)
    {
        FText Detail=ICText::Get(TEXT("Native_461723f0248e"));
        if(Selected)
        {
            if(!Selected->bExists)Detail=ICText::Get(TEXT("Native_0011e9fbfb8b"));
            else if(!Selected->bValid)Detail=ICText::Get(TEXT("Native_b3f14ad18593"));
            else Detail=ICSavePreviewUI::Summary(*Selected);
        }
        IC_SaveLoad_Detail->SetText(Detail);
    }
}
void UICSaveLoadWidget::HandleMenuChanged(const FString& Menu)
{
    IC_TRACE_FUNCTION();auto* PC=Cast<AICPlayerController>(GetOwningPlayer());if(!PC)return;
    const FString Mode=PC->GetSessionMenu();
    if(Mode!=TEXT("Save")&&Mode!=TEXT("Load")){CloseView();return;}
    if(Menu!=TEXT("Confirm")){SetIsEnabled(true);CloseConfirmScreen();UpdateSelection();return;}
    if(ConfirmScreen)return;
    if(!ConfirmWidgetClass){PC->ConfirmModal(false);PC->Show(*ICText::Get(TEXT("Native_b196e7c0611d")).ToString());return;}
    ConfirmScreen=CreateWidget<UICLoadConfirmWidget>(GetOwningPlayer(),ConfirmWidgetClass);
    if(!ConfirmScreen){PC->ConfirmModal(false);return;}
    const int32 PendingSlot=PC->GetPendingSessionSlot();
    const auto* Info=DisplaySlots.FindByPredicate([PendingSlot](const FICSaveSlotInfo& Entry){return Entry.Slot==PendingSlot;});
    ConfirmScreen->Configure(Info?*Info:FICSaveSlotInfo());
    ConfirmScreen->ConfigurePrompt(PC->ModalDisplayText);
    ConfirmScreen->OnConfirmed.AddUniqueDynamic(this,&ThisClass::HandleLoadConfirmed);
    ConfirmScreen->OnCancelled.AddUniqueDynamic(this,&ThisClass::HandleLoadCancelled);
    auto* LP=GetOwningLocalPlayer();auto* UI=LP?LP->GetSubsystem<UICUIManagerSubsystem>():nullptr;
    if(!UI||!UI->OpenScreen(ConfirmScreen,true,200)){CloseConfirmScreen();PC->ConfirmModal(false);return;}
    SetIsEnabled(false);
}
void UICSaveLoadWidget::CloseConfirmScreen()
{
    IC_TRACE_FUNCTION();if(!ConfirmScreen)return;
    auto* Closing=ConfirmScreen.Get();ConfirmScreen=nullptr;
    Closing->OnConfirmed.RemoveDynamic(this,&ThisClass::HandleLoadConfirmed);
    Closing->OnCancelled.RemoveDynamic(this,&ThisClass::HandleLoadCancelled);
    if(auto* LP=GetOwningLocalPlayer())LP->GetSubsystem<UICUIManagerSubsystem>()->CloseScreen(Closing);else Closing->RemoveFromParent();
}
void UICSaveLoadWidget::HandleLoadConfirmed()
{
    IC_TRACE_FUNCTION();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->ConfirmModal(true);
}
void UICSaveLoadWidget::HandleLoadCancelled()
{
    IC_TRACE_FUNCTION();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->ConfirmModal(false);
}
void UICSaveLoadWidget::HandleLoadClicked()
{
    IC_TRACE_FUNCTION();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->RequestLoadPlayer();
}
void UICSaveLoadWidget::HandleSaveClicked()
{
    IC_TRACE_FUNCTION();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->RequestManualSave();
}
void UICSaveLoadWidget::HandleDeleteClicked()
{
    IC_TRACE_FUNCTION();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->RequestDelete();
}
void UICSaveLoadWidget::HandleSaveStarted(int32  )
{
    IC_TRACE_FUNCTION();UpdateSelection();
}
void UICSaveLoadWidget::HandleSaveFinished(int32  ,bool  ,const FString&  )
{
    IC_TRACE_FUNCTION();if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->RefreshSaveSlots();
}
void UICSaveLoadWidget::HandleCloseClicked()
{
    IC_TRACE_FUNCTION();if(auto* Saves=GetSaves();Saves&&Saves->bSaving)return;
    if(auto* PC=Cast<AICPlayerController>(GetOwningPlayer()))PC->CloseSessionMenu();
}
void UICSaveLoadWidget::CloseView()
{
    IC_TRACE_FUNCTION();if(bClosing)return;bClosing=true;
    if(auto* LP=GetOwningLocalPlayer())LP->GetSubsystem<UICUIManagerSubsystem>()->CloseScreen(this);else RemoveFromParent();
    OnClosed.Broadcast();
}
