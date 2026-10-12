#include "UI/Save/ICSaveLoadWidget.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Save/ICLoadConfirmWidget.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "Core/Controller/ICPlayerController.h"
#include "Save/ICSaveSubsystem.h"
#include "Debug/ICLog.h"
#include "Engine/LocalPlayer.h"
#include "ImageUtils.h"
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
    if(!PC){SetNotice(NSLOCTEXT("ICSaveUI","NoController","ICPlayerController가 필요합니다."));return;}
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
    SlotRows.Reset();ThumbnailTextures.Reset();Super::NativeDestruct();
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
    IC_TRACE_FUNCTION();SetNotice(FText::FromString(Notice));UpdateSelection();
}
void UICSaveLoadWidget::RefreshSlots()
{
    IC_TRACE_FUNCTION();auto* PC=Cast<AICPlayerController>(GetOwningPlayer());
    if(!PC||!IC_SaveLoad_SlotList)return;
    DisplaySlots=PC->SaveSlots;ThumbnailTextures.Reset();
    for(const auto& Info:DisplaySlots)
    {
        UTexture2D* Texture=nullptr;
        if(!Info.Preview.ThumbnailPNG.IsEmpty()&&Info.Preview.ThumbnailPNG.Num()<=2*1024*1024)
            Texture=FImageUtils::ImportBufferAsTexture2D(Info.Preview.ThumbnailPNG);
        ThumbnailTextures.Add(Texture);
    }
    for(const auto& Row:SlotRows)if(Row)Row->OnSlotPicked.RemoveDynamic(this,&ThisClass::HandleSlotPicked);
    IC_SaveLoad_SlotList->ClearChildren();SlotRows.Reset();
    for(int32 Index=0;Index<DisplaySlots.Num();++Index)
    {
        auto* Row=CreateWidget<UICSaveSlotWidget>(GetOwningPlayer(),SlotWidgetClass?SlotWidgetClass.Get():UICSaveSlotWidget::StaticClass());
        if(!Row)continue;
        Row->ConfigureData(DisplaySlots[Index],ThumbnailTextures[Index],PortraitTextures);
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
    if(IC_SaveLoad_Title)IC_SaveLoad_Title->SetText(bSaveMode?(PC->IsQuitAfterSave()?NSLOCTEXT("ICSaveUI","SaveThenQuitTitle","저장 후 종료"):NSLOCTEXT("ICSaveUI","SaveTitle","저장")):NSLOCTEXT("ICSaveUI","LoadTitle","불러오기"));
    if(IC_SaveLoad_Save){IC_SaveLoad_Save->SetVisibility(bSaveMode?ESlateVisibility::Visible:ESlateVisibility::Collapsed);IC_SaveLoad_Save->SetIsEnabled(!bBusy&&PC->bCreated&&SelectedSlot>=1&&SelectedSlot<=4);}
    if(IC_SaveLoad_Load){IC_SaveLoad_Load->SetVisibility(bSaveMode?ESlateVisibility::Collapsed:ESlateVisibility::Visible);IC_SaveLoad_Load->SetIsEnabled(!bBusy&&Selected&&Selected->bValid);}
    if(IC_SaveLoad_Delete)IC_SaveLoad_Delete->SetIsEnabled(!bBusy&&Selected&&Selected->bExists);
    if(IC_SaveLoad_Close)IC_SaveLoad_Close->SetIsEnabled(!bBusy);
    const int32 SelectedIndex=Selected?static_cast<int32>(Selected-DisplaySlots.GetData()):INDEX_NONE;
    ICSavePreviewUI::SetImage(IC_SaveLoad_Thumbnail,ThumbnailTextures.IsValidIndex(SelectedIndex)?ThumbnailTextures[SelectedIndex].Get():nullptr);
    UImage* Images[]={IC_SaveLoad_Party0.Get(),IC_SaveLoad_Party1.Get(),IC_SaveLoad_Party2.Get(),IC_SaveLoad_Party3.Get(),IC_SaveLoad_Party4.Get()};
    for(int32 Index=0;Index<5;++Index)ICSavePreviewUI::SetImage(Images[Index],Selected&&Selected->bValid?ICSavePreviewUI::Portrait(*Selected,Index,PortraitTextures):nullptr);
    if(IC_SaveLoad_Detail)
    {
        FText Detail=NSLOCTEXT("ICSaveUI","PickSlot","슬롯을 선택하세요.");
        if(Selected)
        {
            if(!Selected->bExists)Detail=NSLOCTEXT("ICSaveUI","EmptyDetail","저장 데이터가 없는 슬롯입니다.");
            else if(!Selected->bValid)Detail=NSLOCTEXT("ICSaveUI","InvalidDetail","파일 손상 또는 지원하지 않는 데이터로 불러올 수 없습니다.");
            else Detail=FText::Format(NSLOCTEXT("ICSaveUI","SessionDetail","이름: {0}\n저장 시각: {1}\n게임 시간: {2}\n저장 위치: {3}\n{4}"),FText::FromString(Selected->PlayerName),ICSavePreviewUI::Date(*Selected),ICSavePreviewUI::GameTime(*Selected),ICSavePreviewUI::Location(*Selected),ICSavePreviewUI::Recovery(*Selected));
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
    if(!ConfirmWidgetClass){PC->ConfirmModal(false);PC->Show(TEXT("WBP_IC_SaveLoad의 Confirm Widget Class를 지정하세요."));return;}
    ConfirmScreen=CreateWidget<UICLoadConfirmWidget>(GetOwningPlayer(),ConfirmWidgetClass);
    if(!ConfirmScreen){PC->ConfirmModal(false);return;}
    const int32 PendingSlot=PC->GetPendingSessionSlot();
    const auto* Info=DisplaySlots.FindByPredicate([PendingSlot](const FICSaveSlotInfo& Entry){return Entry.Slot==PendingSlot;});
    const int32 Index=Info?static_cast<int32>(Info-DisplaySlots.GetData()):INDEX_NONE;
    ConfirmScreen->Configure(Info?*Info:FICSaveSlotInfo(),ThumbnailTextures.IsValidIndex(Index)?ThumbnailTextures[Index].Get():nullptr);
    ConfirmScreen->ConfigurePrompt(FText::FromString(PC->ModalText));
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
void UICSaveLoadWidget::HandleSaveStarted(int32 /*SlotIndex*/)
{
    IC_TRACE_FUNCTION();UpdateSelection();
}
void UICSaveLoadWidget::HandleSaveFinished(int32 /*SlotIndex*/,bool /*bSuccess*/,const FString& /*Message*/)
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
