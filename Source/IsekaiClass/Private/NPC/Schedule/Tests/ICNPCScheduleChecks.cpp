#include "Misc/AutomationTest.h"
#include "NPC/Schedule/ICNPCScheduleLibrary.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/NPC/ICNPCRow.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "Data/Rows/World/ICAnchorRow.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCScheduleResolutionCheck, "IC.NPC.Schedule.Resolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICNPCScheduleResolutionCheck::RunTest(const FString&)
{
    FICScheduleRow Schedule;
    FICScheduleSegment Sleep;
    Sleep.SegmentID = TEXT("Sleep");
    Sleep.StartGameMinute = 0;
    Sleep.EndGameMinute = 360;
    Sleep.Activity = EICScheduleActivity::Sleep;
    Sleep.DestinationAnchorID = TEXT("ANC_BED");
    FICScheduleSegment Work;
    Work.SegmentID = TEXT("Work");
    Work.StartGameMinute = 360;
    Work.EndGameMinute = 1200;
    Work.Activity = EICScheduleActivity::Work;
    Work.DestinationAnchorID = TEXT("ANC_SHOP");
    FICScheduleSegment Idle;
    Idle.SegmentID = TEXT("Idle");
    Idle.StartGameMinute = 1200;
    Idle.EndGameMinute = 1440;
    Idle.Activity = EICScheduleActivity::Idle;
    Idle.DestinationAnchorID = TEXT("ANC_HOME");
    Schedule.Segments = {Sleep, Work, Idle};

    const FICNPCScheduleResolution BeforeWork = UICNPCScheduleLibrary::ResolveSchedule(Schedule, 359.9);
    TestTrue(TEXT("Before work has a schedule"), BeforeWork.bFound);
    TestEqual(TEXT("Before work remains asleep"), BeforeWork.SegmentID, FName(TEXT("Sleep")));
    const FICNPCScheduleResolution AtWork = UICNPCScheduleLibrary::ResolveSchedule(Schedule, 360.0);
    TestEqual(TEXT("Work begins at its inclusive start"), AtWork.SegmentID, FName(TEXT("Work")));
    const FICNPCScheduleResolution AtNight = UICNPCScheduleLibrary::ResolveSchedule(Schedule, 1440.0);
    TestEqual(TEXT("Next day wraps to sleep"), AtNight.SegmentID, FName(TEXT("Sleep")));
    const FICNPCScheduleResolution Missing = UICNPCScheduleLibrary::ResolveSchedule(FICScheduleRow(), 480.0);
    TestFalse(TEXT("Empty schedule has no active segment"), Missing.bFound);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICR01NPCDefinitionCheck, "IC.NPC.Data.R01Complete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICR01NPCDefinitionCheck::RunTest(const FString&)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UICDefinitionSubsystem* Definitions = GameInstance->GetSubsystem<UICDefinitionSubsystem>();
    TestNotNull(TEXT("Definition subsystem is available"), Definitions);
    if (Definitions)
    {
        for (int32 Number = 1; Number <= 11; ++Number)
        {
            const FName NPCID(*FString::Printf(TEXT("NPC_R01_%03d"), Number));
            FICDefinitionResult Result;
            const FICNPCRow* NPC = Definitions->Find<FICNPCRow>(NPCID, Result);
            if (!TestNotNull(*FString::Printf(TEXT("%s definition exists"), *NPCID.ToString()), NPC)) continue;
            TestEqual(*FString::Printf(TEXT("%s belongs to R01"), *NPCID.ToString()), NPC->RegionID, FName(TEXT("R01")));
            TestFalse(*FString::Printf(TEXT("%s has a schedule"), *NPCID.ToString()), NPC->ScheduleID.IsNone());
            TestFalse(*FString::Printf(TEXT("%s has an initial anchor"), *NPCID.ToString()), NPC->InitialAnchorID.IsNone());
            const FICScheduleRow* Schedule = Definitions->Find<FICScheduleRow>(NPC->ScheduleID, Result);
            TestNotNull(*FString::Printf(TEXT("%s schedule definition exists"), *NPCID.ToString()), Schedule);
            TestNotNull(*FString::Printf(TEXT("%s initial anchor definition exists"), *NPCID.ToString()),
                Definitions->Find<FICAnchorRow>(NPC->InitialAnchorID, Result));
            if (Schedule)
            {
                TestFalse(*FString::Printf(TEXT("%s schedule has segments"), *NPCID.ToString()), Schedule->Segments.IsEmpty());
                for (const FICScheduleSegment& Segment : Schedule->Segments)
                {
                    TestFalse(*FString::Printf(TEXT("%s/%s has a destination anchor"), *NPCID.ToString(), *Segment.SegmentID.ToString()),
                        Segment.DestinationAnchorID.IsNone());
                    TestNotNull(*FString::Printf(TEXT("%s/%s destination anchor exists"), *NPCID.ToString(), *Segment.SegmentID.ToString()),
                        Definitions->Find<FICAnchorRow>(Segment.DestinationAnchorID, Result));
                }
            }
        }
    }
    UWorld* World = GameInstance->GetWorld();
    GameInstance->Shutdown();
    if (World)
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }
    return true;
}
#endif
