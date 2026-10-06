#include "Life/Crafting/ICCraftingComponent.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Internationalization/Regex.h"

UICCraftingComponent::UICCraftingComponent()
{
    auto Add=[this](const TCHAR* ID,const TCHAR* Name,EPlayerRecipeType Type,const TCHAR* Result,int32 ResultCount,const TCHAR* Ingredient,int32 IngredientCount,float SPCost)
    {FPlayerRecipeDefinition D;D.RecipeID=ID;D.DisplayName=FText::FromString(Name);D.Type=Type;D.ResultItemID=Result;D.ResultCount=ResultCount;D.IngredientID1=Ingredient;D.IngredientCount1=IngredientCount;D.SPCost=SPCost;FallbackRecipes.Add(D);};
    Add(TEXT("RCP_COOK_001"),TEXT("빵"),EPlayerRecipeType::Cook,TEXT("ITM_FOOD_002"),1,TEXT("ITM_MAT_002"),2,5.f);
    Add(TEXT("RCP_CRAFT_001"),TEXT("초보자 검"),EPlayerRecipeType::Craft,TEXT("EQP_WPN_SWORD_001"),1,TEXT("ITM_MAT_003"),2,10.f);
    FallbackRecipes.Last().IngredientID2=TEXT("ITM_MAT_001");FallbackRecipes.Last().IngredientCount2=1;

}

const FPlayerRecipeDefinition* UICCraftingComponent::Find(FName RecipeID) const
{
    if(RecipeTable)if(const auto* Row=RecipeTable->FindRow<FPlayerRecipeDefinition>(RecipeID,TEXT("PlayerCrafting"),false))return Row;
    return FallbackRecipes.FindByPredicate([RecipeID](const auto& D){return D.RecipeID==RecipeID;});
}

bool UICCraftingComponent::Craft(FName RecipeID,UICInventoryComponent* Inventory)
{
    const auto* Recipe=Find(RecipeID);if(!Recipe||!Recipe->bUnlockedByDefault||!Inventory)return false;
    TMap<FName,int32> Costs;
    if(!Recipe->IngredientID1.IsNone()&&Recipe->IngredientCount1>0)Costs.Add(Recipe->IngredientID1,Recipe->IngredientCount1);
    if(!Recipe->IngredientID2.IsNone()&&Recipe->IngredientCount2>0)Costs.FindOrAdd(Recipe->IngredientID2)+=Recipe->IngredientCount2;
    return Inventory->ExchangeItems(Costs,Recipe->ResultItemID,Recipe->ResultCount);
}

bool UICCraftingComponent::GetRecipeDefinition(FName RecipeID,FPlayerRecipeDefinition& Definition) const
{
    if(const auto* Found=Find(RecipeID)){Definition=*Found;return true;}
    Definition=FPlayerRecipeDefinition();return false;
}

bool UICCraftingComponent::ValidateDefinitions() const
{
    const FRegexPattern Pattern(TEXT("^RCP_(COOK|CRAFT)_[0-9]{3}$"));TSet<FName> IDs;
    auto Valid=[&](const FPlayerRecipeDefinition& D,FName RowName)
    {FRegexMatcher Match(Pattern,D.RecipeID.ToString());return Match.FindNext()&&(RowName.IsNone()||RowName==D.RecipeID)&&!D.ResultItemID.IsNone()&&D.ResultCount>0&&D.SPCost>=0.f&&!D.IngredientID1.IsNone()&&D.IngredientCount1>0&&!IDs.Contains(D.RecipeID);};
    if(RecipeTable)for(FName Row:RecipeTable->GetRowNames()){const auto* D=RecipeTable->FindRow<FPlayerRecipeDefinition>(Row,TEXT("ValidateRecipes"),false);if(!D||!Valid(*D,Row))return false;IDs.Add(D->RecipeID);}
    else for(const auto& D:FallbackRecipes){if(!Valid(D,NAME_None))return false;IDs.Add(D.RecipeID);}
    return true;
}
