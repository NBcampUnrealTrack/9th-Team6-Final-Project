#include "Inventory/Components/ICInventoryComponent.h"
#include "Internationalization/Regex.h"

bool UICInventoryComponent::ValidateDefinitions() const
{
    const FRegexPattern ItemPattern(TEXT("^ITM_(CONS|FOOD|BOOK|MAT|GATH|FISH|CRAFT|QST|TKT)_[0-9]{3}$"));
    const FRegexPattern EquipmentPattern(TEXT("^EQP_(WPN_(SWORD|BOW|STAFF|WAND)|ARM|RNG|NCK)_[0-9]{3}$"));
    auto IsFormatValid=[&](FName ID)
    {
        const FString Value=ID.ToString();
        if(Value.IsEmpty()||Value.Len()>40)return false;
        FRegexMatcher ItemMatcher(ItemPattern,Value);FRegexMatcher EquipmentMatcher(EquipmentPattern,Value);
        return ItemMatcher.FindNext()||EquipmentMatcher.FindNext();
    };
    auto IsDefinitionValid=[&](const FPlayerItemDefinition& D)
    {
        const FString ID=D.ItemID.ToString();
        static const TSet<FName> LegacyIDs={TEXT("PotionHP"),TEXT("Food"),TEXT("BookSTR"),TEXT("Wood"),TEXT("Ticket"),TEXT("QuestRing"),TEXT("Sword"),TEXT("Bow"),TEXT("Staff"),TEXT("HealBook"),TEXT("Clothes"),TEXT("Ring"),TEXT("Necklace")};
        if(LegacyIDs.Contains(D.ItemID)) return D.StackLimit>=1 && D.StackLimit<=99;
        if(!IsFormatValid(D.ItemID)||D.StackLimit<1||D.StackLimit>99||D.BuyPrice<0||D.SellPrice<0||(D.BuyPrice>0&&D.BuyPrice<D.SellPrice))return false;
        if(static_cast<uint8>(D.DefaultGrade)>static_cast<uint8>(EPlayerItemGrade::Legendary)||D.MaxEnhanceLevel<0||D.MaxRandomOptions<0||D.BaseAttack<0.f||D.BaseDefense<0.f||D.EnhanceAttackPerLevel<0.f||D.EnhanceDefensePerLevel<0.f||D.EnhanceBaseCost<0||D.EnhanceBaseSuccessRate<0.f||D.EnhanceBaseSuccessRate>1.f)return false;
        const bool IsEquipment=ID.StartsWith(TEXT("EQP_"));const bool IsQuest=ID.StartsWith(TEXT("ITM_QST_"));
        if((D.Tab==EPlayerItemTab::Quest)!=IsQuest||(IsQuest&&(D.bCanDiscard||D.bCanSell)))return false;
        if(IsEquipment!=(D.Kind==EPlayerItemKind::Equipment)||IsEquipment!=(D.Category==EPlayerItemCategory::Equipment))return false;
        if(IsEquipment&&(D.StackLimit!=1||D.EquipSlot==EPlayerEquipSlot::None))return false;
        if(IsEquipment&&(D.MaxRandomOptions<=0||D.OptionPoolID.IsNone()||D.MaxEnhanceLevel<=0||D.EnhanceBaseCost<=0))return false;
        if(!IsEquipment&&D.EquipSlot!=EPlayerEquipSlot::None)return false;
        if(D.EquipSlot==EPlayerEquipSlot::Weapon&&(D.RequiredClass<0||D.RequiredClass>3||D.AttachSocket.IsNone()))return false;
        if(D.EquipSlot!=EPlayerEquipSlot::Weapon&&D.RequiredClass!=INDEX_NONE)return false;
        if(ID.StartsWith(TEXT("ITM_CONS_"))&&(D.Category!=EPlayerItemCategory::Consumable&&D.Category!=EPlayerItemCategory::Revival))return false;
        if(ID.StartsWith(TEXT("ITM_FOOD_"))&&D.Category!=EPlayerItemCategory::Food)return false;
        if(ID.StartsWith(TEXT("ITM_BOOK_"))&&(D.Category!=EPlayerItemCategory::Book||D.EffectID!=TEXT("GrowMainStat")||D.GrowthStat==EPlayerMainStat::None||D.GrowthAmount<=0))return false;
        if((ID.StartsWith(TEXT("ITM_CONS_"))||ID.StartsWith(TEXT("ITM_FOOD_"))||ID.StartsWith(TEXT("ITM_BOOK_")))&&D.Kind!=EPlayerItemKind::Consumable)return false;
        if(ID.StartsWith(TEXT("ITM_MAT_"))&&D.Category!=EPlayerItemCategory::Material)return false;
        if(ID.StartsWith(TEXT("ITM_GATH_"))&&D.Category!=EPlayerItemCategory::Gathering)return false;
        if(ID.StartsWith(TEXT("ITM_FISH_"))&&D.Category!=EPlayerItemCategory::Fish)return false;
        if(ID.StartsWith(TEXT("ITM_CRAFT_"))&&D.Category!=EPlayerItemCategory::Crafted)return false;
        if((ID.StartsWith(TEXT("ITM_MAT_"))||ID.StartsWith(TEXT("ITM_GATH_"))||ID.StartsWith(TEXT("ITM_FISH_"))||ID.StartsWith(TEXT("ITM_CRAFT_")))&&D.Kind!=EPlayerItemKind::Material)return false;
        if(IsQuest&&(D.Category!=EPlayerItemCategory::Quest||D.Kind!=EPlayerItemKind::Quest))return false;
        if(ID.StartsWith(TEXT("ITM_TKT_"))&&(D.Category!=EPlayerItemCategory::Ticket||D.Kind!=EPlayerItemKind::Expansion))return false;
        return true;
    };
    TSet<FName> IDs;
    for(const auto& Definition:Definitions)
    {
        if(!IsDefinitionValid(Definition)||IDs.Contains(Definition.ItemID))return false;
        IDs.Add(Definition.ItemID);
    }
    if(ItemTable)
    {
        TSet<FName> RowIDs;
        for(FName RowName:ItemTable->GetRowNames())
        {
            const auto* Row=ItemTable->FindRow<FPlayerItemDefinition>(RowName,TEXT("ValidateItemDefinitions"),false);
            if(!Row||Row->ItemID!=RowName||!IsDefinitionValid(*Row)||RowIDs.Contains(RowName))return false;
            RowIDs.Add(RowName);
        }
    }
    return true;
}
