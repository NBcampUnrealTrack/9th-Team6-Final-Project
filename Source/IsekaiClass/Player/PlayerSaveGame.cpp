#include "PlayerSaveGame.h"
#include "Misc/CommandLine.h"

const TCHAR* GetPlayerSaveSlot()
{
    return FParse::Param(FCommandLine::Get(),TEXT("PlayerPrototypeSmoke"))?TEXT("PlayerPrototype_Smoke"):TEXT("PlayerPrototype_Auto");
}
