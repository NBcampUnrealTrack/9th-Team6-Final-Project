#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerItemTypes.h"
#include "PlayerDroppedItem.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class APlayerPrototypeController;

UCLASS()
class ISEKAICLASS_API APlayerDroppedItem : public AActor
{
    GENERATED_BODY()
public:
    APlayerDroppedItem();
    UPROPERTY(BlueprintReadOnly) FName DropID;
    UPROPERTY(BlueprintReadOnly) FPlayerItemInstance Item;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    void Initialize(const FPlayerItemInstance& InItem);
    void TryPickup(APlayerPrototypeController* Controller);
};
