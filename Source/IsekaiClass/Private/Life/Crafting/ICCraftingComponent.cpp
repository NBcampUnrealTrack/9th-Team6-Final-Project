#include "Life/Crafting/ICCraftingComponent.h"
#include "Debug/ICLog.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Life/Production/ICProductionSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UICCraftingComponent::UICCraftingComponent() { IC_TRACE_FUNCTION(); PrimaryComponentTick.bCanEverTick=false; }
const FICRecipeRow* UICCraftingComponent::Find(FName RecipeID) const
{
    IC_TRACE_FUNCTION();
    auto* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Service=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult Result;
    return Service?Service->Find<FICRecipeRow>(RecipeID,Result):nullptr;
}
TArray<FName> UICCraftingComponent::GetRecipes(EPlayerRecipeType Type) const
{
    IC_TRACE_FUNCTION();
    TArray<FName> IDs;auto* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Service=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    if(!Service || !Service->GetDefinitionIDs(EICDefinitionType::Recipe,IDs).bSuccess)return {};
    IDs.RemoveAll([&](FName ID){ IC_TRACE_FUNCTION();const auto* Row=Find(ID);return !Row||Row->Type!=Type;});return IDs;
}
bool UICCraftingComponent::StartCraft(FName RecipeID,FName WorkAnchorID,UICInventoryComponent* Inventory,FGuid& JobID,FString& Reason)
{
    IC_TRACE_FUNCTION();
    auto* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Service=GI?GI->GetSubsystem<UICProductionSubsystem>():nullptr;
    if(!Service){JobID.Invalidate();Reason=TEXT("생산 시스템이 준비되지 않았습니다.");UE_LOG(LogICProduction, Warning, TEXT("[Craft.Start.Rejected] Recipe=%s Anchor=%s: %s"),*RecipeID.ToString(),*WorkAnchorID.ToString(),*Reason);return false;}
    return Service->StartRecipe(RecipeID,WorkAnchorID,Inventory,JobID,Reason);
}
bool UICCraftingComponent::GetRecipeDefinition(FName RecipeID,FPlayerRecipeDefinition& Definition) const
{
    IC_TRACE_FUNCTION();
    Definition=FPlayerRecipeDefinition();if(const auto* Row=Find(RecipeID)){Definition=*Row;return true;}return false;
}
bool UICCraftingComponent::ValidateDefinitions() const
{
    IC_TRACE_FUNCTION();
    auto* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Service=GI?GI->GetSubsystem<UICDefinitionSubsystem>():nullptr;TArray<FName> IDs;
    if(!Service || !Service->GetDefinitionIDs(EICDefinitionType::Recipe,IDs).bSuccess)return false;
    for(FName ID:IDs){const auto* Row=Find(ID);if(!Row||Row->RecipeID!=ID||Row->Ingredients.IsEmpty()||Row->Results.IsEmpty()||Row->ProductionID.IsNone())return false;}
    return true;
}
