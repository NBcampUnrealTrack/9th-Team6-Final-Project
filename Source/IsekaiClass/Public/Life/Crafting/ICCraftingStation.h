#pragma once
#include "CoreMinimal.h"
#include "Interaction/ICInteractionActor.h"
#include "Life/Crafting/ICCraftingComponent.h"
#include "ICCraftingStation.generated.h"
class UICCraftingStationWidget;

/** A workbench or kitchen. Geometry and widget class remain editable in BP. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICCraftingStation : public AICInteractionActor
{
    GENERATED_BODY()
public:
    AICCraftingStation();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Crafting") EPlayerRecipeType RecipeType=EPlayerRecipeType::Craft;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Crafting") TSubclassOf<UICCraftingStationWidget> StationWidgetClass;
    virtual bool TryInteract_Implementation(AICPlayerController* Controller) override;
private:
    UPROPERTY(Transient) TObjectPtr<UICCraftingStationWidget> OpenWidget;
};
