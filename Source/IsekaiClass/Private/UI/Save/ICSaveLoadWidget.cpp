#include "UI/Save/ICSaveLoadWidget.h"
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
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UICSaveSubsystem>() : nullptr;
}
void UICSaveLoadWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (IC_SaveLoad_Load) IC_SaveLoad_Load->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleLoadClicked);
    if (IC_SaveLoad_Save) IC_SaveLoad_Save->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSaveClicked);
    if (IC_SaveLoad_Close) IC_SaveLoad_Close->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
}
void UICSaveLoadWidget::Configure(bool bInAllowSave, TSoftObjectPtr<UWorld> InGameplayMap)
{
    bAllowSave = bInAllowSave; GameplayMap = InGameplayMap;
}
void UICSaveLoadWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (UICSaveSubsystem* Saves = GetSaves())
        Saves->OnSaveFinished.AddUniqueDynamic(this, &ThisClass::HandleSaveFinished);
    if (IC_SaveLoad_Title)
        IC_SaveLoad_Title->SetText(bAllowSave ? NSLOCTEXT("ICSaveUI", "SaveLoadTitle", "저장 · 불러오기") : NSLOCTEXT("ICSaveUI", "LoadTitle", "불러오기"));
    if (IC_SaveLoad_Save) IC_SaveLoad_Save->SetVisibility(bAllowSave ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bAllowSave)
        if (AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer());PC&&PC->Menu!=TEXT("Save")) PC->OpenSaveMenu();
    RefreshSlots();
    if (APlayerController* PC = GetOwningPlayer())
    {
        if(Cast<AICPlayerController>(PC))
        { FInputModeGameAndUI InputMode;InputMode.SetWidgetToFocus(TakeWidget());InputMode.SetHideCursorDuringCapture(false);PC->SetInputMode(InputMode); }
        else
        { FInputModeUIOnly InputMode;InputMode.SetWidgetToFocus(TakeWidget());PC->SetInputMode(InputMode); }
        PC->bShowMouseCursor = true;
    }
    if (IC_SaveLoad_Close && GetOwningPlayer()) IC_SaveLoad_Close->SetUserFocus(GetOwningPlayer());
}
void UICSaveLoadWidget::NativeDestruct()
{
    if (UICSaveSubsystem* Saves = GetSaves()) Saves->OnSaveFinished.RemoveDynamic(this, &ThisClass::HandleSaveFinished);
    if (ConfirmScreen) { ConfirmScreen->RemoveFromParent(); ConfirmScreen = nullptr; }
    Super::NativeDestruct();
}
void UICSaveLoadWidget::SetNotice(const FText& Text)
{
    if (IC_SaveLoad_Notice) IC_SaveLoad_Notice->SetText(Text);
}
void UICSaveLoadWidget::RefreshSlots()
{
    UICSaveSubsystem* Saves = GetSaves();
    if (!Saves || !IC_SaveLoad_SlotList)
    {
        SetNotice(NSLOCTEXT("ICSaveUI", "Unavailable", "저장 시스템을 사용할 수 없습니다."));
        if (IC_SaveLoad_Load) IC_SaveLoad_Load->SetIsEnabled(false);
        if (IC_SaveLoad_Save) IC_SaveLoad_Save->SetIsEnabled(false);
        return;
    }
    DisplaySlots = Saves->GetSlotList(); ThumbnailTextures.Reset();
    if (bUseSampleData)
    {
        DisplaySlots.SetNum(5);
        const TCHAR* Locations[] = { TEXT("해변마을 · 선착장"), TEXT("태초마을 · 광장"), TEXT("봄마을 · 숲 입구") };
        for (int32 Index = 0; Index < 5; ++Index)
        {
            FICSaveSlotInfo Info; Info.Slot = Index;
            if (Index < 3) { Info.bExists = Info.bValid = true; Info.PlayerName = TEXT("강민준"); Info.SavedAt = FDateTime(2026,10,5,20-Index,30); Info.Preview.Location = Locations[Index]; Info.Preview.PartyIds = { TEXT("CHR_PLAYER"), TEXT("DEMO_1"), TEXT("DEMO_2"), TEXT("DEMO_3"), TEXT("DEMO_4") }; }
            DisplaySlots[Index] = Info;
        }
        SetNotice(NSLOCTEXT("ICSaveUI", "Samples", "샘플 표시 모드 — 확인 팝업을 테스트할 수 있습니다. 실제 데이터는 불러오지 않습니다."));
    }
    for (int32 Index = 0; Index < DisplaySlots.Num(); ++Index)
    {
        UTexture2D* Texture = nullptr;
        if (bUseSampleData && SampleThumbnails.IsValidIndex(Index)) Texture = SampleThumbnails[Index];
        else if (!DisplaySlots[Index].Preview.ThumbnailPNG.IsEmpty() && DisplaySlots[Index].Preview.ThumbnailPNG.Num() <= 2 * 1024 * 1024)
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
    if (bWaitingForSave) return;
    SelectedSlot = SlotIndex; PendingOverwriteSlot = INDEX_NONE;
    SetNotice(FText::GetEmpty()); UpdateSelection();
}
void UICSaveLoadWidget::UpdateSelection()
{
    const UICSaveSubsystem* Saves = GetSaves();
    const TArray<FICSaveSlotInfo>& Slots = DisplaySlots;
    const FICSaveSlotInfo* Selected = Slots.FindByPredicate([this](const FICSaveSlotInfo& Info) { return Info.Slot == SelectedSlot; });
    for (int32 Index = 0; Index < SlotRows.Num(); ++Index)
    {
        SlotRows[Index]->SetSelected(Slots.IsValidIndex(Index) && Slots[Index].Slot == SelectedSlot);
        SlotRows[Index]->SetIsEnabled(!bWaitingForSave);
    }
    if (IC_SaveLoad_Load) IC_SaveLoad_Load->SetIsEnabled(Selected && Selected->bValid && Saves && !Saves->bSaving && !bWaitingForSave);
    if (IC_SaveLoad_Save) IC_SaveLoad_Save->SetIsEnabled(bAllowSave && Selected && SelectedSlot >= 1 && SelectedSlot <= 4 && Saves && !Saves->bSaving && !bWaitingForSave && !bUseSampleData);
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
    if (bWaitingForSave || ConfirmScreen) return;
    const FICSaveSlotInfo* Info = DisplaySlots.FindByPredicate([this](const FICSaveSlotInfo& Entry) { return Entry.Slot == SelectedSlot; });
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
    if (ConfirmScreen) ConfirmScreen->RemoveFromParent();
    ConfirmScreen = nullptr; PendingLoadSlot = INDEX_NONE; SetIsEnabled(true);
    if (APlayerController* PC = GetOwningPlayer())
    {
        if(Cast<AICPlayerController>(PC))
        {FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(TakeWidget());Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);}
        else
        {FInputModeUIOnly Mode;Mode.SetWidgetToFocus(TakeWidget());PC->SetInputMode(Mode);}
        if(IC_SaveLoad_Load)IC_SaveLoad_Load->SetUserFocus(PC);
    }
}
void UICSaveLoadWidget::HandleLoadConfirmed()
{
    const int32 SlotToLoad = PendingLoadSlot;
    HandleLoadCancelled();
    if (bUseSampleData)
    {
        SetNotice(NSLOCTEXT("ICSaveUI", "SampleConfirmed", "샘플 확인 완료 — 실제 저장 데이터는 불러오지 않았습니다."));
        return;
    }
    SelectedSlot = SlotToLoad; PerformLoad();
}
void UICSaveLoadWidget::PerformLoad()
{
    if (bUseSampleData) return;
    UICSaveSubsystem* Saves = GetSaves();
    if (!Saves || Saves->bSaving || bWaitingForSave) return;
    const TArray<FICSaveSlotInfo> Slots = Saves->GetSlotList();
    const FICSaveSlotInfo* Selected = Slots.FindByPredicate([this](const FICSaveSlotInfo& Info) { return Info.Slot == SelectedSlot; });
    if (!Selected || !Selected->bValid) { SetNotice(NSLOCTEXT("ICSaveUI", "SelectValid", "불러올 수 있는 슬롯을 선택하세요.")); return; }
    if (AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer()))
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
    { Time->SetRunning(false); Time->SetMenuPaused(true); }
    UGameplayStatics::SetGamePaused(this, false);
    if (APlayerController* PC = GetOwningPlayer()) { PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor = false; }
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameplayMap);
}
void UICSaveLoadWidget::HandleSaveClicked()
{
    UICSaveSubsystem* Saves = GetSaves();
    AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer());
    if (bUseSampleData || !bAllowSave || !PC || !PC->bCreated || !Saves || Saves->bSaving || bWaitingForSave || SelectedSlot < 1 || SelectedSlot > 4) return;
    PC->SyncSaveSafety();
    const FString Reason = Saves->GetSaveBlockReason(false);
    if (!Reason.IsEmpty()) { SetNotice(FText::FromString(Reason)); return; }
    const TArray<FICSaveSlotInfo> Slots = Saves->GetSlotList();
    const FICSaveSlotInfo* Selected = Slots.FindByPredicate([this](const FICSaveSlotInfo& Info) { return Info.Slot == SelectedSlot; });
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
    if (bWaitingForSave && SlotIndex != SelectedSlot) return;
    bWaitingForSave = false; RefreshSlots(); SetNotice(FText::FromString(Message));
}
void UICSaveLoadWidget::HandleCloseClicked()
{
    if (bWaitingForSave) return;
    if (AICPlayerController* PC = Cast<AICPlayerController>(GetOwningPlayer())) PC->CloseSessionMenu();
    RemoveFromParent(); OnClosed.Broadcast();
}
