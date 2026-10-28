#pragma once
#include "Life/Crafting/ICCraftingComponent.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "World/State/Types/ICCharacterStateTypes.h"
#include "ICRecipeRow.generated.h"

/** Item kinds may be mixed to meet one total, e.g. any ten fish. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICRecipeIngredientGroup
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FName> ItemIDs;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1")) int32 Count=1;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICRecipeRow : public FPlayerRecipeDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ProductionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> Ingredients;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICRecipeIngredientGroup> IngredientGroups;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICProductionToolRequirement> Tools;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double WorkDurationPlaySeconds = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICProductionResultKind ResultKind = EICProductionResultKind::BagItems;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICItemAmount> Results;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FICConditionGroup> UnlockConditions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> InstallationAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<AActor> InstalledActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 StorageCapacityAdded = 40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EICMainStat RequiredMainStat=EICMainStat::CON;
    UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(ClampMin="0")) int32 RequiredMainStatValue=0;
    /** Number of additional chests already installed; -1 does not gate this recipe. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RequiredExtraStorage=INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName InstalledContainerID;
};
