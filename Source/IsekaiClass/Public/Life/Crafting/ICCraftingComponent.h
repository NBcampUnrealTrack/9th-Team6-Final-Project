#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "ICCraftingComponent.generated.h"

class UICInventoryComponent;

UENUM(BlueprintType)
enum class EPlayerRecipeType : uint8 { Cook, Craft };

USTRUCT(BlueprintType)
struct FPlayerRecipeDefinition : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RecipeID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerRecipeType Type=EPlayerRecipeType::Cook;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ResultItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 ResultCount=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName IngredientID1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 IngredientCount1=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName IngredientID2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 IngredientCount2=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float SPCost=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bUnlockedByDefault=true;
};

UCLASS(ClassGroup=(Player),meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICCraftingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICCraftingComponent();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UDataTable> RecipeTable;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FPlayerRecipeDefinition> FallbackRecipes;
    const FPlayerRecipeDefinition* Find(FName RecipeID) const;
    UFUNCTION(BlueprintPure,Category="IC|Crafting") bool GetRecipeDefinition(FName RecipeID,FPlayerRecipeDefinition& Definition) const;
    UFUNCTION(BlueprintCallable) bool Craft(FName RecipeID,UICInventoryComponent* Inventory);
    UFUNCTION(BlueprintPure) bool ValidateDefinitions() const;
};
