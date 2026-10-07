#include "Misc/AutomationTest.h"
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "PlayerPrototypeController.h"
#include "PlayerPrototypeWidget.h"
#include "PlayerInventory.h"
#include "ICCharacterCreateWidget.h"
#include "../Core/ICSaveSubsystem.h"
#include "../Core/ICSaveGame.h"
#include "../Core/ICWorldStateSubsystem.h"
#include "../Core/ICTimeSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationEditorCommon.h"

class FVerifyCharacterCreationFlow : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    int32 Step = 0;
    double Started = FPlatformTime::Seconds();
    FString Prefix = TEXT("IC_Test_Creation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TArray<FString> Paths;
    TArray<uint8> OriginalFile;
    FGuid WeaponInstance;
    void CleanupFiles()
    {
        for (const FString& Path : Paths) IFileManager::Get().Delete(*Path, false, false, true);
    }
public:
    explicit FVerifyCharacterCreationFlow(FAutomationTestBase* InTest) : Test(InTest) {}
    virtual bool Update() override
    {
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        if (Step == 3 && !World) { CleanupFiles(); return true; }
        if (FPlatformTime::Seconds() - Started > 40.0)
        {
            Test->AddError(TEXT("Character creation flow timed out."));
            if (GEditor) GEditor->RequestEndPlayMap();
            CleanupFiles(); return true;
        }
        auto* PC = World ? Cast<APlayerPrototypeController>(World->GetFirstPlayerController()) : nullptr;
        if (!PC || !PC->Screen) return false;
        auto* Saves = PC->GetSaves();
        auto* State = PC->GetWorldState();
        auto* Time = PC->GetTime();
        auto* Inventory = PC->GetInventory();
        if (!Saves || !State || !Time || !Inventory) return false;
        if (Step == 0)
        {
            if (!Test->TestTrue(TEXT("Use isolated test save prefix"), Saves->ConfigureServices(State, Time, Prefix)))
            { GEditor->RequestEndPlayMap(); Step = 3; return false; }
            for (int32 SlotIndex = 0; SlotIndex <= 4; ++SlotIndex)
                for (const FString& Suffix : TArray<FString>{TEXT(""),TEXT(".tmp"),TEXT(".bak"),TEXT(".rollback"),TEXT(".bak.tmp")})
                    Paths.Add(Saves->GetSlotPath(SlotIndex) + Suffix);
            PC->SetMenu(TEXT("Create"));
            auto* Future = NewObject<UICSaveGame>();
            Future->SaveVersion = 999; Future->Slot = 0;
            if (!Test->TestTrue(TEXT("Write protected future-version test save"),
                UICSaveGame::EncodeForFile(Future, OriginalFile) && FFileHelper::SaveArrayToFile(OriginalFile, *Saves->GetSlotPath(0))))
            { GEditor->RequestEndPlayMap(); Step = 3; return false; }

            PC->PlayerName = TEXT(" ");
            PC->RequestCreatePlayer();
            Test->TestEqual(TEXT("Invalid name stays on creation, before overwrite confirmation"), PC->Menu, FString(TEXT("Create")));
            Test->TestFalse(TEXT("Invalid name does not initialize world state"), State->ExportState().bPlayerCreated);
            Test->TestFalse(TEXT("Invalid name does not write save"), Saves->bSaving);

            PC->PlayerName = TEXT("생성테스트");
            PC->RequestCreatePlayer();
            Test->TestEqual(TEXT("Existing auto-save requests confirmation"), PC->Menu, FString(TEXT("Confirm")));
            PC->ConfirmModal(false);
            Test->TestTrue(TEXT("Cancel leaves form editable"), PC->CanEditCreation());
            PC->RequestCreatePlayer(); PC->ConfirmModal(true);
            Test->TestFalse(TEXT("Protected future save does not start gameplay"), PC->bCreated);
            Test->TestEqual(TEXT("Failed initial save returns to creation"), PC->Menu, FString(TEXT("Create")));
            Test->TestTrue(TEXT("Failed initial save can be retried"), PC->CanSubmitCreation());
            Test->TestFalse(TEXT("Pending validated creation remains read-only"), PC->CanEditCreation());
            Test->TestFalse(TEXT("Play clock remains stopped on failure"), Time->IsRunning());
            Test->TestFalse(TEXT("Failure has a notice"), PC->Notice.IsEmpty());
            TArray<uint8> Preserved;
            Test->TestTrue(TEXT("Failed creation preserves previous save bytes"), FFileHelper::LoadFileToArray(Preserved, *Saves->GetSlotPath(0)) && Preserved == OriginalFile);
            WeaponInstance = Inventory->GetSnapshot().Equipment[0].InstanceID;
            // Remove only this isolated test file, then retry the already prepared character.
            Test->TestTrue(TEXT("Remove isolated blocking test save"), Saves->DeleteSlot(0));
            PC->RequestCreatePlayer();
            Test->TestFalse(TEXT("Cannot submit twice while saving"), PC->CanSubmitCreation());
            Step = 1; return false;
        }
        if (Step == 1)
        {
            if (Saves->bSaving) return false;
            Test->TestTrue(TEXT("Retry completes player creation"), PC->bCreated);
            Test->TestTrue(TEXT("Successful creation enters gameplay"), PC->Menu.IsEmpty());
            Test->TestFalse(TEXT("Gameplay is unpaused"), PC->IsPaused());
            Test->TestFalse(TEXT("Gameplay hides mouse cursor"), PC->bShowMouseCursor);
            Test->TestTrue(TEXT("Play clock starts after save success"), Time->IsRunning());
            Test->TestTrue(TEXT("Auto-save is valid after success"), Saves->GetSlotList()[0].bValid);
            PC->Gold=321;PC->ConsumeProductionSP(10.f);PC->MainStats.Dexterity=7;const float SavedSP=PC->Survival.SP;
            Test->TestTrue(TEXT("New player stats save through shared service"),Saves->SaveSlotSynchronously(1));
            PC->Gold=999;PC->ConsumeProductionSP(10.f);PC->MainStats.Dexterity=1;
            PC->SelectedLoadSlot=1;Test->TestTrue(TEXT("Shared save reloads new player stats"),PC->LoadPlayer());
            Test->TestEqual(TEXT("Gold survives reload"),PC->Gold,321);
            Test->TestEqual(TEXT("Dexterity survives reload"),PC->MainStats.Dexterity,7);
            Test->TestEqual(TEXT("SP survives reload"),PC->Survival.SP,SavedSP);
            Test->TestEqual(TEXT("Retry does not duplicate/recreate starting weapon"), Inventory->GetSnapshot().Equipment[0].InstanceID, WeaponInstance);
            TArray<UWidget*> Widgets; PC->Screen->WidgetTree->GetAllWidgets(Widgets);
            bool bCreationFormAttached = false;
            for (UWidget* Widget : Widgets) bCreationFormAttached |= Widget && Widget->GetClass()->IsChildOf(UICCharacterCreateWidget::StaticClass());
            Test->TestFalse(TEXT("Creation form removed from visible widget tree"), bCreationFormAttached);
            // Exercise accepted overwrite with an actual valid save, not just the retry path.
            Test->TestTrue(TEXT("Clear live state for a second new character"), State->RestoreState(FICWorldState()));
            PC->PlayerName = TEXT("두번째생성"); PC->Job = 1;
            PC->RequestCreatePlayer();
            Test->TestEqual(TEXT("Valid existing save requests overwrite"), PC->Menu, FString(TEXT("Confirm")));
            PC->ConfirmModal(true); Step = 2; return false;
        }
        if (Step == 2)
        {
            if (Saves->bSaving) return false;
            Test->TestTrue(TEXT("Accepted overwrite starts gameplay"), PC->bCreated && PC->Menu.IsEmpty() && !PC->IsPaused());
            Test->TestEqual(TEXT("New name survives confirmation"), PC->PlayerName, FString(TEXT("두번째생성")));
            Test->TestEqual(TEXT("New archer weapon is equipped"), Inventory->GetSnapshot().Equipment[0].ItemID, FName(TEXT("EQP_WPN_BOW_001")));
            for (int32 SlotIndex = 0; SlotIndex <= 4; ++SlotIndex) Saves->DeleteSlot(SlotIndex);
            GEditor->RequestEndPlayMap(); Step = 3; return false;
        }
        return false;
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerCreationFlowTest, "Isekai.Player.CreationOverwriteAndRetry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerCreationFlowTest::RunTest(const FString& Parameters)
{
    FRequestPlaySessionParams Params;
    Params.GlobalMapOverride = TEXT("/Game/PlayerPrototype/L_PlayerPrototype");
    Params.bAllowOnlineSubsystem = false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    ADD_LATENT_AUTOMATION_COMMAND(FVerifyCharacterCreationFlow(this));
    return true;
}
#endif
