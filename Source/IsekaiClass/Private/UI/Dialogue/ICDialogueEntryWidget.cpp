#include "UI/Dialogue/ICDialogueEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

FText UICDialogueEntryWidget::GetEntryText() const {return EntryLabel?EntryLabel->GetText():FText::GetEmpty();}
bool UICDialogueEntryWidget::CanSelectEntry() const {return EntryButton&&EntryButton->GetIsEnabled()&&IsVisible();}
bool UICDialogueEntryWidget::HasEntryFocus() const {return EntryButton&&EntryButton->HasKeyboardFocus();}
void UICDialogueEntryWidget::FocusEntry() {if(CanSelectEntry())EntryButton->SetKeyboardFocus();}
void UICDialogueEntryWidget::ActivateEntry() {if(CanSelectEntry())HandleClicked();}
void UICDialogueEntryWidget::SetSelected(bool Selected)
{
    bSelected=Selected;
    if(EntryButton)EntryButton->SetBackgroundColor(Selected?FLinearColor(.36f,.56f,.8f):FLinearColor::White);
    if(EntryLabel)EntryLabel->SetColorAndOpacity(Selected?FLinearColor(1,.83f,.35f):FLinearColor::White);
}

void UICDialogueEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EntryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
    // A stationary mouse must not replace keyboard selection when choices are rebuilt.
}

void UICDialogueEntryWidget::NativeDestruct()
{
    EntryButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClicked);
    Super::NativeDestruct();
}

void UICDialogueEntryWidget::SetEntry(FName InEntryID, const FText& InLabel, bool IsEnabled)
{
    EntryID = InEntryID;
    EntryLabel->SetText(InLabel);
    EntryButton->SetIsEnabled(IsEnabled);
}

void UICDialogueEntryWidget::HandleClicked()
{
    if(CanSelectEntry())OnEntryPressed.Broadcast(EntryID);
}

void UICDialogueEntryWidget::HandleHovered()
{if(CanSelectEntry())OnEntryFocused.Broadcast(EntryID);}

void UICDialogueEntryWidget::NativeOnAddedToFocusPath(const FFocusEvent& FocusEvent)
{
    Super::NativeOnAddedToFocusPath(FocusEvent);
    if(CanSelectEntry())OnEntryFocused.Broadcast(EntryID);
}
