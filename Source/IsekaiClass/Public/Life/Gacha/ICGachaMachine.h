#pragma once
#include "Interaction/ICInteractionActor.h"
#include "ICGachaMachine.generated.h"

UCLASS(Blueprintable)
class ISEKAICLASS_API AICGachaMachine : public AICInteractionActor
{
    GENERATED_BODY()
public:
    AICGachaMachine();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Gacha") FName GachaMachineID=TEXT("GAC_R01_BASIC_001");
    virtual FText GetInteractionText() const override;
    virtual bool TryInteract_Implementation(AICPlayerController* Controller) override;
private:
    bool bDrawing=false;
};
