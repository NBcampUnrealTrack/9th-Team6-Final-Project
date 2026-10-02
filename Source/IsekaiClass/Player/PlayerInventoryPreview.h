#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerInventoryPreview.generated.h"

class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;

UCLASS(Transient, NotBlueprintable)
class ISEKAICLASS_API APlayerInventoryPreview : public AActor
{
    GENERATED_BODY()
public:
    APlayerInventoryPreview();
    void UpdatePreset(int32 Preset);
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> PortraitMesh;
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
    UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
private:
    int32 CurrentPreset=INDEX_NONE;
};
