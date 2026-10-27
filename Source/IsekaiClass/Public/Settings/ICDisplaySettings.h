#pragma once
#include "CoreMinimal.h"

struct FICDisplayMode
{
    int32 Width=0,Height=0,Hz=0;
    bool operator==(const FICDisplayMode& Other) const {return Width==Other.Width&&Height==Other.Height&&Hz==Other.Hz;}
};
struct FICMonitor
{
    FString Device,Label;
    FIntPoint Position;
    FICDisplayMode Current;
    bool bPrimary=false;
};
namespace ICDisplaySettings
{
    TArray<FICMonitor> Monitors();
    TArray<FICDisplayMode> Modes(const FString& Device);
    bool Apply(const FString& Device,const FICDisplayMode& Mode,FString& Error);
    void MoveGameWindow(const FICMonitor& Monitor);
}
