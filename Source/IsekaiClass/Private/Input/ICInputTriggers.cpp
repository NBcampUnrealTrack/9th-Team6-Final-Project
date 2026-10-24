#include "Input/ICInputTriggers.h"
#include "EnhancedPlayerInput.h"
#include "InputCoreTypes.h"

ETriggerState UICInputTriggerControl::UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput,
    FInputActionValue ModifiedValue,float DeltaTime)
{
    return PlayerInput&&ModifiedValue.IsNonZero()&&
        (PlayerInput->IsPressed(EKeys::LeftControl)||PlayerInput->IsPressed(EKeys::RightControl))
        ?ETriggerState::Triggered:ETriggerState::None;
}
