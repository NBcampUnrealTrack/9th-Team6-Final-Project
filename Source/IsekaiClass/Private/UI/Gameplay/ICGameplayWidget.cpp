#include "UI/Gameplay/ICGameplayWidget.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"

AICPlayerController* UICGameplayWidget::GetICPlayerController() const
{
    IC_TRACE_FUNCTION();
    return Cast<AICPlayerController>(GetOwningPlayer());
}

void UICGameplayWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
        ICFrontendLayout::Build(WidgetTree, TEXT("HUD"));
    SetIsFocusable(true);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Resume"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Resume);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Save"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Save);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Load"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Load);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Settings"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Settings);
    if (auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("BTN_Quit"))))
        Button->OnClicked.AddUniqueDynamic(this, &ThisClass::Quit);
}
void UICGameplayWidget::RefreshCurrentScreen()
{
    auto* PC = GetICPlayerController();
    if (!PC)
        return;
    auto SetText = [this](const TCHAR* Name, const FString& Value) {
        if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(Name)))
            Text->SetText(FText::FromString(Value));
    };
    SetText(TEXT("TXT_Player"), PC->PlayerName);
    const auto Snapshot = PC->GetPlayerSnapshot();
    const float Values[] = {Snapshot.HP, Snapshot.MP, Snapshot.SP};
    const float Maximums[] = {Snapshot.MaxHP, Snapshot.MaxMP, Snapshot.MaxSP};
    const TCHAR* Keys[] = {TEXT("HP"), TEXT("MP"), TEXT("SP")};
    for (int32 I = 0; I < 3; ++I)
    {
        SetText(*FString::Printf(TEXT("TXT_%s"), Keys[I]),
                FString::Printf(TEXT("%s  %d / %d"), Keys[I], FMath::FloorToInt(Values[I]),
                                FMath::FloorToInt(Maximums[I])));
        if (auto* Bar = Cast<UProgressBar>(GetWidgetFromName(FName(*FString::Printf(TEXT("BAR_%s"), Keys[I])))))
            Bar->SetPercent(Maximums[I] > 0 ? FMath::Clamp(Values[I] / Maximums[I], 0.f, 1.f) : 0);
    }
    SetText(TEXT("TXT_Hint"), PC->InteractionHint.IsEmpty() ? TEXT("") : TEXT("[F] ") + PC->InteractionHint);
    SetText(TEXT("TXT_Notice"), PC->Notice);
    const bool HasPanel = !PC->Menu.IsEmpty() && PC->Menu != TEXT("Loading") && PC->Menu != TEXT("Create") &&
                          PC->Menu != TEXT("Settings") && PC->Menu != TEXT("Dialogue") && PC->Menu != TEXT("Quest");
    if (auto* Panel = GetWidgetFromName(TEXT("MenuPanel")))
        Panel->SetVisibility(HasPanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (HasPanel)
    {
        const bool Confirm = PC->Menu == TEXT("Confirm");
        const bool Exit = PC->Menu == TEXT("Quit");
        SetText(TEXT("TXT_Menu"), PC->IsDead()                ? TEXT("쓰러졌습니다")
                                  : Confirm                   ? TEXT("확인")
                                  : Exit                      ? TEXT("게임 종료")
                                  : PC->Menu == TEXT("Pause") ? TEXT("일시 정지")
                                                              : PC->Menu);
        FString Body = Confirm ? PC->ModalText : PC->Notice;
        if (PC->Menu == TEXT("Inventory") && PC->GetInventory())
        {
            Body = FString::Printf(TEXT("골드: %d\n\n"), PC->Gold);
            for (const auto& Item : PC->GetInventory()->GetSnapshot().Bag.Slots)
                if (!Item.IsEmpty())
                {
                    FICDefinitionResult Result;
                    const auto* Definition =
                        PC->GetInventory()->GetDefinitions()->Find<FICItemRow>(Item.ItemID, Result);
                    Body += FString::Printf(TEXT("%s  × %d\n"),
                                            Definition ? *Definition->DisplayName.ToString() : *Item.ItemID.ToString(),
                                            Item.Count);
                }
        }
        SetText(TEXT("TXT_MenuBody"), Body);
        auto Label = [this](const TCHAR* Name, const TCHAR* Key) {
            if (auto* Button = Cast<UButton>(GetWidgetFromName(Name)))
                if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
                    Text->SetText(ICFrontendLayout::Text(Key));
        };
        Label(TEXT("BTN_Resume"), Confirm ? TEXT("Accept") : Exit ? TEXT("Back") : TEXT("Resume"));
        Label(TEXT("BTN_Quit"), Confirm ? TEXT("Back") : Exit ? TEXT("QuitWithoutSave") : TEXT("Quit"));
        for (const auto* Name : {TEXT("BTN_Save"), TEXT("BTN_Load"), TEXT("BTN_Settings")})
            if (auto* Button = GetWidgetFromName(Name))
                Button->SetVisibility(Confirm ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
    OnRefreshGameplayUI();
}
FReply UICGameplayWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    auto* PC = GetICPlayerController();
    if (PC && PC->HandleSessionKey(Event.GetKey(), Event.IsRepeat()))
        return FReply::Handled();
    if (PC && !Event.IsRepeat() &&
        (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    {
        PC->HandleInputCommand(EICInputCommand::Menu);
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}
void UICGameplayWidget::Resume()
{
    if (auto* PC = GetICPlayerController())
    {
        if (PC->Menu == TEXT("Confirm"))
            PC->ConfirmModal(true);
        else if (PC->Menu == TEXT("Quit"))
            PC->CloseSessionMenu();
        else
            PC->SetMenu(TEXT(""));
    }
}
void UICGameplayWidget::Save()
{
    if (auto* PC = GetICPlayerController())
        PC->OpenSaveMenu(PC->Menu == TEXT("Quit"));
}
void UICGameplayWidget::Load()
{
    if (auto* PC = GetICPlayerController())
        PC->OpenLoadMenu();
}
void UICGameplayWidget::Settings()
{
    if (auto* PC = GetICPlayerController())
        PC->SetMenu(TEXT("Settings"));
}
void UICGameplayWidget::Quit()
{
    if (auto* PC = GetICPlayerController())
    {
        if (PC->Menu == TEXT("Confirm"))
            PC->ConfirmModal(false);
        else if (PC->Menu == TEXT("Quit"))
            PC->QuitWithoutSave();
        else
            PC->RequestQuit();
    }
}
