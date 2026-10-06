#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICInteractionActor.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
class AICPlayerController;
UCLASS(Blueprintable)
class ISEKAICLASS_API AICInteractionActor : public AActor
{
    GENERATED_BODY()
public:
    AICInteractionActor();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Shape;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction") FText InteractionText;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Action;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ContainerID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bHome=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bAccessGranted=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction") FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction",meta=(ClampMin="1")) int32 ItemCount=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Interaction") bool bDestroyAfterUse=false;
    UFUNCTION(BlueprintPure,Category="IC|Interaction") FText GetInteractionText() const;
    UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="IC|Interaction") bool TryInteract(AICPlayerController* Controller);
};
