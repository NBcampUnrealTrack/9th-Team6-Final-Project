#pragma once
#include "Life/Crafting/ICCraftingComponent.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICRecipeRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICRecipeRow : public FPlayerRecipeDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ProductionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> Ingredients;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICProductionToolRequirement> Tools;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double WorkDurationPlaySeconds = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICProductionResultKind ResultKind = EICProductionResultKind::BagItems;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> Results;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICConditionGroup> UnlockConditions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> InstallationAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<AActor> InstalledActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 StorageCapacityAdded = 40;
};
