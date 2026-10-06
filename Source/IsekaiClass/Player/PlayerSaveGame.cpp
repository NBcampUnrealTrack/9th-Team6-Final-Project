#include "PlayerSaveGame.h"
#include "Misc/CommandLine.h"

const TCHAR* GetPlayerSaveSlot()
{
    return FParse::Param(FCommandLine::Get(),TEXT("PlayerPrototypeSmoke"))?TEXT("PlayerPrototype_Smoke"):TEXT("PlayerPrototype_Auto");
}

const TCHAR* GetPlayerManualSaveSlot(int32 Index)
{
    static const TCHAR* Slots[]={TEXT("PlayerPrototype_Manual1"),TEXT("PlayerPrototype_Manual2"),TEXT("PlayerPrototype_Manual3"),TEXT("PlayerPrototype_Manual4")};
    return Slots[FMath::Clamp(Index,0,3)];
}
