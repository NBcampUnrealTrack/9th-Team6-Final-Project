#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICInventoryPreview.generated.h"

class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;
class AICPlayerCharacter;

UCLASS(Transient, Blueprintable)
class ISEKAICLASS_API AICInventoryPreview : public AActor
{
    GENERATED_BODY()
public:
    AICInventoryPreview();
    UFUNCTION(BlueprintCallable,Category="IC|UI") void UpdateFromCharacter(AICPlayerCharacter* Character);
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> PortraitMesh;
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(BlueprintReadOnly,Category="IC|UI") TObjectPtr<UTextureRenderTarget2D> Target;
};
