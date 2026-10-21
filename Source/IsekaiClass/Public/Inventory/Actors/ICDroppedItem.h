#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "World/State/Types/ICWorldProgressStateTypes.h"
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
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly) FGuid RecordID;
    UPROPERTY(BlueprintReadOnly) FName DropID;
    UPROPERTY(BlueprintReadOnly) FPlayerItemInstance Item;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    void Initialize(const FICDropState& Record);
    void TryPickup(AICPlayerController* Controller);
};
