#include "PlayerPrototypeController.h"
#include "PlayerCharacter.h"
#include "PlayerInventory.h"

void APlayerPrototypeController::ExecuteUI(const FString& Command,int32 Index)
{
    auto* Inventory=GetInventory();if(!Inventory||bEffectWaiting)return;
    if(Command==TEXT("Create")){CreatePlayer();return;}if(Command==TEXT("Load")){LoadPlayer();return;}
    if(Command==TEXT("Preset")){if(bCreated||bInitialSavePending||Index<0||Index>1)return;Preset=Index;Cast<APlayerPrototypeCharacter>(GetPawn())->ApplyPreset(Preset);bDirty=true;return;}
    if(Command==TEXT("Job")){if(bCreated||bInitialSavePending||Index<0||Index>3)return;Job=Index;bDirty=true;return;}
    if(Command==TEXT("Close")){SetMenu(TEXT(""));return;}if(Command==TEXT("Save")){SavePlayer();return;}
    if(Command==TEXT("Target")){ConfirmTarget(Index==0?FName(TEXT("Player")):FName(*FString::Printf(TEXT("Companion%d"),Index)));return;}
    if(Command==TEXT("Cancel")){CancelTarget();return;}if(Command==TEXT("Menu")){SetMenu(Index==0?TEXT("Inventory"):TEXT("Quest"));return;}
    if(Command==TEXT("Bag")){if(MoveFrom!=INDEX_NONE){Inventory->MoveSlot(MoveFrom,Index);MoveFrom=INDEX_NONE;}SelectedBag=Index;bDirty=true;return;}
    if(Command==TEXT("Chest")){SelectedStorage=Index;bDirty=true;return;}
    if(Command==TEXT("Sort"))Inventory->SortBag();else if(Command==TEXT("Unequip"))Inventory->Unequip(static_cast<EPlayerEquipSlot>(Index));else if(Command==TEXT("Expand"))Inventory->ExpandUsingTicket();else if(Command==TEXT("Withdraw"))Inventory->Transfer(OpenStorage,SelectedStorage,false);
    else if(Inventory->Bag.Slots.IsValidIndex(SelectedBag))
    {
        if(Command==TEXT("Use"))BeginItem(Inventory->Bag.Slots[SelectedBag].ItemID);else if(Command==TEXT("Equip"))Inventory->Equip(SelectedBag);else if(Command==TEXT("Lock"))Inventory->ToggleLock(SelectedBag);else if(Command==TEXT("Discard"))Inventory->Discard(SelectedBag);else if(Command==TEXT("Split"))Inventory->SplitStack(SelectedBag,Inventory->Bag.Slots[SelectedBag].Count/2);else if(Command==TEXT("Move")){MoveFrom=SelectedBag;Show(TEXT("이동할 칸을 선택하세요."));}else if(Command==TEXT("Quick"))Inventory->BindQuickSlot(Index,SelectedBag);else if(Command==TEXT("Deposit"))Inventory->Transfer(OpenStorage,SelectedBag,true);
    }
    bDirty=true;
}
