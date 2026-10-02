#include "ICCharacterCreateWidget.h"
#include "PlayerPrototypeController.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UICCharacterCreateWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (IC_Create_Name)
        IC_Create_Name->OnTextChanged.AddUniqueDynamic(this, &ThisClass::HandleNameChanged);
    if (IC_Create_Confirm)
        IC_Create_Confirm->OnClicked.AddUniqueDynamic(this, &ThisClass::CreateCharacter);
}
void UICCharacterCreateWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
        if (IC_Create_Name) IC_Create_Name->SetText(FText::FromString(PC->PlayerName));
    RefreshState();
}
void UICCharacterCreateWidget::RefreshState()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
    {
        const bool bCanEdit = PC->CanEditCreation();
        if (IC_Create_Name) IC_Create_Name->SetIsReadOnly(!bCanEdit);
        if (IC_Create_Confirm) IC_Create_Confirm->SetIsEnabled(bCanEdit);
        if (IC_Create_Notice) IC_Create_Notice->SetText(FText::FromString(PC->Notice));
        OnCreationStateChanged(PC->Preset, PC->Job, bCanEdit);
    }
}
void UICCharacterCreateWidget::HandleNameChanged(const FText& Text)
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
        if (PC->CanEditCreation()) PC->PlayerName = Text.ToString();
}
void UICCharacterCreateWidget::SelectPreset(int32 Index)
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
        PC->ExecuteUI(TEXT("Preset"), Index);
}
void UICCharacterCreateWidget::SelectJob(int32 Index)
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer()))
        PC->ExecuteUI(TEXT("Job"), Index);
}
void UICCharacterCreateWidget::CreateCharacter()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer())) PC->RequestCreatePlayer();
}
void UICCharacterCreateWidget::OpenLoadScreen()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer())) PC->OpenLoadMenu();
}
void UICCharacterCreateWidget::OpenQuitScreen()
{
    if (auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer())) PC->RequestQuit();
}
int32 UICCharacterCreateWidget::GetSelectedPreset() const
{
    const auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer());
    return PC ? PC->Preset : 0;
}
int32 UICCharacterCreateWidget::GetSelectedJob() const
{
    const auto* PC = Cast<APlayerPrototypeController>(GetOwningPlayer());
    return PC ? PC->Job : 0;
}
