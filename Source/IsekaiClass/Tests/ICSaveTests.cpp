#include "../Core/ICSaveSubsystem.h"
#include "../Core/ICSaveGame.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "../Player/PlayerSaveGame.h"
#include "../Player/PlayerInventory.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FICSaveFixture
{
    TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>()};
    TStrongObjectPtr<UICWorldStateSubsystem> State{NewObject<UICWorldStateSubsystem>(Instance.Get())};
    TStrongObjectPtr<UICTimeSubsystem> Time{NewObject<UICTimeSubsystem>(Instance.Get())};
    TStrongObjectPtr<UICSaveSubsystem> Saves{NewObject<UICSaveSubsystem>(Instance.Get())};
    FICSaveFixture()
    {
        Saves->ConfigureServices(State.Get(), Time.Get(), TEXT("IC_Test_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
        FICWorldState Initial; Initial.bPlayerCreated = true; Initial.PlayerName = TEXT("SaveTest"); Initial.FirstSkill = TEXT("SKL_WAR_001");
        FICCharacterState Player; Player.CharacterId = UICWorldStateSubsystem::PlayerId(); Player.Kind = EICCharacterKind::Player;
        Initial.Characters.Add(Player.CharacterId, Player); State->RestoreState(Initial);
        Time->SetRunning(true); Time->SetMenuPaused(false);
    }
    ~FICSaveFixture() { Cleanup(); }
    void Cleanup() const
    {
        for (int32 Slot = 0; Slot <= 4; ++Slot)
        {
            const FString Path = Saves->GetSlotPath(Slot);
            for (const FString& Suffix : TArray<FString>{ TEXT(""), TEXT(".tmp"), TEXT(".bak"), TEXT(".bak.tmp"), TEXT(".rollback") })
                IFileManager::Get().Delete(*(Path + Suffix), false, false, true);
        }
    }
};

bool WriteModel(UICSaveGame* Model, const FString& Path)
{
    TArray<uint8> Bytes;
    return UICSaveGame::EncodeForFile(Model, Bytes) && FFileHelper::SaveArrayToFile(Bytes, *Path);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveSlotsTest, "Isekai.Core.SaveSlotsAndRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICSaveSlotsTest::RunTest(const FString& Parameters)
{
    FICSaveFixture F;
    auto* Inventory = NewObject<UPlayerInventory>(); Inventory->BindWorldState(F.State.Get());
    Inventory->AddItem(TEXT("Sword"), 1, false); Inventory->Equip(0); Inventory->AddItem(TEXT("Wood"), 7, false);
    const FGuid WeaponID = Inventory->GetSnapshot().Equipment[0].InstanceID;
    F.State->EnsureCharacter(TEXT("NPC_R01_001"), EICCharacterKind::Recruitable); F.State->SetAffinity(TEXT("NPC_R01_001"), 100); F.State->SetPartySlot(TEXT("NPC_R01_001"), 2);
    auto Progress = F.State->ExportState(); FICQuestState Quest; Quest.bAccepted = true; Quest.Objectives.Add(TEXT("MON_R01_001"), 2);
    Progress.Quests.Add(TEXT("QST_R01_MAIN_001"), Quest); Progress.UnlockedWarps.Add(TEXT("WRP_R01_001"));
    Progress.MonsterKilledAt.Add(TEXT("MON_R01_001"), 10); Progress.ResourceReadyAt.Add(TEXT("SO_TREE_R01_001"), 300);
    Progress.bEndingCleared = true; Progress.EndingKind = TEXT("Ending.Hidden");
    TestTrue(TEXT("Store world progress"), F.State->ApplyWorldProgress(Progress));
    F.Time->AdvanceClock(120, false);
    for (int32 Slot = 1; Slot <= 4; ++Slot)
    {
        F.State->SetMainStat(UICWorldStateSubsystem::PlayerId(), EICMainStat::STR, Slot + 1);
        TestTrue(TEXT("Save distinct manual slot"), F.Saves->SaveSlotSynchronously(Slot));
        F.Time->JumpGameMinutes(60);
    }
    TestEqual(TEXT("Exactly five visible slots"), F.Saves->GetSlotList().Num(), 5);
    F.Time->JumpGameMinutes(600);
    const auto SavedSlots = F.Saves->GetSlotList();
    for (int32 Slot = 1; Slot <= 4; ++Slot)
    {
        TestEqual(TEXT("Slot metadata keeps its own saved game time"), SavedSlots[Slot].Clock.GameMinute, 420.0 + (Slot - 1) * 60);
        TestEqual(TEXT("Slot metadata keeps its saved play time"), SavedSlots[Slot].Clock.PlaySecond, 120.0);
    }
    TestTrue(TEXT("Load manual 1"), F.Saves->LoadSlot(1));
    TestEqual(TEXT("Slot 1 stat"), F.State->GetSnapshot(UICWorldStateSubsystem::PlayerId()).MainStats.STR, 2);
    TestEqual(TEXT("Clock restores without sleep pollution"), F.Time->GetClock().GameMinute, 420.0);
    TestEqual(TEXT("PlayClock restored"), F.Time->GetClock().PlaySecond, 120.0);
    TestEqual(TEXT("Equipment identity preserved"), F.State->GetInventorySnapshot().Equipment[0].InstanceID, WeaponID);
    TestEqual(TEXT("Companion slot preserved"), F.State->GetSnapshot(TEXT("NPC_R01_001")).PartySlot, 2);
    TestEqual(TEXT("Quest objective preserved"), F.State->ExportState().Quests[TEXT("QST_R01_MAIN_001")].Objectives[TEXT("MON_R01_001")], 2);
    TestEqual(TEXT("Ending preserved"), F.State->ExportState().EndingKind, FName(TEXT("Ending.Hidden")));
    TestTrue(TEXT("Load manual 4"), F.Saves->LoadSlot(4));
    TestEqual(TEXT("Slot 4 isolated stat"), F.State->GetSnapshot(UICWorldStateSubsystem::PlayerId()).MainStats.STR, 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveRecoveryTest, "Isekai.Core.SaveRecoveryAndVersions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICSaveRecoveryTest::RunTest(const FString& Parameters)
{
    FICSaveFixture F;
    TestTrue(TEXT("First auto save"), F.Saves->SaveSlotSynchronously(0));
    F.State->SetMainStat(UICWorldStateSubsystem::PlayerId(), EICMainStat::STR, 7);
    F.Time->JumpGameMinutes(480);
    TestTrue(TEXT("Second auto save"), F.Saves->SaveSlotSynchronously(0));
    TestTrue(TEXT("Previous auto backup exists"), IFileManager::Get().FileExists(*(F.Saves->GetSlotPath(0) + TEXT(".bak"))));
    TArray<uint8> Corrupt; FFileHelper::LoadFileToArray(Corrupt, *F.Saves->GetSlotPath(0));
    Corrupt.Last() ^= 0x7f; FFileHelper::SaveArrayToFile(Corrupt, *F.Saves->GetSlotPath(0));
    TestTrue(TEXT("CRC detects corruption and loads backup"), F.Saves->LoadSlot(0));
    TestEqual(TEXT("Backup is previous save"), F.State->GetSnapshot(UICWorldStateSubsystem::PlayerId()).MainStats.STR, 1);
    TestTrue(TEXT("UI marks recovered backup"), F.Saves->GetSlotList()[0].bUsesBackup);
    TestEqual(TEXT("Slot metadata uses the recovered backup game time"), F.Saves->GetSlotList()[0].Clock.GameMinute, 360.0);
    auto* Model = NewObject<UICSaveGame>(); Model->WorldState = F.State->ExportState(); Model->Clock = F.Time->GetClock(); Model->Slot = 1; Model->SaveVersion = 999;
    TestTrue(TEXT("Write future fixture"), WriteModel(Model, F.Saves->GetSlotPath(1)));
    TestFalse(TEXT("Future version load refused"), F.Saves->LoadSlot(1));
    TestFalse(TEXT("Future version overwrite refused"), F.Saves->SaveSlotSynchronously(1));
    TestEqual(TEXT("Failed load preserves state"), F.State->GetSnapshot(UICWorldStateSubsystem::PlayerId()).MainStats.STR, 1);
    Model->SaveVersion = UICSaveGame::CurrentVersion; Model->Clock.PlaySecond = -1;
    TestTrue(TEXT("Write invalid clock fixture"), WriteModel(Model, F.Saves->GetSlotPath(1)));
    TestFalse(TEXT("Invalid clock load refused"), F.Saves->LoadSlot(1));
    TestEqual(TEXT("Invalid clock does not partially load"), F.Time->GetClock().PlaySecond, 0.0);
    TestFalse(TEXT("Missing slot load refused"), F.Saves->LoadSlot(4));
    TestFalse(TEXT("Out of range save refused"), F.Saves->SaveSlotSynchronously(5));
    Model->Clock = F.Time->GetClock(); Model->Slot = 2;
    TestTrue(TEXT("Write wrong-slot fixture"), WriteModel(Model, F.Saves->GetSlotPath(1)));
    TestFalse(TEXT("Wrong slot metadata refused"), F.Saves->LoadSlot(1));
    TArray<uint8> Truncated; UICSaveGame::EncodeForFile(Model, Truncated); Truncated.SetNum(10);
    TestNull(TEXT("Truncated envelope refused"), UICSaveGame::DecodeFile(Truncated));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICLegacySaveTest, "Isekai.Core.LegacySaveConversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICLegacySaveTest::RunTest(const FString& Parameters)
{
    FICSaveFixture F; auto* Legacy = NewObject<UPlayerPrototypeSave>();
    Legacy->PlayerName = TEXT("Legacy"); Legacy->FirstSkill = TEXT("SKL_HEAL_001"); Legacy->Job = 3;
    Legacy->Position = FVector(10, 20, 130); Legacy->HP = 50; Legacy->SP = 40; Legacy->Strength = 12;
    const FICInventoryState Inventory; Legacy->Bag = Inventory.Bag; Legacy->Equipment = Inventory.Equipment;
    Legacy->QuickSlots = Inventory.QuickSlots; Legacy->Storage = Inventory.Storage;
    FICWorldState Converted;
    TestTrue(TEXT("Convert valid v1"), F.Saves->ConvertLegacySave(Legacy, Converted));
    TestEqual(TEXT("Legacy stats preserved"), Converted.Characters[UICWorldStateSubsystem::PlayerId()].MainStats.STR, 12);
    TestEqual(TEXT("Legacy identifiers preserved"), Converted.FirstSkill, FName(TEXT("SKL_HEAL_001")));
    Legacy->Version = 99;
    TestFalse(TEXT("Unknown legacy version refused"), F.Saves->ConvertLegacySave(Legacy, Converted));
    Legacy->Version = 1; Legacy->HP = -1;
    TestFalse(TEXT("Invalid legacy vitals refused"), F.Saves->ConvertLegacySave(Legacy, Converted));
    Legacy->HP = 50;
    const FString LegacyName = FPaths::GetBaseFilename(F.Saves->GetSlotPath(1)).LeftChop(8) + TEXT("_LegacyV1");
    const FString LegacyPath = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (LegacyName + TEXT(".sav"));
    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(LegacyName, 0); };
    TestTrue(TEXT("Write isolated legacy fixture"), UGameplayStatics::SaveGameToSlot(Legacy, LegacyName, 0));
    TArray<uint8> OriginalBytes, AfterBytes; FFileHelper::LoadFileToArray(OriginalBytes, *LegacyPath);
    TestTrue(TEXT("Import real v1 file to empty manual 1"), F.Saves->ImportLegacySave());
    TestEqual(TEXT("Imported character restored"), F.State->GetSnapshot(UICWorldStateSubsystem::PlayerId()).MainStats.STR, 12);
    FFileHelper::LoadFileToArray(AfterBytes, *LegacyPath);
    TestTrue(TEXT("Original legacy bytes preserved"), OriginalBytes == AfterBytes);
    TestFalse(TEXT("Import refuses occupied manual 1"), F.Saves->ImportLegacySave());
    TestTrue(TEXT("Legacy delete succeeds"),F.Saves->DeleteLegacySave());
    TestFalse(TEXT("Legacy original absent"),F.Saves->HasLegacySave());
    TestTrue(TEXT("Converted manual remains valid"),F.Saves->GetSlotList()[1].bValid);
    return true;
}

class FICWaitForDeferredAutoSave : public IAutomationLatentCommand
{
    TSharedPtr<FICSaveFixture> Fixture;
    FAutomationTestBase* Test;
    double Started = FPlatformTime::Seconds();
    bool bPeriodicRequested = false;
    bool bNewRequestHandled = false;
public:
    FICWaitForDeferredAutoSave(TSharedPtr<FICSaveFixture> InFixture, FAutomationTestBase* InTest) : Fixture(InFixture), Test(InTest) {}
    virtual bool Update() override
    {
        if (Fixture->Saves->bSaving && FPlatformTime::Seconds() - Started < 10) return false;
        Test->TestFalse(TEXT("Async save completed"), Fixture->Saves->bSaving);
        if (!bNewRequestHandled)
        {
            Test->TestTrue(TEXT("Request arriving during IO remains pending"), Fixture->Saves->bAutoSavePending);
            Fixture->Saves->PollAutoSave();
            Test->TestTrue(TEXT("New request starts next save"), Fixture->Saves->bSaving);
            bNewRequestHandled = true; Started = FPlatformTime::Seconds(); return false;
        }
        Test->TestFalse(TEXT("Deferred auto request cleared"), Fixture->Saves->bAutoSavePending);
        Test->TestTrue(TEXT("Deferred auto produced a valid slot"), Fixture->Saves->GetSlotList()[0].bValid);
        if (!bPeriodicRequested)
        {
            Fixture->Saves->AutoSaveInterval = 300;
            Fixture->Time->AdvanceClock(299, false); Fixture->Saves->PollAutoSave();
            Test->TestFalse(TEXT("No interval save before 300 PlayClock seconds"), Fixture->Saves->bSaving);
            Fixture->Time->JumpGameMinutes(1440); Fixture->Saves->PollAutoSave();
            Test->TestFalse(TEXT("Sleep jump does not advance auto interval"), Fixture->Saves->bSaving);
            Fixture->Time->AdvanceClock(1, false); Fixture->Saves->PollAutoSave();
            Test->TestTrue(TEXT("300 PlayClock seconds starts auto save"), Fixture->Saves->bSaving);
            bPeriodicRequested = true; Started = FPlatformTime::Seconds(); return false;
        }
        return true;
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICDeferredSaveTest, "Isekai.Core.AutoSaveSafetyAndDeferral", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICDeferredSaveTest::RunTest(const FString& Parameters)
{
    auto F = MakeShared<FICSaveFixture>(); FICSaveSafety Safety; Safety.bInCombat = true; F->Saves->SetSafetyState(Safety);
    TestFalse(TEXT("Combat blocks manual"), F->Saves->SaveSlotSynchronously(1));
    F->Saves->RequestAutoSave(TEXT("QuestComplete"));
    TestTrue(TEXT("Combat defers auto"), F->Saves->bAutoSavePending);
    TestFalse(TEXT("Combat does not start IO"), F->Saves->bSaving);
    Safety.bInCombat = false; Safety.bInDialogue = true; F->Saves->SetSafetyState(Safety);
    TestFalse(TEXT("Dialogue blocks save"), F->Saves->CanSave(false));
    Safety.bInDialogue = false; Safety.bInEvent = true; F->Saves->SetSafetyState(Safety);
    TestFalse(TEXT("Event blocks save"), F->Saves->CanSave(false));
    Safety.bInEvent = false; F->Saves->SetSafetyState(Safety);
    F->State->SetVitals(UICWorldStateSubsystem::PlayerId(), 10, 100, 70);
    TestFalse(TEXT("Low HP defers auto"), F->Saves->CanSave(true));
    TestTrue(TEXT("Low HP permits manual"), F->Saves->CanSave(false));
    F->State->SetVitals(UICWorldStateSubsystem::PlayerId(), 60, 100, 70);
    auto Player = *F->State->GetStoredCharacter(UICWorldStateSubsystem::PlayerId()); FICDebuffState D; D.ID = TEXT("DBF_001"); D.RemainingSeconds = 10;
    Player.Debuffs.Add(D); F->State->SetCharacterState(Player);
    TestFalse(TEXT("Active debuff defers auto"), F->Saves->CanSave(true));
    Player.Debuffs.Reset(); F->State->SetCharacterState(Player);
    F->Saves->PollAutoSave(); TestTrue(TEXT("Safe state starts deferred async IO"), F->Saves->bSaving);
    TestFalse(TEXT("Busy slot deletion blocked"),F->Saves->DeleteSlot(0));
    TestFalse(TEXT("Busy legacy deletion blocked"),F->Saves->DeleteLegacySave());
    F->Saves->RequestAutoSave(TEXT("WarpDuringSave"));
    ADD_LATENT_AUTOMATION_COMMAND(FICWaitForDeferredAutoSave(F, this));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveDeletionTest,"Isekai.Core.SaveDeletion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSaveDeletionTest::RunTest(const FString& Parameters)
{
    FICSaveFixture F;
    TestTrue(TEXT("Keep other manual"),F.Saves->SaveSlotSynchronously(4));
    TArray<uint8> Other;FFileHelper::LoadFileToArray(Other,*F.Saves->GetSlotPath(4));
    TestTrue(TEXT("Auto first"),F.Saves->SaveSlotSynchronously(0));
    TestTrue(TEXT("Auto second with backup"),F.Saves->SaveSlotSynchronously(0));
    const auto Before=F.State->ExportState();const auto Clock=F.Time->GetClock();
    for(const FString& Suffix:TArray<FString>{TEXT(".tmp"),TEXT(".rollback"),TEXT(".bak.tmp")})
        FFileHelper::SaveStringToFile(TEXT("orphan"),*(F.Saves->GetSlotPath(0)+Suffix));
    F.Saves->SetAutoSaveSuspended(true);F.Saves->RequestAutoSave(TEXT("Pending"));
    TestTrue(TEXT("Auto deletion succeeds"),F.Saves->DeleteSlot(0));
    TestFalse(TEXT("Deletion clears pending auto"),F.Saves->bAutoSavePending);
    F.Saves->SetAutoSaveSuspended(false);F.Saves->PollAutoSave();
    TestFalse(TEXT("Deleted auto not immediately recreated"),F.Saves->bSaving);
    TestFalse(TEXT("Deleted auto has no ghost backup"),F.Saves->LoadSlot(0));
    TestFalse(TEXT("Deleted slot shown empty"),F.Saves->GetSlotList()[0].bExists);
    TestEqual(TEXT("Deletion preserves active character"),F.State->ExportState().PlayerName,Before.PlayerName);
    TestEqual(TEXT("Deletion preserves active clock"),F.Time->GetClock().PlaySecond,Clock.PlaySecond);
    TArray<uint8> After;FFileHelper::LoadFileToArray(After,*F.Saves->GetSlotPath(4));
    TestTrue(TEXT("Other save bytes unchanged"),Other==After);
    FFileHelper::SaveStringToFile(TEXT("corrupt"),*(F.Saves->GetSlotPath(3)+TEXT(".tmp")));
    TestTrue(TEXT("Orphan temporary visible for deletion"),F.Saves->GetSlotList()[3].bExists);
    TestTrue(TEXT("Corrupt orphan deleted"),F.Saves->DeleteSlot(3));
    TestFalse(TEXT("Orphan no longer visible"),F.Saves->GetSlotList()[3].bExists);
    TestFalse(TEXT("Invalid deletion rejected"),F.Saves->DeleteSlot(5));
    return true;
}
#endif
