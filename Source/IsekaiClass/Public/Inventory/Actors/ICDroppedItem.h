#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "ICDroppedItem.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class AICPlayerController;

UCLASS(Blueprintable)
class ISEKAICLASS_API AICDroppedItem : public AActor
{
    GENERATED_BODY()
public:
    AICDroppedItem();
    UPROPERTY(BlueprintReadOnly) FName DropID;
    UPROPERTY(BlueprintReadOnly) FPlayerItemInstance Item;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    void Initialize(const FPlayerItemInstance& InItem);
    void TryPickup(AICPlayerController* Controller);
};
