#pragma once
#include "CoreMinimal.h"
#include "World/State/ICWorldStateTypes.h"
class UICDefinitionSubsystem;
struct FICMonsterRow;
class ISEKAICLASS_API FICRewardService
{
public:

    static bool PrepareMonsterDefeat(FICWorldState& Candidate,FName SpawnKey,const FICMonsterRow& Monster,UICDefinitionSubsystem* Definitions,double PlaySecond,FString& Reason);
};
