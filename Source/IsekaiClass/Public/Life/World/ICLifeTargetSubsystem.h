#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ICLifeTargetSubsystem.generated.h"

class AICLifeActor;

/** Loaded life locations, maintained by actor lifetime rather than repeated world scans. */
UCLASS()
class ISEKAICLASS_API UICLifeTargetSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void Register(AICLifeActor* Actor);
    void Unregister(AICLifeActor* Actor);
    const TArray<TWeakObjectPtr<AICLifeActor>>& GetTargets() const { return Targets; }
private:
    TArray<TWeakObjectPtr<AICLifeActor>> Targets;
};
