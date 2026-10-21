#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Debug/ICLog.h"
#include "Misc/Base64.h"
#include "ICWorldStateV2Fixture.h"
#include "Misc/Crc.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveGame.h"
#include "Save/ICSaveSubsystem.h"
#include "Save/Compatibility/ICLegacyPlayerSaveGame.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "UObject/StrongObjectPtr.h"
#include "Life/Survival/ICPlayerSurvival.h"
#include "Player/State/ICPlayerStateConversions.h"

namespace
{
FICWorldState PlayerWorld()
{
    IC_TRACE_FUNCTION();
    FICWorldState S; S.bPlayerCreated=true; S.PlayerName=TEXT("테스트"); S.FirstSkill=TEXT("SKL_TEST");
    FICCharacterState Player; Player.CharacterId=UICWorldStateSubsystem::PlayerId(); Player.Kind=EICCharacterKind::Player;
    S.Characters.Add(Player.CharacterId,Player);
    return S;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSPClockCheck,"IC.WorldState.SPPlayClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSPClockCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    FPlayerSurvivalState S;
    TestEqual(TEXT("Nine seconds do not drain"),S.AdvanceNaturalSP(9),0.f);
    TestEqual(TEXT("Threshold drains exactly one"),S.AdvanceNaturalSP(1),1.f);
    TestEqual(TEXT("Initial SP retained"),S.SP,69.f);
    TestEqual(TEXT("Long frame drains three units"),S.AdvanceNaturalSP(35),3.f);
    TestEqual(TEXT("Fractional interval is retained"),S.SPDrainElapsedPlaySeconds,5.0);
    FICCharacterState Character; ICPlayerStateConversions::Apply(FPlayerMainStats(),S,Character);
    auto Restored=ICPlayerStateConversions::Survival(Character);
    TestEqual(TEXT("Load continues the remaining five seconds"),Restored.AdvanceNaturalSP(5),1.f);
    TestEqual(TEXT("Negative delta cannot drain"),Restored.AdvanceNaturalSP(-10),0.f);
    Restored.CompleteSleep(); TestEqual(TEXT("Sleep clears partial drain"),Restored.SPDrainElapsedPlaySeconds,0.0);
    Restored.SP=1; TestEqual(TEXT("Exhaustion clamps at zero"),Restored.AdvanceNaturalSP(100),1.f);
    TestTrue(TEXT("Exhausted state is valid"),Restored.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPauseClockCheck,"IC.WorldState.PauseOwnership",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPauseClockCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    auto* Time=NewObject<UICTimeSubsystem>(NewObject<UGameInstance>());auto* Menu=NewObject<UGameInstance>();auto* Popup=NewObject<UGameInstance>();
    Time->SetRunning(true);Time->SetMenuPaused(false);Time->AdvanceClock(10,false);
    TestEqual(TEXT("Initial game clock"),Time->GetClock().GameMinute,485.0);
    Time->SetPauseRequest(Menu,TEXT("Menu"),true);Time->SetPauseRequest(Popup,TEXT("Popup"),true);
    Time->SetPauseRequest(Menu,TEXT("Menu"),true);
    Time->AdvanceClock(50,false);TestEqual(TEXT("Both clocks freeze"),Time->GetClock().PlaySecond,10.0);
    Time->ReleasePauseRequests(Menu);TestFalse(TEXT("Popup still owns pause"),Time->IsRunning());
    Time->AdvanceClock(50,false);TestEqual(TEXT("Game time still frozen"),Time->GetClock().GameMinute,485.0);
    Time->SetPauseRequest(Popup,TEXT("Popup"),false);TestTrue(TEXT("Last request resumes"),Time->IsRunning());
    Time->AdvanceClock(5,true);TestEqual(TEXT("External world pause is respected"),Time->GetClock().PlaySecond,10.0);
    Time->AdvanceClock(5,false);TestEqual(TEXT("Actual play resumes"),Time->GetClock().PlaySecond,15.0);
    Time->JumpGameMinutes(60);TestEqual(TEXT("Sleep jump leaves PlayClock unchanged"),Time->GetClock().PlaySecond,15.0);
    const FICClockState Saved=Time->GetClock();Time->PrepareForTravel();
    Time->RestoreClock(Saved);Time->AdvanceClock(100,false);
    TestEqual(TEXT("Restore/travel do not include offline time"),Time->GetClock().PlaySecond,15.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICStateRoundTripCheck,"IC.WorldState.SaveRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICStateRoundTripCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    auto* Save=NewObject<UICSaveGame>();Save->WorldState=PlayerWorld();Save->Clock.GameMinute=900;Save->Clock.PlaySecond=333;
    auto& S=Save->WorldState;auto& Player=S.Characters.FindChecked(UICWorldStateSubsystem::PlayerId());
    Player.SPDrainElapsedPlaySeconds=7;Player.UnlockedSkillIDs.Add(TEXT("SKL_TEST"));Player.CurrentAnchorID=TEXT("ANC_HOME");
    Player.Combat.SkillReadyAtPlaySecond.Add(TEXT("SKL_TEST"),350);
    FICActiveEffectState Effect;Effect.InstanceID=FGuid::NewGuid();Effect.EffectID=TEXT("EFF_TEST");Effect.bHarmful=true;Effect.ExpiresAtPlaySecond=360;Effect.CapturedMagnitudes.Add(TEXT("Power"),12.5);Player.Combat.Effects.Add(Effect);
    FICProductionJobState Job;Job.JobID=FGuid::NewGuid();Job.OwnerCharacterID=Player.CharacterId;Job.ProductionID=TEXT("PRD_TEST");Job.WorkAnchorID=TEXT("ANC_WORK");Job.StartedAtPlaySecond=300;Job.CompletesAtPlaySecond=360;S.ProductionJobs.Add(Job.JobID,Job);
    FICCropState Crop;Crop.CropID=TEXT("CRP_TEST");Crop.OwnerCharacterID=Player.CharacterId;Crop.PlantedAtGameMinute=500;Crop.HarvestableAtGameMinute=1000;S.Crops.Add(TEXT("ANC_CROP"),Crop);
    FICResourcePointState Resource;Resource.DefinitionID=TEXT("RES_TEST");Resource.ReadyAtPlaySecond=500;S.ResourcePoints.Add(TEXT("ANC_RESOURCE"),Resource);
    FICSpawnSlotState Spawn;Spawn.SpawnerID=TEXT("SPN_TEST");Spawn.MonsterID=TEXT("MON_TEST");Spawn.bDefeated=true;Spawn.RespawnReadyAtPlaySecond=400;S.SpawnSlots.Add(TEXT("SPN_TEST_0"),Spawn);
    FICContainerOwnershipState Ownership;Ownership.OwnerCharacterID=Player.CharacterId;Ownership.bPlayerOwned=true;Ownership.HomeID=TEXT("HOME_TEST");S.ContainerOwnership.Add(TEXT("PlayerHome"),Ownership);
    FICStorageInstallationState Storage;Storage.HomeID=Ownership.HomeID;Storage.ContainerID=TEXT("PlayerHome");Storage.ProductionID=TEXT("PRD_STORAGE");S.StorageInstallations.Add(TEXT("ANC_STORAGE"),Storage);
    S.DiscoveredRegionIDs.Add(TEXT("REG_R08"));S.UnlockedRecipeIDs.Add(TEXT("RCP_TEST"));S.Flags.Add(TEXT("FLAG_TEST"),true);
    FICQuestState Quest;Quest.bAccepted=true;Quest.bCompleted=true;Quest.bRewardClaimed=true;Quest.RewardTransactionID=FGuid::NewGuid();Quest.CompletionCount=1;Quest.Objectives.Add(TEXT("OBJ_TEST"),2);S.Quests.Add(TEXT("QST_TEST"),Quest);S.CompletedTransactionIDs.Add(Quest.RewardTransactionID);
    S.ProcessedEventIDs.Add(FGuid::NewGuid());Save->Preview.ThumbnailPNG={1,2,3};
    TArray<uint8> Bytes;TestTrue(TEXT("Encode durable schema"),UICSaveGame::EncodeForFile(Save,Bytes));
    auto* Loaded=UICSaveGame::DecodeFile(Bytes);if(!TestNotNull(TEXT("Decode durable schema"),Loaded))return false;
    const auto& Restored=Loaded->WorldState;const auto& RestoredPlayer=Restored.Characters.FindChecked(Player.CharacterId);
    TestEqual(TEXT("SP partial interval"),RestoredPlayer.SPDrainElapsedPlaySeconds,7.0);
    TestEqual(TEXT("Captured effect magnitude"),RestoredPlayer.Combat.Effects[0].CapturedMagnitudes.FindRef(TEXT("Power")),12.5);
    TestEqual(TEXT("Cooldown deadline"),RestoredPlayer.Combat.SkillReadyAtPlaySecond.FindRef(TEXT("SKL_TEST")),350.0);
    TestTrue(TEXT("Quest reward receipt"),Restored.CompletedTransactionIDs.Contains(Quest.RewardTransactionID));
    TestTrue(TEXT("Unlocks and flags"),Restored.UnlockedRecipeIDs.Contains(TEXT("RCP_TEST"))&&Restored.Flags.FindRef(TEXT("FLAG_TEST")));
    TestEqual(TEXT("Crop growth deadline"),Restored.Crops.FindChecked(TEXT("ANC_CROP")).HarvestableAtGameMinute,1000.0);
    TestEqual(TEXT("Resource PlayClock deadline"),Restored.ResourcePoints.FindChecked(TEXT("ANC_RESOURCE")).ReadyAtPlaySecond,500.0);
    TestTrue(TEXT("Spawn progress"),Restored.SpawnSlots.FindChecked(TEXT("SPN_TEST_0")).bDefeated);
    TestEqual(TEXT("Ownership persists"),Restored.ContainerOwnership.Num(),1);
    TestEqual(TEXT("Storage installation persists"),Restored.StorageInstallations.Num(),1);
    TestEqual(TEXT("Screenshot payload persists"),Loaded->Preview.ThumbnailPNG.Num(),3);
    auto* State=NewObject<UICWorldStateSubsystem>(NewObject<UGameInstance>());TestTrue(TEXT("World restore"),State->RestoreState(Restored));
    TestEqual(TEXT("Production resumes with remaining time"),State->GetProductionRemainingSeconds(Job.JobID,Loaded->Clock.PlaySecond),27.0);
    TestEqual(TEXT("Production ready clamps at zero"),State->GetProductionRemainingSeconds(Job.JobID,400),0.0);
    Bytes.Last()^=1;TestNull(TEXT("Corrupt CRC rejected"),UICSaveGame::DecodeFile(Bytes));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICReservationCheck,"IC.WorldState.ReservationValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICReservationCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    FICWorldState S=PlayerWorld();auto& Item=S.Inventory.Bag.Slots[0];Item.InstanceID=FGuid::NewGuid();Item.ItemID=TEXT("MAT_TEST");Item.Count=3;
    FICProductionJobState Job;Job.JobID=FGuid::NewGuid();Job.OwnerCharacterID=UICWorldStateSubsystem::PlayerId();Job.ProductionID=TEXT("PRD_TEST");Job.WorkAnchorID=TEXT("ANC_TEST");Job.CompletesAtPlaySecond=10;
    FICMaterialReservationState Material;Material.ItemInstanceID=Item.InstanceID;Material.ItemID=Item.ItemID;Material.Count=2;Job.Materials.Add(Material);S.ProductionJobs.Add(Job.JobID,Job);
    TestTrue(TEXT("Valid reservation"),UICWorldStateSubsystem::ValidateState(S));
    auto* State=NewObject<UICWorldStateSubsystem>(NewObject<UGameInstance>());State->RestoreState(S);
    auto Inventory=S.Inventory;Inventory.Bag.Slots[0].Count=1;
    TestFalse(TEXT("Reserved stack cannot be partially consumed"),State->CommitInventoryState(Inventory));
    TestEqual(TEXT("Failed write preserves inventory"),State->ExportState().Inventory.Bag.Slots[0].Count,3);
    FICProductionJobState Other=Job;Other.JobID=FGuid::NewGuid();S.ProductionJobs.Add(Other.JobID,Other);
    FString Reason;TestFalse(TEXT("Overlapping reservation cannot overbook"),UICWorldStateSubsystem::ValidateStateWithReason(S,Reason));
    TestTrue(TEXT("Failure identifies field"),Reason.Contains(TEXT("Materials.Count")));
    S.ProductionJobs.Remove(Other.JobID);S.ProductionJobs.FindChecked(Job.JobID).ReservedGold=201;
    TestFalse(TEXT("Reserved gold must exist"),UICWorldStateSubsystem::ValidateState(S));
    S.ProductionJobs.FindChecked(Job.JobID).ReservedGold=100;State->RestoreState(S);
    TestFalse(TEXT("Gold write cannot spend reservation"),State->SetPlayerGold(50));TestEqual(TEXT("Gold unchanged"),State->ExportState().Gold,200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveCompatibilityCheck,"IC.WorldState.LegacyV2Compatibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSaveCompatibilityCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    TArray<uint8> LegacyBytes;
    TestTrue(TEXT("Real v2 binary fixture decodes"),FBase64::Decode(FString(ICWorldStateV2Fixture),LegacyBytes));
    auto* Real=UICSaveGame::DecodeFile(LegacyBytes);
    if (!Real && LegacyBytes.Num()>16)
    {
        uint32 StoredCRC=0;FMemory::Memcpy(&StoredCRC,LegacyBytes.GetData()+12,4);
        TestEqual(TEXT("Fixture CRC"),FCrc::MemCrc32(LegacyBytes.GetData()+16,LegacyBytes.Num()-16),StoredCRC);
        TArray<uint8> Payload;Payload.Append(LegacyBytes.GetData()+16,LegacyBytes.Num()-16);
        if (auto* Raw=Cast<UICSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload)))
        {
            AddInfo(FString::Printf(TEXT("Raw fixture version %d, game minute %.1f, bag %d"),Raw->SaveVersion,Raw->Clock.GameMinute,Raw->WorldState.Inventory.Bag.Slots.Num()));
            const bool Migrated=Raw->UpgradeFromVersion2();FString Reason;UICWorldStateSubsystem::ValidateStateWithReason(Raw->WorldState,Reason);
            AddInfo(FString::Printf(TEXT("Raw migration %d, reason %s"),Migrated,*Reason));
            for(const auto& Pair:Raw->WorldState.Inventory.Storage)AddInfo(FString::Printf(TEXT("Raw storage %s count %d"),*Pair.Key.ToString(),Pair.Value.Slots.Num()));
        }
    }
    if(TestNotNull(TEXT("Missing new fields in actual v2 payload migrate"),Real))TestEqual(TEXT("Actual old file upgrades"),Real->SaveVersion,3);
    auto* Save=NewObject<UICSaveGame>();Save->SaveVersion=2;Save->Clock.PlaySecond=333;
    FICCharacterState NPC;NPC.CharacterId=TEXT("NPC_R08_001");NPC.Mode=EICCharacterMode::Downed;NPC.PartySlot=0;
    FICDebuffState Debuff;Debuff.ID=TEXT("EFF_POISON");Debuff.RemainingSeconds=20;NPC.Debuffs.Add(Debuff);Save->WorldState.Characters.Add(NPC.CharacterId,NPC);
    FICQuestState Quest;Quest.bAccepted=true;Quest.bCompleted=true;Save->WorldState.Quests.Add(TEXT("QST_TEST"),Quest);
    TestTrue(TEXT("Old semantic migration"),Save->UpgradeFromVersion2());
    const auto& C=Save->WorldState.Characters.FindChecked(NPC.CharacterId);
    TestTrue(TEXT("Downed separated from companion mode"),C.IsDowned()&&C.Mode==EICCharacterMode::Companion&&C.HP==0);
    TestTrue(TEXT("Old debuff becomes saved effect"),C.Debuffs.IsEmpty()&&C.Combat.Effects.Num()==1);
    TestEqual(TEXT("Remaining effect duration retained"),C.Combat.Effects[0].ExpiresAtPlaySecond,353.0);
    TestTrue(TEXT("Old completed reward cannot be paid again"),Save->WorldState.Quests.FindChecked(TEXT("QST_TEST")).bRewardClaimed);
    TestEqual(TEXT("Migration does not grant gold"),Save->WorldState.Gold,200);
    TArray<uint8> Bytes;TestTrue(TEXT("Migrated save can be rewritten"),UICSaveGame::EncodeForFile(Save,Bytes));
    if(Bytes.Num()>=8){uint32 Future=4;FMemory::Memcpy(Bytes.GetData()+4,&Future,sizeof(Future));TestTrue(TEXT("Future format protected"),UICSaveGame::IsFutureFile(Bytes));TestNull(TEXT("Future format rejected"),UICSaveGame::DecodeFile(Bytes));}
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSchemaInvariantCheck,"IC.WorldState.SchemaInvariants",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSchemaInvariantCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    FICWorldState S=PlayerWorld();TestTrue(TEXT("Initial character/resources unchanged"),UICWorldStateSubsystem::ValidateState(S));
    TestEqual(TEXT("Storage capacity forty"),S.Inventory.Storage.FindChecked(TEXT("PlayerHome")).Slots.Num(),40);
    TestTrue(TEXT("R08 NPC ID allowed"),UICWorldStateSubsystem::IsCharacterIdValid(TEXT("NPC_R08_001")));
    TestFalse(TEXT("R09 NPC ID rejected"),UICWorldStateSubsystem::IsCharacterIdValid(TEXT("NPC_R09_001")));
    auto Bad=S;Bad.Characters.FindChecked(UICWorldStateSubsystem::PlayerId()).SPDrainElapsedPlaySeconds=10;
    TestFalse(TEXT("Invalid SP partial interval rejected"),UICWorldStateSubsystem::ValidateState(Bad));
    Bad=S;Bad.CompletedTransactionIDs.Add(FGuid());TestFalse(TEXT("Invalid receipt rejected"),UICWorldStateSubsystem::ValidateState(Bad));
    FICQuestState Q;Q.bAccepted=true;Q.bCompleted=true;Q.bRewardClaimed=true;Bad=S;Bad.Quests.Add(TEXT("QST_TEST"),Q);
    TestFalse(TEXT("Paid reward without receipt rejected"),UICWorldStateSubsystem::ValidateState(Bad));
    Bad=S;Bad.Characters.FindChecked(UICWorldStateSubsystem::PlayerId()).LastSimPlaySecond=1;
    TestFalse(TEXT("Capture cannot come from future clock"),UICWorldStateSubsystem::ValidateStateClock(Bad,FICClockState()));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICProductionSaveCheck,"IC.WorldState.ProductionSaveService",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICProductionSaveCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();
    auto* GI=NewObject<UGameInstance>();
    auto* WorldState=NewObject<UICWorldStateSubsystem>(GI);auto* Time=NewObject<UICTimeSubsystem>(GI);auto* Saves=NewObject<UICSaveSubsystem>(GI);
    auto S=PlayerWorld();S.Characters.FindChecked(UICWorldStateSubsystem::PlayerId()).SPDrainElapsedPlaySeconds=7;
    FICProductionJobState Job;Job.JobID=FGuid::NewGuid();Job.OwnerCharacterID=UICWorldStateSubsystem::PlayerId();Job.ProductionID=TEXT("PRD_TEST");Job.WorkAnchorID=TEXT("ANC_WORK");Job.StartedAtPlaySecond=300;Job.CompletesAtPlaySecond=360;S.ProductionJobs.Add(Job.JobID,Job);
    FICClockState Clock;Clock.GameMinute=900;Clock.PlaySecond=333;
    TestTrue(TEXT("Initialize production state"),WorldState->RestoreState(S));Time->RestoreClock(Clock);
    const FString Prefix=TEXT("IC_Test_WorldState_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TestTrue(TEXT("Isolated save services"),Saves->ConfigureServices(WorldState,Time,Prefix));Saves->bCaptureSaveThumbnail=false;
    TestTrue(TEXT("Production does not block manual save"),Saves->CanSave(false));
    TestTrue(TEXT("Production saved through actual save service"),Saves->SaveSlotSynchronously(1));
    WorldState->SetPlayerGold(10);Clock.PlaySecond=500;Time->RestoreClock(Clock);
    TestTrue(TEXT("Production loaded through actual save service"),Saves->LoadSlot(1));
    TestEqual(TEXT("Saved PlayClock restored, without wall time"),Time->GetClock().PlaySecond,333.0);
    TestEqual(TEXT("Production remaining time restored"),WorldState->GetProductionRemainingSeconds(Job.JobID,Time->GetClock().PlaySecond),27.0);
    TestEqual(TEXT("Player's partial SP interval restored"),WorldState->GetSnapshot(UICWorldStateSubsystem::PlayerId()).SPDrainElapsedPlaySeconds,7.0);
    TestEqual(TEXT("Gold restored"),WorldState->ExportState().Gold,200);
    TestTrue(TEXT("Delete only isolated test slot"),Saves->DeleteSlot(1));
    FICSaveSafety Safety;Safety.bInCombat=true;Saves->SetSafetyState(Safety);
    TestFalse(TEXT("Production does not bypass combat save restriction"),Saves->CanSave(false));
    Saves->SetSafetyState(FICSaveSafety());S=WorldState->ExportState();
    FICActiveEffectState Effect;Effect.InstanceID=FGuid::NewGuid();Effect.EffectID=TEXT("EFF_TEST");Effect.bHarmful=true;Effect.ExpiresAtPlaySecond=400;
    S.Characters.FindChecked(UICWorldStateSubsystem::PlayerId()).Combat.Effects.Add(Effect);WorldState->RestoreState(S);
    TestFalse(TEXT("Active harmful effect defers autosave"),Saves->CanSave(true));
    S.Characters.FindChecked(UICWorldStateSubsystem::PlayerId()).Combat.Effects[0].ExpiresAtPlaySecond=330;WorldState->RestoreState(S);
    TestTrue(TEXT("Expired effect no longer defers autosave"),Saves->CanSave(true));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveRecoveryCheck,"IC.WorldState.BackupAndVersion1",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSaveRecoveryCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();auto* GI=NewObject<UGameInstance>();auto* State=NewObject<UICWorldStateSubsystem>(GI);auto* Time=NewObject<UICTimeSubsystem>(GI);auto* Saves=NewObject<UICSaveSubsystem>(GI);
    const FString Prefix=TEXT("IC_Test_Recovery_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    State->RestoreState(PlayerWorld());Saves->ConfigureServices(State,Time,Prefix);Saves->bCaptureSaveThumbnail=false;
    FICClockState Clock;Clock.PlaySecond=10;Time->RestoreClock(Clock);TestTrue(TEXT("First auto save"),Saves->SaveSlotSynchronously(0));const auto First=Saves->GetSlotList()[0];
    Clock.GameMinute=900;Clock.PlaySecond=40;Time->RestoreClock(Clock);State->SetPlayerGold(321);TestTrue(TEXT("Second auto save rotates old backup"),Saves->SaveSlotSynchronously(0));
    TArray<uint8> Bytes;TestTrue(TEXT("Read only isolated auto file"),FFileHelper::LoadFileToArray(Bytes,*Saves->GetSlotPath(0)));
    if(!Bytes.IsEmpty()){Bytes.Last()^=1;TestTrue(TEXT("Corrupt only isolated test file"),FFileHelper::SaveArrayToFile(Bytes,*Saves->GetSlotPath(0)));}
    const auto Recovery=Saves->GetSlotList()[0];TestTrue(TEXT("Corruption offers validated backup"),Recovery.bValid&&Recovery.bUsesBackup);TestEqual(TEXT("Preview date comes from backup"),Recovery.SavedAt,First.SavedAt);TestEqual(TEXT("Preview game clock comes from backup"),Recovery.Clock.GameMinute,480.0);
    TestTrue(TEXT("Backup load succeeds"),Saves->LoadSlot(0));TestEqual(TEXT("Backup restores game clock"),Time->GetClock().GameMinute,480.0);TestEqual(TEXT("Backup restores play clock"),Time->GetClock().PlaySecond,10.0);TestEqual(TEXT("Backup restores corresponding state"),State->ExportState().Gold,200);
    auto* Legacy=NewObject<UPlayerPrototypeSave>();Legacy->Version=1;Legacy->PlayerName=TEXT("기존 저장 검증");Legacy->FirstSkill=TEXT("SKL_TEST");Legacy->Bag=PlayerWorld().Inventory.Bag;Legacy->Storage=PlayerWorld().Inventory.Storage;Legacy->QuickSlots=PlayerWorld().Inventory.QuickSlots;
    Legacy->Equipment=PlayerWorld().Inventory.Equipment;
    TestTrue(TEXT("Isolated v1 legacy fixture saved"),UGameplayStatics::SaveGameToSlot(Legacy,Prefix+TEXT("_LegacyV1"),0));TestTrue(TEXT("V1 imports through public service"),Saves->ImportLegacySave());
    TestEqual(TEXT("V1 has no old clock and starts at eight AM"),Time->GetClock().GameMinute,480.0);TestEqual(TEXT("V1 play clock starts at zero"),Time->GetClock().PlaySecond,0.0);TestTrue(TEXT("Legacy original remains available"),Saves->HasLegacySave());TestTrue(TEXT("Converted v1 is a valid normal slot"),Saves->GetSlotList()[1].bValid);
    Saves->DeleteSlot(0);Saves->DeleteSlot(1);Saves->DeleteLegacySave();return !HasAnyErrors();
}
namespace
{
class FICAwaitAsyncSlot final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    TStrongObjectPtr<UICSaveSubsystem> Saves;
    TStrongObjectPtr<UICWorldStateSubsystem> State;
    TStrongObjectPtr<UICTimeSubsystem> Time;
    double Deadline;
public:
    FICAwaitAsyncSlot(FAutomationTestBase* InTest,UICSaveSubsystem* InSaves,UICWorldStateSubsystem* InState,UICTimeSubsystem* InTime)
        :Test(InTest),Saves(InSaves),State(InState),Time(InTime),Deadline(FPlatformTime::Seconds()+15){}
    virtual bool Update() override
    {
        IC_TRACE_FUNCTION();
        if(Saves->bSaving)
        {
            if(FPlatformTime::Seconds()<Deadline)return false;
            Test->AddError(TEXT("Isolated asynchronous save did not finish in fifteen seconds."));return true;
        }
        Test->TestTrue(TEXT("Async write produced a validated slot"),Saves->GetSlotList()[1].bValid);
        Test->TestTrue(TEXT("Async snapshot loads"),Saves->LoadSlot(1));
        Test->TestEqual(TEXT("Async file uses captured clock rather than later clock"),Time->GetClock().PlaySecond,333.0);
        Test->TestEqual(TEXT("Async file uses captured state rather than later mutation"),State->ExportState().Gold,200);
        Saves->DeleteSlot(1);return true;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICAsyncSlotCheck,"IC.WorldState.AsyncSlotSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICAsyncSlotCheck::RunTest(const FString& Parameters)
{
    IC_TRACE_FUNCTION();auto* GI=NewObject<UGameInstance>();auto* State=NewObject<UICWorldStateSubsystem>(GI);auto* Time=NewObject<UICTimeSubsystem>(GI);auto* Saves=NewObject<UICSaveSubsystem>(GI);
    State->RestoreState(PlayerWorld());FICClockState Clock;Clock.PlaySecond=333;Time->RestoreClock(Clock);
    Saves->ConfigureServices(State,Time,TEXT("IC_Test_Async_")+FGuid::NewGuid().ToString(EGuidFormats::Digits));Saves->bCaptureSaveThumbnail=false;
    if(!TestTrue(TEXT("Async request accepted"),Saves->SaveSlot(1)))return false;
    TestTrue(TEXT("Write remains pending until game-thread completion"),Saves->bSaving);
    Clock.PlaySecond=1000;Time->RestoreClock(Clock);State->SetPlayerGold(123);
    ADD_LATENT_AUTOMATION_COMMAND(FICAwaitAsyncSlot(this,Saves,State,Time));return true;
}
#endif
