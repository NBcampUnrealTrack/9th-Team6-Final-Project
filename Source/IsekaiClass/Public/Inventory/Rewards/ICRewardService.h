#pragma once
#include "CoreMinimal.h"
#include "World/State/ICWorldStateTypes.h"
class UICDefinitionSubsystem;
struct FICMonsterRow;
class ISEKAICLASS_API FICRewardService
{
public:
    // All rolls and allocations occur in the candidate. A failure never changes items or gold.
    static bool PrepareMonsterDefeat(FICWorldState& Candidate,FName SpawnKey,const FICMonsterRow& Monster,UICDefinitionSubsystem* Definitions,double PlaySecond,FString& Reason);
};
