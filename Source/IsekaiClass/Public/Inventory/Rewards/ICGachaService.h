#pragma once
#include "World/State/ICWorldStateTypes.h"
#include "Data/Rows/Life/ICGachaMachineRow.h"
class UICDefinitionSubsystem;

struct FICGachaDrawResult
{
    FICItemAmount Prize;
    int32 ReceivedCount=0;
    int32 DroppedCount=0;
    FText Failure;
};

class ISEKAICLASS_API FICGachaService
{
public:
    /** Stages payment, prize and overflow together. Caller commits the candidate once. */
    static bool PrepareDraw(FICWorldState& Candidate,const FICGachaMachineRow& Rule,UICDefinitionSubsystem* Definitions,FRandomStream& Random,const FVector& Position,double PlaySecond,FICGachaDrawResult& Result);
};
