#pragma once

#include "CoreMinimal.h"
#include "AI/ICAIController.h"
#include "ICNPCAIController.generated.h"

/** NPC-only AI controller extension. Keeps NPC life/interactions out of the shared AI controller. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICNPCAIController : public AICAIController
{
    GENERATED_BODY()

public:
    virtual void StopExecution() override;
};
