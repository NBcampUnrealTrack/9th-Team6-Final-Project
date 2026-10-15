#include "UI/Dialogue/ICDialogueEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UICDialogueEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EntryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void UICDialogueEntryWidget::NativeDestruct()
{
    EntryButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClicked);
    Super::NativeDestruct();
}

void UICDialogueEntryWidget::SetEntry(FName InEntryID, const FText& InLabel)
{
    EntryID = InEntryID;
    EntryLabel->SetText(InLabel);
}

void UICDialogueEntryWidget::HandleClicked()
{
    OnEntryPressed.Broadcast(EntryID);
}
