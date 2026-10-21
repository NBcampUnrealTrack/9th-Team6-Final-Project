#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCraftingComponent.generated.h"

class UICInventoryComponent;

UENUM(BlueprintType)
enum class EPlayerRecipeType : uint8 { Cook , Craft  };

USTRUCT(BlueprintType)
struct FPlayerRecipeDefinition : public FICDefinitionRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RecipeID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) EPlayerRecipeType Type=EPlayerRecipeType::Cook;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float SPCost=0.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bUnlockedByDefault=true;
};

UCLASS(ClassGroup=(Player),meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICCraftingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICCraftingComponent();
    UFUNCTION(BlueprintPure,Category="IC|Crafting") TArray<FName> GetRecipes(EPlayerRecipeType Type) const;
    const struct FICRecipeRow* Find(FName RecipeID) const;
    UFUNCTION(BlueprintPure,Category="IC|Crafting") bool GetRecipeDefinition(FName RecipeID,FPlayerRecipeDefinition& Definition) const;
    UFUNCTION(BlueprintCallable) bool StartCraft(FName RecipeID,FName WorkAnchorID,UICInventoryComponent* Inventory,FGuid& JobID,FString& Reason);
    UFUNCTION(BlueprintPure) bool ValidateDefinitions() const;
};
