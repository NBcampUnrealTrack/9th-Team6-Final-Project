#pragma once
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"

// EndPlayMap persists custom PIE window sizes. Restore the user's launch size afterwards.
class FICPlayWindowSettingsRestore : public IAutomationLatentCommand
{
public:
    FICPlayWindowSettingsRestore()
    {
        const auto* Settings=GetDefault<ULevelEditorPlaySettings>();
        Width=Settings->NewWindowWidth;Height=Settings->NewWindowHeight;LastSize=Settings->LastSize;
        Position=Settings->NewWindowPosition;
    }
    bool Update() override
    {
        auto* Settings=GetMutableDefault<ULevelEditorPlaySettings>();
        Settings->NewWindowWidth=Width;Settings->NewWindowHeight=Height;
        Settings->LastSize=LastSize;Settings->NewWindowPosition=Position;Settings->SaveConfig();return true;
    }
private:
    int32 Width,Height;
    FIntPoint LastSize,Position;
};
