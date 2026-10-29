#include "UI/Gameplay/ICInventoryMenuWidget.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"

void UICInventoryMenuWidget::SortItems()
{if(IsBusy())return;if(auto* Inv=Inventory())Inv->SortBag();SelectedItem=0;RefreshInventory();}
void UICInventoryMenuWidget::CycleFilter()
{
    if(IsBusy())return;FilterKind=(FilterKind+1)%7;SelectedItem=0;
    const FText Names[]={NSLOCTEXT("ICInventory","FilterAll","분류: 전체"),NSLOCTEXT("ICInventory","FilterEquipment","분류: 무기"),NSLOCTEXT("ICInventory","FilterConsumable","분류: 소비품"),NSLOCTEXT("ICInventory","FilterFood","분류: 음식"),NSLOCTEXT("ICInventory","FilterBook","분류: 성장서"),NSLOCTEXT("ICInventory","FilterMaterial","분류: 재료"),NSLOCTEXT("ICInventory","FilterLocked","분류: 잠금")};
    if(auto* T=Cast<UTextBlock>(GetWidgetFromName(TEXT("BTN_FilterItems_Text"))))T->SetText(Names[FilterKind]);RefreshItems();
}
void UICInventoryMenuWidget::LockSelected()
{
    auto* Inv=Inventory();const auto Item=CurrentItem();if(!Inv||!bDetailOpen||Item.InstanceID!=DetailInstance||!VisibleBagIndices.IsValidIndex(SelectedItem)||VisibleBagIndices[SelectedItem]<0)return;
    Inv->ToggleLock(VisibleBagIndices[SelectedItem]);RefreshInventory();
}
void UICInventoryMenuWidget::SplitSelected()
{
    auto* Inv=Inventory();const auto Item=CurrentItem();if(!Inv||!bDetailOpen||Item.InstanceID!=DetailInstance||Item.bLocked||Item.Count<2)return;
    auto* Popup=Cast<UICInventoryItemPopupWidget>(GetWidgetFromName(TEXT("ItemPopup")));auto* Quantity=Popup?Cast<USpinBox>(Popup->GetWidgetFromName(TEXT("SplitQuantity"))):nullptr;
    if(!Quantity||!VisibleBagIndices.IsValidIndex(SelectedItem)||VisibleBagIndices[SelectedItem]<0)return;
    if(Inv->SplitStack(VisibleBagIndices[SelectedItem],FMath::Clamp(FMath::RoundToInt(Quantity->GetValue()),1,Item.Count-1)))CloseItem();RefreshInventory();
}
