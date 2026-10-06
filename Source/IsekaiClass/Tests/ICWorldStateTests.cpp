#include "../Core/ICWorldStateSubsystem.h"
#include "../Player/PlayerPrototypeController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICWorldRulesTest, "Isekai.Core.WorldStateRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICWorldRulesTest::RunTest(const FString& Parameters)
{
    auto* Instance = NewObject<UGameInstance>();
    auto* State = NewObject<UICWorldStateSubsystem>(Instance);
    TestTrue(TEXT("Create player state"), State->EnsureCharacter(UICWorldStateSubsystem::PlayerId(), EICCharacterKind::Player));
    TestTrue(TEXT("Create recruitable"), State->EnsureCharacter(TEXT("NPC_R01_001"), EICCharacterKind::Recruitable));
    TestTrue(TEXT("Create resident"), State->EnsureCharacter(TEXT("NPC_R01_101"), EICCharacterKind::Resident));
    TestFalse(TEXT("Reject malformed character id"), State->EnsureCharacter(TEXT("NPC_INVALID"), EICCharacterKind::Recruitable));
    TestFalse(TEXT("NPC serial range must match kind"), State->EnsureCharacter(TEXT("NPC_R01_100"), EICCharacterKind::Recruitable));
    TestFalse(TEXT("Resident cannot use recruitable range"), State->EnsureCharacter(TEXT("NPC_R01_003"), EICCharacterKind::Resident));
    TestFalse(TEXT("Reject affinity above 100"), State->SetAffinity(TEXT("NPC_R01_001"), 101));
    TestFalse(TEXT("Reject stat above cap"), State->SetMainStat(UICWorldStateSubsystem::PlayerId(), EICMainStat::STR, 10001));
    TestFalse(TEXT("Residents have no growth stats"), State->SetMainStat(TEXT("NPC_R01_101"), EICMainStat::STR, 2));
    TestFalse(TEXT("Recruit requires affinity 100"), State->SetPartySlot(TEXT("NPC_R01_001"), 0));
    TestTrue(TEXT("Affinity 100"), State->SetAffinity(TEXT("NPC_R01_001"), 100));
    TestTrue(TEXT("Recruit into first slot"), State->SetPartySlot(TEXT("NPC_R01_001"), 0));
    State->EnsureCharacter(TEXT("NPC_R01_002"), EICCharacterKind::Recruitable);
    State->SetAffinity(TEXT("NPC_R01_002"), 100);
    TestFalse(TEXT("Reject occupied party slot"), State->SetPartySlot(TEXT("NPC_R01_002"), 0));
    TestFalse(TEXT("Reject fifth slot"), State->SetPartySlot(TEXT("NPC_R01_002"), 4));
    TestTrue(TEXT("Affinity can decrease while accompanying"), State->SetAffinity(TEXT("NPC_R01_001"), 90));
    TestTrue(TEXT("Companion downed"), State->SetCharacterMode(TEXT("NPC_R01_001"), EICCharacterMode::Downed));
    TestFalse(TEXT("Cannot dismiss downed companion"), State->SetPartySlot(TEXT("NPC_R01_001"), INDEX_NONE));
    TestTrue(TEXT("Revive mode"), State->SetCharacterMode(TEXT("NPC_R01_001"), EICCharacterMode::Companion));
    TestTrue(TEXT("Dismiss living companion"), State->SetPartySlot(TEXT("NPC_R01_001"), INDEX_NONE));
    TestFalse(TEXT("Rehire still requires affinity 100"), State->SetPartySlot(TEXT("NPC_R01_001"), 0));
    auto Copy = State->GetSnapshot(TEXT("NPC_R01_001")); Copy.Affinity = 0;
    TestEqual(TEXT("Snapshot mutations cannot modify source"), State->GetSnapshot(TEXT("NPC_R01_001")).Affinity, 90);
    auto Invalid = State->ExportState(); Invalid.Characters[TEXT("NPC_R01_001")].Affinity = -1;
    TestFalse(TEXT("Invalid restore rejected"), State->RestoreState(Invalid));
    TestEqual(TEXT("Rejected restore preserves state"), State->GetSnapshot(TEXT("NPC_R01_001")).Affinity, 90);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICParticipantTest, "Isekai.Core.ParticipantRegistration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICParticipantTest::RunTest(const FString& Parameters)
{
    auto* Instance = NewObject<UGameInstance>(); auto* State = NewObject<UICWorldStateSubsystem>(Instance);
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* First = World->SpawnActor<APlayerPrototypeController>();
    auto* Second = World->SpawnActor<APlayerPrototypeController>();
    TestNotNull(TEXT("First participant actor"), First); TestNotNull(TEXT("Second participant actor"), Second);
    if (First && Second)
    {
        TestTrue(TEXT("Register first"), State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(), First));
        TestTrue(TEXT("Idempotent registration"), State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(), First));
        TestFalse(TEXT("Reject duplicate live actor"), State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(), Second));
        State->UnregisterStateParticipant(UICWorldStateSubsystem::PlayerId(), Second);
        TestFalse(TEXT("Wrong actor cannot unregister first"), State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(), Second));
        State->UnregisterStateParticipant(UICWorldStateSubsystem::PlayerId(), First);
        TestTrue(TEXT("Register replacement"), State->RegisterStateParticipant(UICWorldStateSubsystem::PlayerId(), Second));
        State->UnregisterStateParticipant(UICWorldStateSubsystem::PlayerId(), Second);
    }
    TestFalse(TEXT("Reject object without interface"), State->RegisterStateParticipant(TEXT("NPC_R01_001"), Instance));
    World->DestroyWorld(false);
    return true;
}
#endif
