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
    UFUNCTION(BlueprintCallable) void SetTexture(UTexture2D* Value);
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool Children) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<SICMapTextureImage> Image;
};
