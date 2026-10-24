#pragma once
#include "CoreMinimal.h"
#include "InputTriggers.h"
#include "ICInputTriggers.generated.h"

// A modifier gate that leaves ordinary WASD movement active while roll is reserved.
UCLASS(meta=(DisplayName="Control Held"))
class ISEKAICLASS_API UICInputTriggerControl : public UInputTrigger
{
    GENERATED_BODY()
protected:
    virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Implicit; }
    virtual bool IsBlocking(ETriggerState State) const override { return State!=ETriggerState::Triggered; }
    virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput,
        FInputActionValue ModifiedValue, float DeltaTime) override;
};
