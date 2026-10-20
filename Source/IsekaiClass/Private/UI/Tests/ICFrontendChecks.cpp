#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/Flow/ICGameFlowSubsystem.h"
#include "Save/ICSaveGame.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCreationAnswerCheck, "IC.Frontend.AnswersAndSave",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICCreationAnswerCheck::RunTest(const FString& Parameters)
{
    FICCreationAnswers Answers;
    TestFalse(TEXT("Unanswered questionnaire rejected"), Answers.IsComplete());
    Answers.Name = TEXT("새로운 여행자");
    Answers.Gender = 1;
    Answers.Story = 2;
    Answers.Music = 3;
    Answers.Happiness = 1;
    TestTrue(TEXT("All five answers accepted"), Answers.IsComplete());
    for (const FString Invalid : {FString(TEXT("   ")), FString(TEXT("줄\n바꿈")), FString(TEXT("1234567890123"))})
        TestFalse(TEXT("Invalid player name rejected"), FICCreationAnswers::IsNameValid(Invalid));
    Answers.Happiness = 4;
    TestFalse(TEXT("Out of range answer rejected"), Answers.IsComplete());
    Answers.Happiness = 1;
    auto* Save = NewObject<UICSaveGame>();
    Save->WorldState.CreationAnswers = Answers;
    TArray<uint8> Bytes;
    TestTrue(TEXT("Actual save encoding"), UICSaveGame::EncodeForFile(Save, Bytes));
    auto* Restored = UICSaveGame::DecodeFile(Bytes);
    if (!TestNotNull(TEXT("Actual save decoding"), Restored))
        return false;
    const auto& Value = Restored->WorldState.CreationAnswers;
    TestEqual(TEXT("Name retained"), Value.Name, Answers.Name);
    TestEqual(TEXT("Gender retained"), Value.Gender, 1);
    TestEqual(TEXT("Story retained"), Value.Story, 2);
    TestEqual(TEXT("Music retained"), Value.Music, 3);
    TestEqual(TEXT("Happiness retained"), Value.Happiness, 1);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICCreationRewardCheck, "IC.Frontend.RewardData",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICCreationRewardCheck::RunTest(const FString& Parameters)
{
    auto* GI = NewObject<UGameInstance>();
    auto* Flow = NewObject<UICGameFlowSubsystem>(GI);
    auto* Definitions = NewObject<UICDefinitionSubsystem>(GI);
    Definitions->ConfigureCatalog(GetDefault<UICDefinitionSettings>()->Catalog.LoadSynchronous());
    const EICWeaponFamily Families[] = {EICWeaponFamily::Sword, EICWeaponFamily::Bow, EICWeaponFamily::Staff,
                                        EICWeaponFamily::Wand};
    for (int32 I = 0; I < 4; ++I)
    {
        const auto* Row = Flow->FindOption(3, I);
        if (!TestNotNull(TEXT("Music choice exists"), Row))
            continue;
        TestFalse(TEXT("Weapon configured"), Row->WeaponItemID.IsNone());
        TestFalse(TEXT("Skill configured"), Row->SkillID.IsNone());
        FICDefinitionResult Result;
        const auto* Skill = Definitions->Find<FICSkillRow>(Row->SkillID, Result);
        const auto* Item = Definitions->Find<FICItemRow>(Row->WeaponItemID, Result);
        const auto* Equipment = Item ? Definitions->Find<FICEquipmentRow>(Item->EquipmentID, Result) : nullptr;
        if (TestNotNull(TEXT("Starting skill resolves in actual catalog"), Skill))
        {
            TestEqual(TEXT("Skill family matches choice"), Skill->RequiredWeaponFamily, Families[I]);
            TestFalse(TEXT("Starting skill has an effect"), Skill->Hits.IsEmpty());
        }
        if (TestNotNull(TEXT("Starting weapon resolves in actual catalog"), Equipment))
            TestEqual(TEXT("Weapon family matches choice"), Equipment->WeaponFamily, Families[I]);
        TestTrue(TEXT("Temporary NPC reward explicitly marked"), Row->bTestReward || !Row->NPCID.IsNone());
        TestFalse(TEXT("Visible label present"), Row->Label.IsEmpty());
        const auto* Happy = Flow->FindOption(4, I);
        if (TestNotNull(TEXT("Happiness choice exists"), Happy))
            TestTrue(TEXT("Happiness has no gameplay reward"), Happy->NPCID.IsNone() && Happy->WeaponItemID.IsNone() &&
                                                                   Happy->SkillID.IsNone() &&
                                                                   Happy->RecipeID.IsNone() && Happy->CodexID.IsNone());
    }
    return !HasAnyErrors();
}

#if WITH_EDITOR
#include "Editor.h"
#include "FileHelpers.h"
#include "Tests/AutomationEditorCommon.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/MainMenu/ICMainMenuWidget.h"
#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "UI/Framework/ICConfirmPopupWidget.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

namespace
{
class FICPlaythroughCommand : public IAutomationLatentCommand
{
public:
    explicit FICPlaythroughCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds())
    {
    }
    virtual bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 75)
        {
            Test->AddError(TEXT("Title-to-play timed out"));
            Cleanup();
            return true;
        }
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!World)
            return false;
        auto* PC = Cast<AICPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
        if (!PC)
            return false;
        auto* Flow = PC->GetGameInstance()->GetSubsystem<UICGameFlowSubsystem>();
        if (Stage == 0)
        {
            if (!PC->IsMainMenuLevel())
                return false;
            Test->TestNull(TEXT("Title does not spawn a pawn"), PC->GetPawn());
            GI = PC->GetGameInstance();
            auto* Saves = PC->GetSaves();
            const FString Prefix = TEXT("IC_Frontend_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
            Test->TestTrue(TEXT("Save files isolated from player saves"),
                           Saves->ConfigureServices(PC->GetWorldState(), PC->GetTime(), Prefix));
            Saves->bCaptureSaveThumbnail = false;
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICMainMenuWidget::StaticClass(), false);
            UICMainMenuWidget* Menu = nullptr;
            for (auto* Widget : Widgets)
                if (auto* Candidate = Cast<UICMainMenuWidget>(Widget))
                    if (Candidate->IsActivated())
                        Menu = Candidate;
            if (!Menu)
                return false;
            auto* Continue = Cast<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Continue")));
            Test->TestTrue(TEXT("Continue disabled for this milestone"), Continue && !Continue->GetIsEnabled());
            auto* Start = Cast<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Start")));
            if (!Start)
            {
                Test->AddError(TEXT("Start button missing"));
                Cleanup();
                return true;
            }
            Start->OnClicked.Broadcast();
            auto* Wizard = PC->CharacterCreateScreen.Get();
            if (!Wizard)
            {
                Test->AddError(TEXT("Start did not show questionnaire"));
                Cleanup();
                return true;
            }
            auto* Name = Cast<UEditableTextBox>(Wizard->GetWidgetFromName(TEXT("EDIT_Name")));
            Name->SetText(FText::FromString(TEXT("테스트여행자")));
            Wizard->Next();
            Wizard->SelectAnswer(1);
            Wizard->SelectAnswer(2);
            Wizard->Back();
            Test->TestEqual(TEXT("Back preserves selected story"), Flow->Answers.Story, 2);
            Wizard->SelectAnswer(2);
            Wizard->SelectAnswer(2);
            Wizard->SelectAnswer(3);
            Test->TestEqual(TEXT("All questions reach review"), Wizard->GetStep(), 5);
            Wizard->Next();
            Widgets.Reset();
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICConfirmPopupWidget::StaticClass(), false);
            for (auto* Widget : Widgets)
                if (auto* Popup = Cast<UICConfirmPopupWidget>(Widget))
                    if (Popup->IsActivated())
                    {
                        CastChecked<UButton>(Popup->GetWidgetFromName(TEXT("BTN_Accept")))->OnClicked.Broadcast();
                        Stage = 1;
                        return false;
                    }
            Test->AddError(TEXT("Final confirmation missing"));
            Cleanup();
            return true;
        }
        if (Stage == 1)
        {
            if (Flow->Phase == EICGameFlowPhase::Failed)
            {
                Test->AddError(Flow->LastError);
                Cleanup();
                return true;
            }
            if (Flow->Phase != EICGameFlowPhase::Playing || !PC->bCreated)
                return false;
            Test->TestFalse(TEXT("Title replaced by gameplay map"), PC->IsMainMenuLevel());
            Test->TestTrue(TEXT("Gameplay uses L_TestMap"), World->GetName().Contains(TEXT("L_TestMap")));
            Test->TestTrue(TEXT("Playable pawn possessed"), PC->GetPawn() != nullptr);
            Test->TestTrue(TEXT("Gameplay input unlocked"), PC->Menu.IsEmpty() && !World->IsPaused());
            const auto Snapshot = PC->GetWorldState()->ExportState();
            Test->TestEqual(TEXT("Answers survive map travel"), Snapshot.CreationAnswers.Music, 2);
            Test->TestEqual(TEXT("Gender follows answer"), PC->Preset, 1);
            Test->TestEqual(TEXT("No NPC fabricated for unconfigured choice"), Snapshot.CreationAnswers.Story, 2);
            Test->TestEqual(TEXT("Staff granted by music choice"), PC->GetInventory()->GetEquippedWeaponFamily(),
                            EICWeaponFamily::Staff);
            Test->TestTrue(TEXT("Initial auto save completed"),
                           PC->GetSaves()->GetSlotList()[0].bValid && !PC->GetSaves()->bSaving);
            Test->TestNotNull(TEXT("HUD mounted"), PC->Screen.Get());
            PC->SetMenu(TEXT("Pause"));
            PausedAt = PC->GetTime()->GetClock().PlaySecond;
            StageAt = FPlatformTime::Seconds();
            Stage = 2;
            return false;
        }
        if (Stage == 2)
        {
            if (FPlatformTime::Seconds() - StageAt < .25)
                return false;
            Test->TestEqual(TEXT("Pause freezes PlayClock"), PC->GetTime()->GetClock().PlaySecond, PausedAt);
            PC->SetMenu(TEXT(""));
            StartPosition = PC->GetPawn()->GetActorLocation();
            StageAt = FPlatformTime::Seconds();
            Stage = 3;
            return false;
        }
        if (Stage == 3)
        {
            PC->GetPawn()->AddMovementInput(FVector::ForwardVector, 1.f);
            if (FPlatformTime::Seconds() - StageAt < .35)
                return false;
            Test->TestTrue(TEXT("Pawn actually moves after first save"),
                           FVector::Dist2D(StartPosition, PC->GetPawn()->GetActorLocation()) > 1.f);
            Test->TestFalse(TEXT("Cursor returns to gameplay"), PC->bShowMouseCursor);
            Test->TestTrue(TEXT("Initial save loads through save service"), PC->GetSaves()->LoadSlot(0));
            Test->TestEqual(TEXT("Questionnaire retained on load"),
                            PC->GetWorldState()->ExportState().CreationAnswers.Happiness, 3);
            Cleanup();
            return true;
        }
        return false;
    }

private:
    FAutomationTestBase* Test;
    TWeakObjectPtr<UGameInstance> GI;
    int32 Stage = 0;
    double Started = 0, StageAt = 0, PausedAt = 0;
    FVector StartPosition = FVector::ZeroVector;
    void Cleanup()
    {
        if (GI.IsValid())
            if (auto* Saves = GI->GetSubsystem<UICSaveSubsystem>())
                if (!Saves->bSaving)
                    Saves->DeleteSlot(0);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFrontendPlaythroughCheck, "IC.Frontend.TitleToPlay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICFrontendPlaythroughCheck::RunTest(const FString& Parameters)
{
    if (GEditor->PlayWorld)
    {
        AddError(TEXT("Close the existing PIE session before running this test."));
        return false;
    }
    if (!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/Maps/L_Title"), false, true))
    {
        AddError(TEXT("Could not open title map"));
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICPlaythroughCommand>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
    return true;
}

namespace
{
class FICQuitCommand : public IAutomationLatentCommand
{
public:
    explicit FICQuitCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds())
    {
    }
    virtual bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 20)
        {
            Test->AddError(TEXT("Main menu quit timed out"));
            return true;
        }
        if (Clicked)
            return GEditor->PlayWorld == nullptr;
        UWorld* World = GEditor->PlayWorld;
        if (!World)
            return false;
        TArray<UUserWidget*> Widgets;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICMainMenuWidget::StaticClass(), false);
        for (auto* Widget : Widgets)
            if (auto* Menu = Cast<UICMainMenuWidget>(Widget))
                if (Menu->IsActivated())
                {
                    auto* Button = Cast<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Quit")));
                    if (!Button)
                    {
                        Test->AddError(TEXT("Main menu quit button missing"));
                        return true;
                    }
                    Clicked = true;
                    Button->OnClicked.Broadcast();
                    return false;
                }
        return false;
    }

private:
    FAutomationTestBase* Test;
    double Started;
    bool Clicked = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFrontendQuitCheck, "IC.Frontend.TitleQuit",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FICFrontendQuitCheck::RunTest(const FString& Parameters)
{
    if (!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/Maps/L_Title"), false, true))
    {
        AddError(TEXT("Could not open title map"));
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICQuitCommand>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
    return true;
}
#endif
#endif
