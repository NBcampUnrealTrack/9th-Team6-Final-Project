#include "Inventory/Components/ICInventoryComponent.h"
#include "Debug/ICLog.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"

bool UICInventoryComponent::ValidateDefinitions() const
{
    IC_TRACE_FUNCTION();
    auto* Service=GetDefinitions();if(!Service){UE_LOG(LogICInventory, Verbose, TEXT("[Inventory.Definitions.Rejected] 정의 조회 서비스 누락"));return false;}
    TArray<FName> IDs;if(!Service->GetDefinitionIDs(EICDefinitionType::Item,IDs).bSuccess || IDs.IsEmpty())return false;
    for(FName ID:IDs)
    {
        const auto* Row=Find(ID);
        if(!Row || Row->ItemID!=ID || Row->StackLimit<1 || Row->StackLimit>99 || Row->BuyPrice<0 || Row->SellPrice<0)return false;
        if(Row->Kind==EPlayerItemKind::Equipment && (Row->StackLimit!=1 || Row->EquipSlot==EPlayerEquipSlot::None))return false;
        if(Row->Kind==EPlayerItemKind::Quest && (Row->bCanDiscard || Row->bCanSell))return false;
    }
    return true;
}
