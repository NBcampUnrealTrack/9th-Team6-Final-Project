#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerInteractionObject.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
UCLASS()
class ISEKAICLASS_API APlayerPrototypeObject : public AActor
{
    GENERATED_BODY()
public:
    APlayerPrototypeObject();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shape;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Action;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ContainerID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bHome=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bAccessGranted=true;
};
