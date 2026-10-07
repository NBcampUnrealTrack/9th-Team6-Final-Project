#pragma once
#include "CoreMinimal.h"
#include "World/State/ICWorldStateTypes.h"
class UICDefinitionSubsystem;
namespace ICCombatSimulation
{
    // Executes persistent PlayClock effects without constructing an actor, ASC or GAS handle.
    ISEKAICLASS_API bool Advance(FICCharacterState& Character,const FICWorldState& World,UICDefinitionSubsystem* Definitions,double PlaySecond,double GameMinute);
    ISEKAICLASS_API bool PrepareItemStatus(FICCharacterState& Character,FName EffectID,UICDefinitionSubsystem* Definitions,FString& Reason);
}
