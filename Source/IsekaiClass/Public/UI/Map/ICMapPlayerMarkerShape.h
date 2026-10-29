#pragma once
#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ICMapPlayerMarkerShape.generated.h"

class SICMapPlayerMarkerShape;

/** A narrow direction triangle whose geometric centre stays on the player position. */
UCLASS()
class ISEKAICLASS_API UICMapPlayerMarkerShape : public UWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map marker") FLinearColor FillColor=FLinearColor(.4f,1.f,.4f,1.f);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map marker") FLinearColor OutlineColor=FLinearColor(.02f,.08f,.03f,1.f);
    static TArray<FVector2D> TrianglePoints(FVector2D Size);
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool Children) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<SICMapPlayerMarkerShape> Shape;
};
