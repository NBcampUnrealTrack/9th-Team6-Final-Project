#include "Core/Controller/ICPlayerController.h"
#include "Debug/ICLog.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Save/ICSaveSubsystem.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "World/State/ICWorldStateSubsystem.h"

void AICPlayerController::ExecuteUI(const FString& Command,int32 Index)
{
    IC_TRACE_FUNCTION();
    UE_LOG(LogICUI, Verbose, TEXT("[UI.Command] Command=%s Index=%d Menu=%s"),*Command,Index,*Menu);
    auto* Inventory=GetInventory();auto* Saves=GetSaves();if(!Saves)return;
    if(Command==TEXT("ConfirmYes")){ConfirmModal(true);return;}
    if(Command==TEXT("ConfirmNo")){ConfirmModal(false);return;}
    if(Command==TEXT("Quit")){RequestQuit();return;}
    if(Command==TEXT("QuitWithout")){QuitWithoutSave();return;}
    if(Command==TEXT("QuitSave")){OpenSaveMenu(true);return;}
    if(Command==TEXT("Close")){CloseSessionMenu();return;}
    if(Command==TEXT("Delete")){RequestDelete();return;}
    if(Command==TEXT("DeleteLegacy")){RequestDelete(true);return;}
    if(Command==TEXT("Save")){OpenSaveMenu();return;}
    if(Command==TEXT("SaveNow")){RequestManualSave();return;}
    if(Command==TEXT("LoadMenu")){OpenLoadMenu();return;}
    if(bItemTransactionInProgress)return;
    if(Command==TEXT("SaveSlot")||Command==TEXT("LoadSlot")){SelectSessionSlot(Index);return;}
    if(Command==TEXT("Create")){RequestCreatePlayer();return;}
    if(Command==TEXT("Load")){RequestLoadPlayer();return;}
    if(Command==TEXT("Craft"))
    {
        const auto Recipes=GetCrafting()?GetCrafting()->GetRecipes(ActiveStation==TEXT("Kitchen")?EPlayerRecipeType::Cook:EPlayerRecipeType::Craft):TArray<FName>();
        if(Recipes.IsValidIndex(Index))CraftRecipe(Recipes[Index]);return;
    }
    if(Command==TEXT("ImportLegacy"))
    {
        SyncSaveSafety();
        const bool WasPending=bInitialSavePending;bInitialSavePending=false;
        if(Saves->ImportLegacySave()){SelectedLoadSlot=1;RefreshSaveSlots();}else bInitialSavePending=WasPending;
        Show(Saves->GetLastMessage());return;
    }
    if(!Inventory)return;
    if(Command==TEXT("Preset")){if(bCreated||bInitialSavePending||Index<0||Index>1)return;Preset=Index;if(auto* TargetCharacter=Cast<AICPlayerCharacter>(GetPawn()))TargetCharacter->ApplyPreset(Preset);bDirty=true;return;}
    if(Command==TEXT("Job")){if(bCreated||bInitialSavePending||Index<0||Index>3)return;Job=Index;bDirty=true;return;}

    if(Command==TEXT("Target")){if(Index==0)ConfirmTarget(UICWorldStateSubsystem::PlayerId());else ConfirmPartyTarget(Index-1);return;}
    if(Command==TEXT("Cancel")){CancelTarget();return;}
    if(Command==TEXT("Menu")){SetMenu(Index==0?TEXT("Inventory"):TEXT("Quest"));return;}
    const auto Snapshot=Inventory->GetSnapshot();
    if(Command==TEXT("Bag")){if(MoveFrom!=INDEX_NONE){Inventory->MoveSlot(MoveFrom,Index);MoveFrom=INDEX_NONE;}SelectedBag=Index;bDirty=true;return;}
    if(Command==TEXT("Chest")){SelectedStorage=Index;bDirty=true;return;}
    if(Command==TEXT("Sort"))Inventory->SortBag();
    else if(Command==TEXT("Unequip"))Inventory->Unequip(static_cast<EPlayerEquipSlot>(Index));
    else if(Command==TEXT("Expand"))Inventory->ExpandUsingTicket();
    else if(Command==TEXT("Withdraw"))Inventory->Transfer(OpenStorage,SelectedStorage,false);
    else if(Snapshot.Bag.Slots.IsValidIndex(SelectedBag))
    {
        const auto& Item=Snapshot.Bag.Slots[SelectedBag];
        if(Command==TEXT("Use"))BeginItem(Item.ItemID);
        else if(Command==TEXT("Equip"))Inventory->Equip(SelectedBag);
        else if(Command==TEXT("Enhance"))
        {if(Menu!=TEXT("Craft")||ActiveStation!=TEXT("Forge"))Show(TEXT("대장간 강화 메뉴에서만 강화할 수 있습니다."));else{Inventory->EnhanceEquipment(SelectedBag,Gold);FlushWorldState();}}
        else if(Command==TEXT("Lock"))Inventory->ToggleLock(SelectedBag);
        else if(Command==TEXT("Discard"))Inventory->Discard(SelectedBag);
        else if(Command==TEXT("Split"))Inventory->SplitStack(SelectedBag,Item.Count/2);
        else if(Command==TEXT("Move")){MoveFrom=SelectedBag;Show(TEXT("이동할 칸을 선택하세요."));}
        else if(Command==TEXT("Quick"))Inventory->BindQuickSlot(Index,SelectedBag);
        else if(Command==TEXT("Deposit"))Inventory->Transfer(OpenStorage,SelectedBag,true);
    }
    bDirty=true;
}
