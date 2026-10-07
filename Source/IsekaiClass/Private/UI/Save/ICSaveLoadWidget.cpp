#include "UI/Save/ICSaveLoadWidget.h"
#include "Debug/ICLog.h"
#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Save/ICLoadConfirmWidget.h"
#include "UI/Save/ICSavePreviewHelpers.h"
#include "ImageUtils.h"
#include "Save/ICSaveSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UICSaveSubsystem* UICSaveLoadWidget::GetSaves() const
{
    IC_TRACE_FUNCTION();
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UICSaveSubsystem>() : nullptr;
}
void UICSaveLoadWidget::NativeOnInitialized()
{
    IC_TRACE_FUNCTION();
    Super::NativeOnInitialized();
    if (IC_SaveLoad_Load) IC_SaveLoad_Load->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleLoadClicked);
    if (IC_SaveLoad_Save) IC_SaveLoad_Save->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSaveClicked);
    if (IC_SaveLoad_Close) IC_SaveLoad_Close->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
}
void UICSaveLoadWidget::Configure(bool bInAllowSave, TSoftObjectPtr<UWorld> InGameplayMap)
{
    IC_TRACE_FUNCTION();
    bAllowSave = bInAllowSave; GameplayMap = InGameplayMap;
    UE_LOG(LogICUI, Verbose, TEXT("[SaveUI.Configure] SaveAllowed=%d Map=%s"),bAllowSave,*GameplayMap.ToSoftObjectPath().ToString());
}
void UICSaveLoadWidget::NativeConstruct()
{
    IC_TRACE_FUNCTION();
    Super::NativeConstruct();
    UE_LOG(LogICUI, Log, TEXT("[SaveUI.Opened] Widget=%s SaveAllowed=%d Map=%s"),*GetNameSafe(this),bAllowSave,*GameplayMap.ToSoftObjectPath().ToString());
    if (UICSaveSubsystem* Saves = GetSaves())
        Saves->OnSaveFinished.AddUniqueDynamic(this, &ThisClass::HandleSaveFinished);
    if (IC_SaveLoad_Title)
        IC_SaveLoad_Title->SetText(bAllowSave ? NSLOCTEXT("ICSaveUI", "SaveLoadTitle", "저장 · 불러오기") : NSLOCTEXT("ICSaveUI", "LoadTitle", "불러오기"));
    if (IC_SaveLoad_Save) IC_SaveLoad_Save->SetVisibility(bAllowSave ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bAllowSave)
        if (AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer());PC&&!PC->IsMainMenuLevel()&&PC->Menu!=TEXT("Save")) PC->OpenSaveMenu();
    RefreshSlots();
    if (APlayerController* PC = GetOwningPlayer())
    {
        if(const AICPlayerController* IC = Cast<AICPlayerController>(PC);IC&&!IC->IsMainMenuLevel())
        { FInputModeGameAndUI InputMode;InputMode.SetWidgetToFocus(TakeWidget());InputMode.SetHideCursorDuringCapture(false);PC->SetInputMode(InputMode); }
        else
        { FInputModeUIOnly InputMode;InputMode.SetWidgetToFocus(TakeWidget());PC->SetInputMode(InputMode); }
        PC->bShowMouseCursor = true;
    }
    if (IC_SaveLoad_Close && GetOwningPlayer()) IC_SaveLoad_Close->SetUserFocus(GetOwningPlayer());
}
void UICSaveLoadWidget::NativeDestruct()
{
    IC_TRACE_FUNCTION();
    if (UICSaveSubsystem* Saves = GetSaves()) Saves->OnSaveFinished.RemoveDynamic(this, &ThisClass::HandleSaveFinished);
    if (ConfirmScreen) { ConfirmScreen->RemoveFromParent(); ConfirmScreen = nullptr; }
    Super::NativeDestruct();
    UE_LOG(LogICUI, Verbose, TEXT("[SaveUI.Destruct] Widget=%s: 저장 이벤트 구독 정리"),*GetNameSafe(this));
}
void UICSaveLoadWidget::SetNotice(const FText& Text)
{
    IC_TRACE_FUNCTION();
    if (IC_SaveLoad_Notice) IC_SaveLoad_Notice->SetText(Text);
    if(!Text.IsEmpty())UE_LOG(LogICUI, Verbose, TEXT("[SaveUI.Notice] %s"),*Text.ToString());
}
void UICSaveLoadWidget::RefreshSlots()
{
    IC_TRACE_FUNCTION();
    UICSaveSubsystem* Saves = GetSaves();
    if (!Saves || !IC_SaveLoad_SlotList)
    {
        SetNotice(NSLOCTEXT("ICSaveUI", "Unavailable", "저장 시스템을 사용할 수 없습니다."));
        if (IC_SaveLoad_Load) IC_SaveLoad_Load->SetIsEnabled(false);
        if (IC_SaveLoad_Save) IC_SaveLoad_Save->SetIsEnabled(false);
        return;
    }
    DisplaySlots = Saves->GetSlotList(); ThumbnailTextures.Reset();
    for (int32 Index = 0; Index < DisplaySlots.Num(); ++Index)
    {
        UTexture2D* Texture = nullptr;
        if (!DisplaySlots[Index].Preview.ThumbnailPNG.IsEmpty() && DisplaySlots[Index].Preview.ThumbnailPNG.Num() <= 2 * 1024 * 1024)
            Texture = FImageUtils::ImportBufferAsTexture2D(DisplaySlots[Index].Preview.ThumbnailPNG);
        ThumbnailTextures.Add(Texture);
    }
    const TArray<FICSaveSlotInfo>& Slots = DisplaySlots;
    IC_SaveLoad_SlotList->ClearChildren(); SlotRows.Reset();
    bool bHasSelected = false;
    for (const FICSaveSlotInfo& Info : Slots)
    {
        const FText SlotLabel = Info.Slot == 0 ? NSLOCTEXT("ICSaveUI", "AutoSlot", "자동 저장") : FText::Format(NSLOCTEXT("ICSaveUI", "ManualSlot", "수동 슬롯 {0}"), FText::AsNumber(Info.Slot));
        const FText StateLabel = !Info.bExists ? NSLOCTEXT("ICSaveUI", "Empty", "비어 있음") : !Info.bValid ? NSLOCTEXT("ICSaveUI", "Invalid", "불러올 수 없는 저장") : FText::Format(NSLOCTEXT("ICSaveUI", "SlotNameDate", "{0} · {1}"), FText::FromString(Info.PlayerName), FText::FromString(Info.SavedAt.ToString(TEXT("%Y.%m.%d %H:%M"))));
        UClass* RowClass = SlotWidgetClass ? SlotWidgetClass.Get() : UICSaveSlotWidget::StaticClass();
        UICSaveSlotWidget* Row = CreateWidget<UICSaveSlotWidget>(GetOwningPlayer(), RowClass);
        if (!Row) continue;
        Row->Configure(Info.Slot, FText::Format(NSLOCTEXT("ICSaveUI", "SlotRowV2", "{0}\n{1}"), SlotLabel, StateLabel));
        const int32 InfoIndex = static_cast<int32>(&Info - Slots.GetData());
        Row->ConfigureData(Info, ThumbnailTextures.IsValidIndex(InfoIndex) ? ThumbnailTextures[InfoIndex].Get() : nullptr, PortraitTextures);
        Row->OnSlotPicked.AddUniqueDynamic(this, &ThisClass::HandleSlotPicked);
        if (UVerticalBoxSlot* RowLayoutSlot = IC_SaveLoad_SlotList->AddChildToVerticalBox(Row)) RowLayoutSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
        SlotRows.Add(Row);
        bHasSelected |= Info.Slot == SelectedSlot;
    }
    if (!bHasSelected)
    {
        SelectedSlot = INDEX_NONE;
        for (const FICSaveSlotInfo& Info : Slots) if (Info.bValid) { SelectedSlot = Info.Slot; break; }
        if (SelectedSlot == INDEX_NONE && bAllowSave) SelectedSlot = 1;
    }
    UpdateSelection();
}
void UICSaveLoadWidget::HandleSlotPicked(int32 SlotIndex)
{
    IC_TRACE_FUNCTION();
    if (bWaitingForSave) return;
    SelectedSlot = SlotIndex; PendingOverwriteSlot = INDEX_NONE;
    UE_LOG(LogICUI, Verbose, TEXT("[SaveUI.SlotSelected] Slot=%d"),SlotIndex);
    SetNotice(FText::GetEmpty()); UpdateSelection();
}
void UICSaveLoadWidget::UpdateSelection()
{
    IC_TRACE_FUNCTION();
    const UICSaveSubsystem* Saves = GetSaves();
    const TArray<FICSaveSlotInfo>& Slots = DisplaySlots;
    const FICSaveSlotInfo* Selected = Slots.FindByPredicate([this](const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION(); return Info.Slot == SelectedSlot; });
    for (int32 Index = 0; Index < SlotRows.Num(); ++Index)
    {
        SlotRows[Index]->SetSelected(Slots.IsValidIndex(Index) && Slots[Index].Slot == SelectedSlot);
        SlotRows[Index]->SetIsEnabled(!bWaitingForSave);
    }
    if (IC_SaveLoad_Load) IC_SaveLoad_Load->SetIsEnabled(Selected && Selected->bValid && Saves && !Saves->bSaving && !bWaitingForSave);
    if (IC_SaveLoad_Save) IC_SaveLoad_Save->SetIsEnabled(bAllowSave && Selected && SelectedSlot >= 1 && SelectedSlot <= 4 && Saves && !Saves->bSaving && !bWaitingForSave);
    if (IC_SaveLoad_Close) IC_SaveLoad_Close->SetIsEnabled(!bWaitingForSave);
    const int32 SelectedIndex = Selected ? static_cast<int32>(Selected - Slots.GetData()) : INDEX_NONE;
    ICSavePreviewUI::SetImage(IC_SaveLoad_Thumbnail, ThumbnailTextures.IsValidIndex(SelectedIndex) ? ThumbnailTextures[SelectedIndex].Get() : nullptr);
    UImage* PartyImages[] = { IC_SaveLoad_Party0.Get(), IC_SaveLoad_Party1.Get(), IC_SaveLoad_Party2.Get(), IC_SaveLoad_Party3.Get(), IC_SaveLoad_Party4.Get() };
    for (int32 Index = 0; Index < 5; ++Index) ICSavePreviewUI::SetImage(PartyImages[Index], Selected && Selected->bValid ? ICSavePreviewUI::Portrait(*Selected, Index, PortraitTextures) : nullptr);
    if (IC_SaveLoad_Detail)
    {
        FText Detail = NSLOCTEXT("ICSaveUI", "PickSlot", "슬롯을 선택하세요.");
        if (Selected)
        {
            if (!Selected->bExists) Detail = NSLOCTEXT("ICSaveUI", "EmptyDetail", "저장 데이터가 없는 슬롯입니다.");
            else if (!Selected->bValid) Detail = NSLOCTEXT("ICSaveUI", "InvalidDetail", "파일 손상 또는 지원하지 않는 데이터로 불러올 수 없습니다.");
            else Detail = FText::Format(NSLOCTEXT("ICSaveUI", "DetailWithLocation", "이름: {0}\n저장 시각: {1}\n저장 위치: {2}\n{3}"), FText::FromString(Selected->PlayerName), ICSavePreviewUI::Date(*Selected), ICSavePreviewUI::Location(*Selected), Selected->bUsesBackup ? NSLOCTEXT("ICSaveUI", "Backup", "복구 가능한 백업에서 불러옵니다.") : FText::GetEmpty());
        }
        IC_SaveLoad_Detail->SetText(Detail);
    }
}
void UICSaveLoadWidget::HandleLoadClicked()
{
    IC_TRACE_FUNCTION();
    if (bWaitingForSave || ConfirmScreen) return;
    const FICSaveSlotInfo* Info = DisplaySlots.FindByPredicate([this](const FICSaveSlotInfo& Entry) { IC_TRACE_FUNCTION(); return Entry.Slot == SelectedSlot; });
    if (!Info || !Info->bValid) return;
    if (!ConfirmWidgetClass) { SetNotice(NSLOCTEXT("ICSaveUI", "NoConfirm", "Confirm Widget Class를 지정하세요.")); return; }
    ConfirmScreen = CreateWidget<UICLoadConfirmWidget>(GetOwningPlayer(), ConfirmWidgetClass);
    if (!ConfirmScreen) return;
    PendingLoadSlot = SelectedSlot;
    const int32 Index = static_cast<int32>(Info - DisplaySlots.GetData());
    ConfirmScreen->Configure(*Info, ThumbnailTextures.IsValidIndex(Index) ? ThumbnailTextures[Index].Get() : nullptr);
    ConfirmScreen->OnConfirmed.AddUniqueDynamic(this, &ThisClass::HandleLoadConfirmed);
    ConfirmScreen->OnCancelled.AddUniqueDynamic(this, &ThisClass::HandleLoadCancelled);
    ConfirmScreen->AddToViewport(200); SetIsEnabled(false);
}
void UICSaveLoadWidget::HandleLoadCancelled()
{
    IC_TRACE_FUNCTION();
    if (ConfirmScreen) ConfirmScreen->RemoveFromParent();
    ConfirmScreen = nullptr; PendingLoadSlot = INDEX_NONE; SetIsEnabled(true);
    if (APlayerController* PC = GetOwningPlayer())
    {
        if(const AICPlayerController* IC = Cast<AICPlayerController>(PC);IC&&!IC->IsMainMenuLevel())
        {FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(TakeWidget());Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);}
        else
        {FInputModeUIOnly Mode;Mode.SetWidgetToFocus(TakeWidget());PC->SetInputMode(Mode);}
        if(IC_SaveLoad_Load)IC_SaveLoad_Load->SetUserFocus(PC);
    }
}
void UICSaveLoadWidget::HandleLoadConfirmed()
{
    IC_TRACE_FUNCTION();
    const int32 SlotToLoad = PendingLoadSlot;
    UE_LOG(LogICUI, Log, TEXT("[SaveUI.LoadConfirmed] Slot=%d"),SlotToLoad);
    HandleLoadCancelled();
    SelectedSlot = SlotToLoad; PerformLoad();
}
void UICSaveLoadWidget::PerformLoad()
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICUI, Verbose, TEXT("[SaveUI.Load.Request] Slot=%d"),SelectedSlot);
    UICSaveSubsystem* Saves = GetSaves();
    if (!Saves || Saves->bSaving || bWaitingForSave) return;
    const TArray<FICSaveSlotInfo> Slots = Saves->GetSlotList();
    const FICSaveSlotInfo* Selected = Slots.FindByPredicate([this](const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION(); return Info.Slot == SelectedSlot; });
    if (!Selected || !Selected->bValid) { SetNotice(NSLOCTEXT("ICSaveUI", "SelectValid", "불러올 수 있는 슬롯을 선택하세요.")); return; }
    if (AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer());PC&&!PC->IsMainMenuLevel())
    {
        PC->SelectedLoadSlot = SelectedSlot;
        if (!PC->LoadPlayer()) { SetNotice(FText::FromString(Saves->GetLastMessage())); return; }
        RemoveFromParent(); OnClosed.Broadcast(); return;
    }
    if (GameplayMap.IsNull() || GameplayMap.ToSoftObjectPath().GetLongPackageName().IsEmpty())
    { SetNotice(NSLOCTEXT("ICSaveUI", "NoMap", "메인메뉴 위젯의 Start Map을 지정하세요.")); return; }
    if (!Saves->LoadSlot(SelectedSlot)) { SetNotice(FText::FromString(Saves->GetLastMessage())); return; }
    // The GameInstance owns loaded state; the gameplay controller restores it in BeginPlay.
    Saves->SetAutoSaveSuspended(true);
    if (UICTimeSubsystem* Time = GetGameInstance()->GetSubsystem<UICTimeSubsystem>())
    { Time->PrepareForTravel(); }
    UGameplayStatics::SetGamePaused(this, false);
    if (APlayerController* PC = GetOwningPlayer()) { PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor = false; }
    UE_LOG(LogICUI, Log, TEXT("[SaveUI.Travel] Slot=%d Map=%s"),SelectedSlot,*GameplayMap.ToSoftObjectPath().ToString());
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameplayMap);
}
void UICSaveLoadWidget::HandleSaveClicked()
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICUI, Verbose, TEXT("[SaveUI.Save.Request] Slot=%d"),SelectedSlot);
    UICSaveSubsystem* Saves = GetSaves();
    AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer());
    if (!bAllowSave || !PC || !PC->bCreated || !Saves || Saves->bSaving || bWaitingForSave || SelectedSlot < 1 || SelectedSlot > 4) return;
    PC->SyncSaveSafety();
    const FString Reason = Saves->GetSaveBlockReason(false);
    if (!Reason.IsEmpty()) { SetNotice(FText::FromString(Reason)); return; }
    const TArray<FICSaveSlotInfo> Slots = Saves->GetSlotList();
    const FICSaveSlotInfo* Selected = Slots.FindByPredicate([this](const FICSaveSlotInfo& Info) { IC_TRACE_FUNCTION(); return Info.Slot == SelectedSlot; });
    if (Selected && Selected->bExists && PendingOverwriteSlot != SelectedSlot)
    {
        PendingOverwriteSlot = SelectedSlot;
        SetNotice(NSLOCTEXT("ICSaveUI", "ConfirmOverwrite", "기존 저장을 덮어씁니다. 계속하려면 저장 버튼을 한 번 더 누르세요. 취소하려면 다른 슬롯을 선택하거나 닫으세요.")); return;
    }
    PendingOverwriteSlot = INDEX_NONE; PC->SelectedSaveSlot = SelectedSlot;
    bWaitingForSave = true; UpdateSelection();
    SetNotice(NSLOCTEXT("ICSaveUI", "Saving", "저장 중…"));
    if (!PC->SavePlayer()) { bWaitingForSave = false; SetNotice(FText::FromString(Saves->GetLastMessage())); UpdateSelection(); }
}
void UICSaveLoadWidget::HandleSaveFinished(int32 SlotIndex, bool /*bSuccess*/, const FString& Message)
{
    IC_TRACE_FUNCTION();
    if (bWaitingForSave && SlotIndex != SelectedSlot) return;
    bWaitingForSave = false; RefreshSlots(); SetNotice(FText::FromString(Message));
}
void UICSaveLoadWidget::HandleCloseClicked()
{
    IC_TRACE_FUNCTION();
    if (bWaitingForSave) return;
    if (AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer());PC&&!PC->IsMainMenuLevel()) PC->CloseSessionMenu();
    UE_LOG(LogICUI, Log, TEXT("[SaveUI.Closed] Widget=%s"),*GetNameSafe(this));
    RemoveFromParent(); OnClosed.Broadcast();
}
