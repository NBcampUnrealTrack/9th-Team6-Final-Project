#pragma once
#include "CoreMinimal.h"
#include "World/State/ICWorldStateTypes.h"
class UICDefinitionSubsystem;
namespace ICCombatSimulation
{

    ISEKAICLASS_API bool Advance(FICCharacterState& Character,const FICWorldState& World,UICDefinitionSubsystem* Definitions,double PlaySecond,double GameMinute);
    ISEKAICLASS_API bool PrepareItemStatus(FICCharacterState& Character,FName EffectID,UICDefinitionSubsystem* Definitions,FString& Reason);
    ISEKAICLASS_API bool RecoverFromSleep(FICCharacterState& Character,const FICWorldState& World,UICDefinitionSubsystem* Definitions);
}
