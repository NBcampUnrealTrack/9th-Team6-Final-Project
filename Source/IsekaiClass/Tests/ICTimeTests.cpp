#include "../Core/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICTimeRulesTest, "Isekai.Core.TimeRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICTimeRulesTest::RunTest(const FString& Parameters)
{
    auto* Instance = NewObject<UGameInstance>(); auto* Time = NewObject<UICTimeSubsystem>(Instance);
    const auto Initial = Time->GetClock(); Time->AdvanceClock(10, false);
    TestEqual(TEXT("Creation does not advance"), Time->GetClock().GameMinute, Initial.GameMinute);
    Time->SetRunning(true); Time->SetMenuPaused(false); Time->AdvanceClock(2880, false);
    TestEqual(TEXT("48 real minutes is a game day"), Time->GetClock().GameMinute, Initial.GameMinute + 1440);
    TestEqual(TEXT("PlayClock measures active seconds"), Time->GetClock().PlaySecond, 2880.0);
    const auto Before = Time->GetClock(); Time->SetMenuPaused(true); Time->AdvanceClock(300, false);
    TestEqual(TEXT("Menu pauses GameClock"), Time->GetClock().GameMinute, Before.GameMinute);
    TestEqual(TEXT("Menu pauses PlayClock"), Time->GetClock().PlaySecond, Before.PlaySecond);
    Time->SetMenuPaused(false); Time->AdvanceClock(30, true);
    TestEqual(TEXT("World pause stops both"), Time->GetClock().PlaySecond, Before.PlaySecond);
    TestTrue(TEXT("Sleep jump"), Time->JumpGameMinutes(480));
    TestEqual(TEXT("Sleep advances only game time"), Time->GetClock().GameMinute, Before.GameMinute + 480);
    TestEqual(TEXT("Sleep does not advance timers"), Time->GetClock().PlaySecond, Before.PlaySecond);
    TestFalse(TEXT("Reject backward jump"), Time->JumpGameMinutes(-1));
    Time->AdvanceClock(-10, false); TestEqual(TEXT("Reject negative delta"), Time->GetClock().PlaySecond, Before.PlaySecond);
    TestTrue(TEXT("Restore clock"), Time->RestoreClock(Before));
    FICClockState Invalid = Before; Invalid.PlaySecond = -1;
    TestFalse(TEXT("Invalid saved clock rejected"), Time->RestoreClock(Invalid));
    TestEqual(TEXT("Invalid restore is atomic"), Time->GetClock().GameMinute, Before.GameMinute);
    return true;
}
#endif
