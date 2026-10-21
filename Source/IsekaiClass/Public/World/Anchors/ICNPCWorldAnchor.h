#pragma once

#include "CoreMinimal.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "ICNPCWorldAnchor.generated.h"

class UArrowComponent;
class UTextRenderComponent;


UCLASS(Blueprintable)
class ISEKAICLASS_API AICNPCWorldAnchor : public AICWorldAnchor
{
    GENERATED_BODY()

public:
    AICNPCWorldAnchor();
    virtual void OnConstruction(const FTransform& Transform) override;


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|NPC Anchor")
    TObjectPtr<UArrowComponent> FacingArrow;


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|NPC Anchor")
    TObjectPtr<UTextRenderComponent> AnchorLabel;

private:
    void RefreshEditorLabel();
};
