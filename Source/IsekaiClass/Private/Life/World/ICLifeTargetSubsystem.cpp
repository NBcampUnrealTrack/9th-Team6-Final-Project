#include "Life/World/ICLifeTargetSubsystem.h"
#include "Life/World/ICLifeActor.h"

void UICLifeTargetSubsystem::Register(AICLifeActor* Actor)
{
    if (IsValid(Actor)) Targets.AddUnique(Actor);
}

void UICLifeTargetSubsystem::Unregister(AICLifeActor* Actor)
{
    Targets.RemoveAll([Actor](const TWeakObjectPtr<AICLifeActor>& Entry)
    {
        return !Entry.IsValid() || Entry.Get() == Actor;
    });
}
