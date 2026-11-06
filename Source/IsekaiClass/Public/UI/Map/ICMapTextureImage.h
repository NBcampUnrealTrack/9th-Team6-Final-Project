#pragma once
#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ICMapTextureImage.generated.h"
class UTexture2D;
class SICMapTextureImage;

/** Image control placed and sized in WBP; optional edge fading supports the minimap. */
UCLASS()
class ISEKAICLASS_API UICMapTextureImage : public UWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map image") TObjectPtr<UTexture2D> Texture;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map image") bool bFadeEdges=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map image") FVector2D UVMin=FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map image") FVector2D UVMax=FVector2D(1.0,1.0);
    UFUNCTION(BlueprintCallable) void SetTexture(UTexture2D* Value);
    UFUNCTION(BlueprintCallable) void SetUVRegion(FVector2D Min,FVector2D Max);
    /** Clip geometry, not the requested UV window, so map edges never stretch under markers. */
    static bool ClipUVRegion(FVector2D Min,FVector2D Max,FVector2D& LocalMin,FVector2D& LocalMax);
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool Children) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<SICMapTextureImage> Image;
};
