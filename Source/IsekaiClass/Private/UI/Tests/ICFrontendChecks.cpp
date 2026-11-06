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
    const FName StartingNPCs[]={TEXT("NPC_R01_002"),TEXT("NPC_R01_004"),TEXT("NPC_R01_001"),TEXT("NPC_R01_003")};
    for (int32 I = 0; I < 4; ++I)
    {
        const auto* Row = Flow->FindOption(3, I);
        if (!TestNotNull(TEXT("Music choice exists"), Row))
            continue;
        TestFalse(TEXT("Weapon configured"), Row->WeaponItemID.IsNone());
        TestFalse(TEXT("Skill configured"), Row->SkillID.IsNone());
        TestEqual(TEXT("Approved music NPC mapping"),Row->NPCID,StartingNPCs[I]);
        FICDefinitionResult Result;
        const auto* Skill = Definitions->Find<FICSkillRow>(Row->SkillID, Result);
        const auto* Item = Definitions->Find<FICItemRow>(Row->WeaponItemID, Result);
        const auto* Equipment = Item ? Definitions->Find<FICEquipmentRow>(Item->EquipmentID, Result) : nullptr;
        if (TestNotNull(TEXT("Starting skill resolves in actual catalog"), Skill))
        {
            TestEqual(TEXT("Skill family matches choice"), Skill->RequiredWeaponFamily, Families[I]);
            TestFalse(TEXT("Starting skill has an effect"), Skill->Hits.IsEmpty());
            TestEqual(TEXT("Starting skill is the chosen NPC unique skill"),Skill->NPCID,Row->NPCID);
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
#include "UI/Framework/ICPrologueWidget.h"
#include "UI/Framework/ICLoadingWidget.h"
#include "Audio/ICBGMSubsystem.h"
#include "Settings/ICGameUserSettings.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Player/Character/ICPlayerAppearance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "Core/Controller/ICPlayerController.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/Framework/ICFrontendLayout.h"
#include "UI/Settings/ICSettingsWidget.h"
#include "UI/Save/ICSaveLoadWidget.h"
#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Tests/ICPlayWindowSettingsRestore.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SceneViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "PlayInEditorDataTypes.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "Save/ICSaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

struct FICPrologueTestAccess
{
    static void Tick(UICPrologueWidget* Widget,float Delta){Widget->NativeTick(Widget->GetCachedGeometry(),Delta);}
    static bool Reading(UICPrologueWidget* W){return W->State==UICPrologueWidget::EState::Reading;}
    static bool Finishing(UICPrologueWidget* W){return W->State==UICPrologueWidget::EState::FadeOut;}
    static bool Revealing(UICPrologueWidget* W){return W->State==UICPrologueWidget::EState::Revealing;}
    static float Held(UICPrologueWidget* W){return W->SkipElapsed;}
};
struct FICKeyboardTestAccess
{
    static void Sync(UICCharacterCreateWidget* Widget){Widget->NativeTick(Widget->GetCachedGeometry(),0.f);}
};
namespace
{
class FICPlaythroughCommand : public IAutomationLatentCommand
{
public:
    explicit FICPlaythroughCommand(FAutomationTestBase* InTest,bool Skip=false) : Test(InTest), bSkipRun(Skip), Started(FPlatformTime::Seconds())
    {
    }
    virtual bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 180)
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
        auto* Client = PC->GetLocalPlayer() ? PC->GetLocalPlayer()->ViewportClient.Get() : nullptr;
        if (!Client || !Client->GetWindow()) return false;
        if(Flow->Phase==EICGameFlowPhase::Initializing&&PC->bCreated&&PC->Menu==TEXT("Loading"))
        {
            if(!bSawCharacterPreparation)
                {bSawCharacterPreparation=true;Capture(Client,bSkipRun?TEXT("Loading_Preparing_Pose_H"):TEXT("Loading_Preparing_Pose_G"));}
        }
        if(Flow->bPendingNewGame&&Flow->Phase==EICGameFlowPhase::Loading&&PC->IsMainMenuLevel()&&PC->Menu==TEXT("Loading"))
        {
            auto* Music = World->GetSubsystem<UICBGMSubsystem>();
            Music->Tick(1.f);
            Test->TestEqual(TEXT("Loading after the story is silent"), Music->GetCurrentMusicRow(), NAME_None);
            TArray<UUserWidget*> Loaders;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Loaders,UICLoadingWidget::StaticClass(),false);
            for(auto* Loader:Loaders)if(Loader->IsVisible()&&Loader->GetCachedGeometry().GetLocalSize().X>1)
            {
                auto* Card=Loader->GetWidgetFromName(TEXT("Card"));auto* Status=Cast<UTextBlock>(Loader->GetWidgetFromName(TEXT("TXT_Status")));
                Test->TestTrue(TEXT("Post-story loading card is visible"),Card&&Card->IsVisible());
                Test->TestTrue(TEXT("Post-story loading has a visible status"),Status&&Status->IsVisible()&&!Status->GetText().IsEmpty());
                if(!bSawStoryLoading)
                {
                    bSawStoryLoading=true;StoryLoadingFirst=FPlatformTime::Seconds();
                    Capture(Client,bSkipRun?TEXT("Loading_After_H"):TEXT("Loading_After_G"));
                }
                StoryLoadingLast=FPlatformTime::Seconds();
                break;
            }
            return false;
        }
        if(Flow->Phase==EICGameFlowPhase::Prologue)
        {
            TArray<UUserWidget*> Prologues;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Prologues,UICPrologueWidget::StaticClass(),false);
            for(auto* Entry:Prologues)if(auto* Prologue=Cast<UICPrologueWidget>(Entry);Prologue&&Prologue->IsActivated())
            {
                if(FICPrologueTestAccess::Revealing(Prologue))
                {
                    auto* Music = World->GetSubsystem<UICBGMSubsystem>();
                    Music->Tick(1.f);
                    Test->TestEqual(TEXT("Revealing the open world does not restart story music"), Music->GetCurrentMusicRow(), NAME_None);
                    auto* Player=Cast<AICPlayerCharacter>(PC->GetPawn());
                    Test->TestTrue(TEXT("World reveal starts with an evaluated arm pose rather than a reference T-pose"),Player&&ICPlayerAppearance::IsFirstPoseReady(Player->GetMesh()));
                    bSawPreparedReveal=true;return false;
                }
                Test->TestTrue(TEXT("Prologue UI keeps the cursor visible"),PC->bShowMouseCursor);
                auto* Music=World->GetSubsystem<UICBGMSubsystem>();Music->Tick(1.f);
                if(FICPrologueTestAccess::Reading(Prologue))Test->TestEqual(TEXT("Prologue reading uses its editable music row"),Music->GetCurrentMusicRow(),FName(TEXT("BGM_Prologue")));
                if (FICPrologueTestAccess::Finishing(Prologue))
                    Test->TestEqual(TEXT("The story fade begins in silence"), Music->GetCurrentMusicRow(), NAME_None);
                const int32 Before=Prologue->GetLineIndex();
                Test->TestEqual(TEXT("The WBP loads all fifty-five DT entries"),Prologue->GetLineCount(),55);
                if(bSkipRun&&Before>=17)
                {
                    if(!FICPrologueTestAccess::Reading(Prologue))return false;
                    if(!bPartialSkip)
                    {
                        FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::H,FModifierKeysState(),0,false,0,0));
                        FICPrologueTestAccess::Tick(Prologue,.6f);
                        Test->TestTrue(TEXT("Holding H fills the skip gauge during typing"),FICPrologueTestAccess::Held(Prologue)>.5f);
                        bPartialSkip=true;return false;
                    }
                    Capture(Client,TEXT("Prologue_HoldSkip"));
                    FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::H,FModifierKeysState(),0,false,0,0));
                    Test->TestEqual(TEXT("Releasing H resets its gauge"),FICPrologueTestAccess::Held(Prologue),0.f);
                    Test->TestEqual(TEXT("A short hold does not skip"),Prologue->GetLineIndex(),Before);
                    FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::H,FModifierKeysState(),0,false,0,0));
                    FICPrologueTestAccess::Tick(Prologue,1.6f);
                    Test->TestTrue(TEXT("A complete H hold fades into the normal new-game path"),FICPrologueTestAccess::Finishing(Prologue));
                    Test->TestEqual(TEXT("Skipping stops story music immediately"), Music->GetCurrentMusicRow(), NAME_None);
                    FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::H,FModifierKeysState(),0,false,0,0));
                    return false;
                }
                if(Before<2)
                {
                    // Exercise real Slate time and deliberately leave focus on the viewport.
                    // The old test forced focus and manually ticked before every G, hiding the bug.
                    if(!Prologue->IsLineComplete())
                    {
                        PressKey(EKeys::G);
                        Test->TestEqual(TEXT("G during typing cannot skip a line"),Prologue->GetLineIndex(),Before);
                        return false;
                    }
                    FSlateApplication::Get().SetUserFocus(0,Client->GetGameViewportWidget(),EFocusCause::SetDirectly);
                }
                else FICPrologueTestAccess::Tick(Prologue,4.f);
                if(Prologue->IsLineComplete())
                {
                    if(Before==0||Before==17||Before==50||Before==54)Capture(Client,FString::Printf(TEXT("Prologue_%02d"),Before+1));
                    FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::G,FModifierKeysState(),0,true,0,0));
                    Test->TestEqual(TEXT("Held G cannot advance a complete line"),Prologue->GetLineIndex(),Before);
                    PressKey(EKeys::G);
                    if(Before<54)Test->TestEqual(TEXT("G advances even when viewport has the keyboard focus"),Prologue->GetLineIndex(),Before+1);
                    else Test->TestEqual(TEXT("Completing the last line stops story music immediately"), Music->GetCurrentMusicRow(), NAME_None);
                }
                return false;
            }
        }
        if (Stage == 0)
        {
            if (!PC->IsMainMenuLevel())
                return false;
            auto* TitleMusic=World->GetSubsystem<UICBGMSubsystem>();TitleMusic->Tick(1.f);
            Test->TestEqual(TEXT("Title uses its editable music row"),TitleMusic->GetCurrentMusicRow(),FName(TEXT("BGM_MainMenu")));
            Test->TestNull(TEXT("Title does not spawn a pawn"), PC->GetPawn());
            auto* Saves = PC->GetSaves();
            const FString Prefix = TEXT("IC_Frontend_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICMainMenuWidget::StaticClass(), false);
            UICMainMenuWidget* Menu = nullptr;
            for (auto* Widget : Widgets)
                if (auto* Candidate = Cast<UICMainMenuWidget>(Widget))
                    if (Candidate->IsActivated())
                        Menu = Candidate;
            if (!Menu)
                return false;
            if (!bSeededSaves)
            {
                if (!bRequestedEmptyContinue)
                {
                    if (!Test->TestTrue(TEXT("Save files isolated from player saves"),
                                        Saves->ConfigureServices(PC->GetWorldState(), PC->GetTime(), Prefix)))
                        return true;
                    GI = PC->GetGameInstance();
                    auto* Continue = Cast<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Continue")));
                    Test->TestTrue(TEXT("Continue stays clickable with no save"), Continue && Continue->GetIsEnabled());
                    if (!Click(Client, Continue)) return false;
                    Test->TestEqual(TEXT("Continue covers the title before checking files"), PC->Menu, FString(TEXT("Loading")));
                    bRequestedEmptyContinue = true;
                    return false;
                }
                Test->TestEqual(TEXT("Empty continue stays on title"), PC->Menu, FString(TEXT("MainMenu")));
                Test->TestEqual(TEXT("Empty continue explains the missing save"),
                                CastChecked<UTextBlock>(Menu->GetWidgetFromName(TEXT("TXT_Subtitle")))->GetText().ToString(),
                                FString(TEXT("이어할 저장 데이터가 없습니다.")));
                for (int32 Slot = 0; Slot < 2; ++Slot)
                {
                    auto* Save = NewObject<UICSaveGame>();
                    Save->Slot = Slot; Save->SavedAt = FDateTime::UtcNow() - FTimespan::FromDays(1);
                    Save->WorldState.bPlayerCreated = true; Save->WorldState.PlayerName = TEXT("이전 여행자");
                    Save->WorldState.FirstSkill = TEXT("SKL_TEST");
                    FICCharacterState Player; Player.CharacterId = UICWorldStateSubsystem::PlayerId();
                    Player.Kind = EICCharacterKind::Player; Save->WorldState.Characters.Add(Player.CharacterId, Player);
                    TArray<uint8> Bytes;
                    Test->TestTrue(TEXT("Encode existing save fixture"), UICSaveGame::EncodeForFile(Save, Bytes));
                    Test->TestTrue(TEXT("Write isolated save fixture"), FFileHelper::SaveArrayToFile(Bytes, *Saves->GetSlotPath(Slot)));
                    if (Slot == 1) ManualBytes = Bytes;
                }
                bSeededSaves = true;
                Test->TestTrue(TEXT("Existing auto save is valid before new game"), Saves->GetSlotList()[0].bValid);
            }
            const TCHAR* Actions[] = {TEXT("BTN_Continue"), TEXT("BTN_Start"), TEXT("BTN_SaveList"),
                                      TEXT("BTN_Settings"), TEXT("BTN_Quit")};
            const TCHAR* Labels[] = {TEXT("이어하기"), TEXT("게임시작"), TEXT("저장목록"),
                                     TEXT("설정"), TEXT("그만하기")};
            float PreviousY = -1.f;
            for (int32 I = 0; I < UE_ARRAY_COUNT(Actions); ++I)
            {
                auto* Button = CastChecked<UButton>(Menu->GetWidgetFromName(Actions[I]));
                Test->TestTrue(TEXT("Every title action is clickable"), Button->GetIsEnabled());
                Test->TestEqual(TEXT("Title action uses requested label"),
                                CastChecked<UTextBlock>(Button->GetContent())->GetText().ToString(), FString(Labels[I]));
                const float Y = Button->GetCachedGeometry().GetAbsolutePosition().Y;
                Test->TestTrue(TEXT("Title actions follow requested order"), Y > PreviousY);
                PreviousY = Y;
            }
            if (!Click(Client, CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_SaveList"))))) return false;
            Test->TestEqual(TEXT("Save list opens from title"), PC->Menu, FString(TEXT("Load")));
            PC->CloseSessionMenu();
            Test->TestEqual(TEXT("Save list close returns to title"), PC->Menu, FString(TEXT("MainMenu")));
            NextStage(8);
            return false;
        }
        if (Stage == 8)
        {
            if (FPlatformTime::Seconds() - StageAt < .3) return false;
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICMainMenuWidget::StaticClass(), false);
            UICMainMenuWidget* Menu = nullptr;
            for (auto* Widget : Widgets)
                if (auto* Candidate = Cast<UICMainMenuWidget>(Widget))
                    if (Candidate->IsActivated()) Menu = Candidate;
            if (!Menu) return false;
            if (!Click(Client, CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Settings"))))) return false;
            Test->TestTrue(TEXT("Title settings opens existing options"), PC->Menu == TEXT("Settings") && PC->SettingsScreen);
            PC->CloseSessionMenu();
            Test->TestEqual(TEXT("Closing title settings returns to title"), PC->Menu, FString(TEXT("MainMenu")));
            NextStage(9);
            return false;
        }
        if (Stage == 9)
        {
            if (FPlatformTime::Seconds() - StageAt < .3) return false;
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICMainMenuWidget::StaticClass(), false);
            UICMainMenuWidget* Menu = nullptr;
            for (auto* Widget : Widgets)
                if (auto* Candidate = Cast<UICMainMenuWidget>(Widget))
                    if (Candidate->IsActivated()) Menu = Candidate;
            if (!Menu) return false;
            auto* Start = Cast<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Start")));
            if (!Start)
            {
                Test->AddError(TEXT("Start button missing"));
                Cleanup();
                return true;
            }
            Start->SetUserFocus(PC);
            PressKey(EKeys::W);
            Test->TestTrue(TEXT("W selects the preceding title action"),Menu->GetWidgetFromName(TEXT("BTN_Continue"))->HasUserFocus(PC));
            PressKey(EKeys::S);
            Test->TestTrue(TEXT("S selects the next title action"),Start->HasUserFocus(PC));
            PressKey(EKeys::F);
            Test->TestEqual(TEXT("G opens character creation from focused title button"),PC->Menu,FString(TEXT("Create")));
            NextStage(10);
            return false;
        }
        if (Stage == 15)
        {
            if (FPlatformTime::Seconds() - StageAt < .3) return false;
            auto* Wizard = PC->CharacterCreateScreen.Get();
            if (!Wizard) return false;
            Capture(Client, TEXT("01_Name"));
            if (!Click(Client, CastChecked<UButton>(Wizard->GetWidgetFromName(TEXT("BTN_Next"))))) return false;
            NextStage(11); return false;
        }
        if (Stage >= 16 && Stage <= 18)
        {
            if (FPlatformTime::Seconds() - StageAt < .3) return false;
            auto* Wizard = PC->CharacterCreateScreen.Get();
            if (!Wizard) return false;
            auto* Keyboard = Wizard->GetWidgetFromName(TEXT("NameKeyboard"));
            auto* Name = CastChecked<UEditableTextBox>(Wizard->GetWidgetFromName(TEXT("EDIT_Name")));
            const FKey Accept = UICGameUserSettings::GamepadFaceKey(3);
            if (Stage == 16)
            {
                Test->TestTrue(TEXT("Controller input opens the name keyboard"), Keyboard && Keyboard->IsVisible());
                const auto ChooseVirtualKey = [&](int32 Index)
                {
                    auto* Key = Wizard->GetWidgetFromName(FName(*FString::Printf(TEXT("NameKey%d"), Index)));
                    Key->SetUserFocus(PC);
                    FICKeyboardTestAccess::Sync(Wizard);
                    PressKey(Accept);
                };
                for (int32 Index : {14, 17, 11})
                    ChooseVirtualKey(Index);
                Test->TestEqual(TEXT("Gamepad keyboard composes Hangul without OS IME"), Flow->Answers.Name, FString(TEXT("한")));
                PressKey(EKeys::Gamepad_LeftShoulder);
                ChooseVirtualKey(0);
                Test->TestEqual(TEXT("English name keys work with the controller"), Flow->Answers.Name, FString(TEXT("한q")));
                NextStage(17);
                return false;
            }
            if (Stage == 17)
            {
                Capture(Client, TEXT("01_Name_Gamepad"));
                PressKey(EKeys::LeftShift);
                NextStage(18);
                return false;
            }
            Test->TestTrue(TEXT("Returning to physical input hides the name keyboard"), Keyboard && !Keyboard->IsVisible());
            Test->TestEqual(TEXT("Changing input devices preserves the name"), Flow->Answers.Name, FString(TEXT("한q")));
            Test->TestTrue(TEXT("Physical input focus returns to the name field"), Name->HasKeyboardFocus() || Name->HasFocusedDescendants());
            Name->SetText(FText::GetEmpty());
            for (TCHAR Character : FString(TEXT("테스트여행자")))
                FSlateApplication::Get().ProcessKeyCharEvent(FCharacterEvent(Character, FModifierKeysState(), 0, false));
            Test->TestEqual(TEXT("Typed name reaches creation state"), Flow->Answers.Name, FString(TEXT("테스트여행자")));
            NextStage(15);
            return false;
        }
        if (Stage >= 10 && Stage <= 14)
        {
            if (FPlatformTime::Seconds() - StageAt < .3) return false;
            auto* Wizard = PC->CharacterCreateScreen.Get();
            if (!Wizard)
            {
                Test->AddError(TEXT("Start did not show questionnaire"));
                Cleanup();
                return true;
            }
            const int32 Question = Stage - 10;
            if (!Test->TestEqual(TEXT("Question advances exactly once"), Wizard->GetStep(), Question))
            { Cleanup(); return true; }
            for (const auto* Name : {TEXT("TXT_Step"), TEXT("TXT_Detail"), TEXT("BTN_Back")})
                Test->TestNull(TEXT("Obsolete progress, hints and back controls removed"), Wizard->GetWidgetFromName(Name));
            auto* Prompt = CastChecked<UTextBlock>(Wizard->GetWidgetFromName(TEXT("TXT_Question")));
            Test->TestTrue(TEXT("Question is a single line"), !Prompt->GetAutoWrapText() && Prompt->GetWrapTextAt() == 0.f);
            Wizard->Back();
            Test->TestEqual(TEXT("Back cannot reverse creation"), Wizard->GetStep(), Question);
            if (Question == 0)
            {
                auto* CreationMusic=World->GetSubsystem<UICBGMSubsystem>();CreationMusic->Tick(1.f);
                Test->TestEqual(TEXT("Creation uses its editable music row"),CreationMusic->GetCurrentMusicRow(),FName(TEXT("BGM_CharacterCreate")));
                auto* Keyboard = Wizard->GetWidgetFromName(TEXT("NameKeyboard"));
                auto* Next = CastChecked<UButton>(Wizard->GetWidgetFromName(TEXT("BTN_Next")));
                Test->TestTrue(TEXT("Keyboard and mouse entry starts with no virtual keyboard"), Keyboard && !Keyboard->IsVisible());
                Test->TestFalse(TEXT("Empty name cannot advance"), Next->GetIsEnabled());
                Test->TestEqual(TEXT("Name button caption"), CastChecked<UTextBlock>(Next->GetContent())->GetText().ToString(), FString(TEXT("이름 설정")));
                PressKey(EKeys::Gamepad_DPad_Down);
                NextStage(16);
                return false;
            }
            else
            {
                const int32 Answers[] = {0, 1, 2, 2, 3};
                const int32 Choice = Answers[Question];
                for (int32 I = 0; I < (Question == 1 ? 2 : 4); ++I)
                {
                    auto* Button = CastChecked<UButton>(Wizard->GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Choice%d"), I))));
                    auto* Caption = CastChecked<UTextBlock>(Button->GetContent());
                    Test->TestTrue(TEXT("Choice caption is a single line"), !Caption->GetAutoWrapText() && Caption->GetWrapTextAt() == 0.f);
                    Test->TestTrue(TEXT("Choice fits inside its button"), Caption->GetDesiredSize().X <= Button->GetCachedGeometry().GetLocalSize().X);
                    if (Question > 1)
                        Test->TestEqual(TEXT("Choice uses current StringTable label"), Caption->GetText().ToString(), Flow->GetOption(Question, I).Label.ToString());
                }
                Capture(Client, FString::Printf(TEXT("%02d_Question"), Question + 1));
                auto* Answer = CastChecked<UButton>(Wizard->GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_Choice%d"), Choice))));
                Answer->SetUserFocus(PC);PressKey(Question%2?EKeys::F:EKeys::G);
            }
            if (Question == 4)
            {
                Test->TestTrue(TEXT("Last answer starts the prologue"), !Flow->bPendingNewGame && Flow->Phase == EICGameFlowPhase::Prologue);
                NextStage(1);
            }
            else NextStage(Stage + 1);
            return false;
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
            auto* Music = World->GetSubsystem<UICBGMSubsystem>();
            Music->Tick(1.f);
            Test->TestEqual(TEXT("World music resumes from its area rules"), Music->GetCurrentMusicRow(),
                            Music->ResolveMusicRow(PC->GetPawn()->GetActorLocation(), PC->bInCombat));
            Test->TestTrue(TEXT("Finishing the story presents a painted loading screen before travel"),bSawStoryLoading);
            Test->TestTrue(TEXT("Post-story loading remains visible long enough to read"),StoryLoadingLast-StoryLoadingFirst>=.8);
            Test->TestTrue(TEXT("Loading cover remains after save while the character is prepared"),bSawCharacterPreparation);
            Test->TestTrue(TEXT("Animation is evaluated before the first world reveal"),bSawPreparedReveal);
            Capture(Client,bSkipRun?TEXT("Startup_Pose_After_H"):TEXT("Startup_Pose_After_G"));
            Test->TestFalse(TEXT("Title replaced by gameplay map"), PC->IsMainMenuLevel());
            Test->TestEqual(TEXT("Gameplay uses the configured play map"), UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()), GetDefault<UICGameFlowSettings>()->PlayMap.GetLongPackageName());
            Test->TestTrue(TEXT("Playable pawn possessed"), PC->GetPawn() != nullptr);
            Test->TestTrue(TEXT("Gameplay input unlocked"), PC->Menu.IsEmpty() && !World->IsPaused());
            const auto Snapshot = PC->GetWorldState()->ExportState();
            Test->TestEqual(TEXT("Answers survive map travel"), Snapshot.CreationAnswers.Music, 2);
            Test->TestEqual(TEXT("Gender follows answer"), PC->Preset, 1);
            Test->TestEqual(TEXT("No NPC fabricated for unconfigured choice"), Snapshot.CreationAnswers.Story, 2);
            Test->TestEqual(TEXT("Staff granted by music choice"), PC->GetInventory()->GetSelectedWeaponFamily(),
                            EICWeaponFamily::Staff);
            Test->TestEqual(TEXT("New game starts with empty hands"), PC->GetInventory()->GetEquippedWeaponFamily(),
                            EICWeaponFamily::None);
            Test->TestTrue(TEXT("Starting weapon is owned but put away"), Snapshot.Inventory.bWeaponSheathed);
            Test->TestTrue(TEXT("Initial auto save completed"),
                           PC->GetSaves()->GetSlotList()[0].bValid && !PC->GetSaves()->bSaving);
            Test->TestNotNull(TEXT("HUD mounted"), PC->Screen.Get());
            Test->TestEqual(TEXT("New game replaced the auto save"), PC->GetSaves()->GetSlotList()[0].PlayerName, FString(TEXT("테스트여행자")));
            TArray<uint8> CurrentManual;
            FFileHelper::LoadFileToArray(CurrentManual, *PC->GetSaves()->GetSlotPath(1));
            Test->TestTrue(TEXT("New game preserves manual save bytes"), CurrentManual == ManualBytes);
            PC->HandleInputCommand(EICInputCommand::Menu);
            PausedAt = PC->GetTime()->GetClock().PlaySecond;
            StageAt = FPlatformTime::Seconds();
            Stage = 2;
            return false;
        }
        if (Stage == 2)
        {
            if(FPlatformTime::Seconds()-StageAt<.25)return false;
            Test->TestEqual(TEXT("Pause freezes PlayClock"),PC->GetTime()->GetClock().PlaySecond,PausedAt);
            auto* CurrentMenu=Cast<UICGameMenuWidget>(PC->Screen->GetWidgetFromName(TEXT("GameMenu")));
            if(!Test->TestNotNull(TEXT("Current tabbed gameplay menu mounted"),CurrentMenu)){Cleanup();return true;}
            Test->TestTrue(TEXT("Menu cursor is visible"),PC->bShowMouseCursor);
            Capture(Client,TEXT("Morpheus_Menu"));
            CurrentMenu->SelectTab(TEXT("Settings"));CurrentMenu->NavigationArea=1;CurrentMenu->SettingsActionIndex=0;
            Test->TestTrue(TEXT("G confirms the selected menu action"),CurrentMenu->HandleMenuKey(EKeys::G));
            Test->TestTrue(TEXT("G resumes gameplay"),PC->Menu.IsEmpty()&&!World->IsPaused());
            PC->HandleInputCommand(EICInputCommand::Menu);
            CurrentMenu->SelectTab(TEXT("Settings"));CurrentMenu->NavigationArea=1;CurrentMenu->SettingsActionIndex=0;
            Test->TestTrue(TEXT("F confirms the selected menu action"),CurrentMenu->HandleMenuKey(EKeys::F));
            Test->TestTrue(TEXT("F resumes gameplay"),PC->Menu.IsEmpty()&&!World->IsPaused());
            PC->SetMenu(TEXT("Settings"));
            NextStage(36);return false;
        }
        if(Stage==36)
        {
            if(FPlatformTime::Seconds()-StageAt<.3)return false;
            auto* Settings=PC->SettingsScreen.Get();
            if(!Test->TestNotNull(TEXT("Morpheus options screen opens"),Settings)){Cleanup();return true;}
            auto* Toggle=CastChecked<UCheckBox>(Settings->GetWidgetFromName(TEXT("OPTION_bAutoSelectTarget")));
            const bool PreviousToggle=Toggle->IsChecked();PressKey(EKeys::F);
            Test->TestEqual(TEXT("Actual F event toggles the selected setting"),Toggle->IsChecked(),!PreviousToggle);
            PressKey(EKeys::W);PressKey(EKeys::D);PressKey(EKeys::S);
            Capture(Client,TEXT("Morpheus_Settings"));
            PressKey(EKeys::Escape);
            Test->TestTrue(TEXT("ESC closes options and releases gameplay"),PC->Menu.IsEmpty()&&!World->IsPaused());
            StartPosition=PC->GetPawn()->GetActorLocation();MovementStartedAt=World->GetTimeSeconds();NextStage(3);return false;
        }
        if (Stage == 3)
        {
            PC->GetPawn()->AddMovementInput(FVector::ForwardVector, 1.f);
            // Loading and shader work can consume wall time without advancing
            // the resumed world. Allow actual gameplay ticks before checking movement.
            if (World->GetTimeSeconds() - MovementStartedAt < .35)
                return false;
            Test->TestTrue(TEXT("Gameplay movement accepts input after reveal"),PC->Menu.IsEmpty()&&!World->IsPaused()&&PC->bCreated&&!PC->IsMoveInputIgnored());
            if(auto* Player=Cast<AICPlayerCharacter>(PC->GetPawn()))
                Test->TestTrue(TEXT("The character movement speed is unlocked"),Player->GetCharacterMovement()->MaxWalkSpeed>0);
            Test->TestFalse(TEXT("Cursor returns to gameplay"), PC->bShowMouseCursor);
            Test->TestTrue(TEXT("Initial save loads through save service"), PC->GetSaves()->LoadSlot(0));
            Test->TestEqual(TEXT("Questionnaire retained on load"),
                            PC->GetWorldState()->ExportState().CreationAnswers.Happiness, 3);
            Flow->ReturnToTitle(PC);
            NextStage(4);
            return false;
        }
        if (Stage == 4)
        {
            if (!PC->IsMainMenuLevel()) return false;
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICMainMenuWidget::StaticClass(), false);
            UICMainMenuWidget* Menu = nullptr;
            for (auto* Widget : Widgets)
                if (auto* Candidate = Cast<UICMainMenuWidget>(Widget))
                    if (Candidate->IsActivated()) Menu = Candidate;
            if (!Menu) return false;
            Test->TestTrue(TEXT("Continue is enabled after returning to title"),
                           CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Continue")))->GetIsEnabled());
            if (!Click(Client, CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Continue"))))) return false;
            Test->TestEqual(TEXT("Continue covers the title before reading the latest save"), PC->Menu, FString(TEXT("Loading")));
            NextStage(5);
            return false;
        }
        if (Stage == 5)
        {
            if (PC->IsMainMenuLevel() || !PC->bCreated || !PC->GetPawn() || Flow->Phase != EICGameFlowPhase::Playing) return false;
            Test->TestEqual(TEXT("Continue loads the newest valid save"), PC->PlayerName, FString(TEXT("테스트여행자")));
            Test->TestEqual(TEXT("Continue restores playing flow"), Flow->Phase, EICGameFlowPhase::Playing);
            Test->TestTrue(TEXT("Continue unlocks gameplay"), PC->Menu.IsEmpty() && !World->IsPaused());
            Flow->ReturnToTitle(PC);
            NextStage(30);
            return false;
        }
        if (Stage == 30)
        {
            if (!PC->IsMainMenuLevel()) return false;
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICMainMenuWidget::StaticClass(), false);
            UICMainMenuWidget* Menu = nullptr;
            for (auto* Widget : Widgets)
                if (auto* Candidate = Cast<UICMainMenuWidget>(Widget))
                    if (Candidate->IsActivated()) Menu = Candidate;
            if (!Menu) return false;
            if (!Click(Client, CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("BTN_SaveList"))))) return false;
            NextStage(31);
            return false;
        }
        if (Stage >= 31 && Stage <= 33)
        {
            if (FPlatformTime::Seconds() - StageAt < .3) return false;
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UICSaveLoadWidget::StaticClass(), false);
            UICSaveLoadWidget* List = nullptr;
            for (auto* Widget : Widgets)
                if (auto* Candidate = Cast<UICSaveLoadWidget>(Widget))
                    if (Candidate->GetParent()) List = Candidate;
            if (!List) return false;
            Test->TestEqual(TEXT("Title save list stays in load mode"), PC->GetSessionMenu(), FString(TEXT("Load")));
            if (Stage == 31 || Stage == 32)
            {
                const int32 Slot = Stage == 31 ? 1 : 0;
                auto* Rows = CastChecked<UVerticalBox>(List->GetWidgetFromName(TEXT("IC_SaveLoad_SlotList")));
                UICSaveSlotWidget* Row = nullptr;
                for (int32 I = 0; I < Rows->GetChildrenCount(); ++I)
                    if (auto* Candidate = Cast<UICSaveSlotWidget>(Rows->GetChildAt(I)))
                        if (Candidate->GetSlotIndex() == Slot) Row = Candidate;
                if (!Row || !Click(Client, CastChecked<UButton>(Row->GetWidgetFromName(TEXT("IC_SaveSlot_Button"))))) return false;
                Test->TestEqual(TEXT("Clicking a saved row changes selected load slot"), PC->SelectedLoadSlot, Slot);
            }
            else
            {
                auto* Load = CastChecked<UButton>(List->GetWidgetFromName(TEXT("IC_SaveLoad_Load")));
                Test->TestTrue(TEXT("Selected save can be loaded"), Load->GetIsEnabled());
                if (!Click(Client, Load)) return false;
                Test->TestEqual(TEXT("Clicking title load shows loading before restoring the slot"), PC->Menu, FString(TEXT("Loading")));
            }
            NextStage(Stage + 1);
            return false;
        }
        if (Stage == 34)
        {
            if (PC->IsMainMenuLevel() || !PC->bCreated || !PC->GetPawn() || Flow->Phase != EICGameFlowPhase::Playing) return false;
            Test->TestEqual(TEXT("Chosen title save restores the player"), PC->PlayerName, FString(TEXT("테스트여행자")));
            Test->TestTrue(TEXT("Chosen title save returns to active gameplay"), PC->Menu.IsEmpty() && !World->IsPaused());
            Cleanup();
            return true;
        }
        return false;
    }

private:
    FAutomationTestBase* Test;
    TWeakObjectPtr<UGameInstance> GI;
    int32 Stage = 0;
    bool bSeededSaves = false;
    bool bRequestedEmptyContinue = false;
    bool bSkipRun=false,bPartialSkip=false;
    bool bSawStoryLoading=false;
    bool bSawCharacterPreparation=false,bSawPreparedReveal=false;
    double StoryLoadingFirst=0,StoryLoadingLast=0;
    double MovementStartedAt=0;
    TArray<uint8> ManualBytes;
    double Started = 0, StageAt = 0, PausedAt = 0;
    FVector StartPosition = FVector::ZeroVector;
    void NextStage(int32 Value) { Stage = Value; StageAt = FPlatformTime::Seconds(); }
    void PressKey(FKey Key)
    {
        FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
        FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
    }
    bool Click(UGameViewportClient* Client, UButton* Button)
    {
        const auto G = Button->GetCachedGeometry();
        if (G.GetLocalSize().X < 1 || G.GetLocalSize().Y < 1) return false;
        const FVector2D Position = G.LocalToAbsolute(G.GetLocalSize() * .5f);
        auto& App = FSlateApplication::Get(); App.SetCursorPos(Position);
        FPointerEvent Move(0, Position, Position, TSet<FKey>{}, EKeys::Invalid, 0, FModifierKeysState()); App.ProcessMouseMoveEvent(Move);
        FPointerEvent Down(0, Position, Position, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState()); App.ProcessMouseButtonDownEvent(Client->GetWindow()->GetNativeWindow(), Down);
        FPointerEvent Up(0, Position, Position, TSet<FKey>{}, EKeys::LeftMouseButton, 0, FModifierKeysState()); App.ProcessMouseButtonUpEvent(Up);
        return true;
    }
    void TestMenuButtons(AICPlayerController* PC, std::initializer_list<const TCHAR*> Visible)
    {
        for (const auto* Key : {TEXT("Party"), TEXT("Inventory"), TEXT("Quest"), TEXT("Codex"), TEXT("Settings"), TEXT("Resume"), TEXT("Save"), TEXT("Load"), TEXT("GameSettings"), TEXT("Quit")})
        {
            bool Expected = false;
            for (const auto* Entry : Visible) if (FCString::Strcmp(Key, Entry) == 0) Expected = true;
            auto* Button = CastChecked<UButton>(PC->Screen->GetWidgetFromName(FName(*FString::Printf(TEXT("BTN_%s"), Key))));
            Test->TestEqual(TEXT("Only expected menu buttons are visible"), Button->GetVisibility() == ESlateVisibility::Visible, Expected);
        }
    }
    void Capture(UGameViewportClient* Client, const FString& Name)
    {
        TArray<FColor> Pixels; FIntVector Size;
        if (!FSlateApplication::Get().TakeScreenshot(Client->GetWindow().ToSharedRef(), Pixels, Size)) return;
        const FString Folder = FPaths::ProjectSavedDir() / TEXT("CreationMenuReview"); IFileManager::Get().MakeDirectory(*Folder, true);
        for (auto& Pixel : Pixels) Pixel.A = 255;
        TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
        FFileHelper::SaveArrayToFile(PNG, *(Folder / Name + TEXT(".png")));
    }
    void Cleanup()
    {
        if (GI.IsValid())
            if (auto* Saves = GI->GetSubsystem<UICSaveSubsystem>())
                if (!Saves->bSaving)
                { Saves->DeleteSlot(0); Saves->DeleteSlot(1); }
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
    auto RestorePlayWindow = MakeShared<FICPlayWindowSettingsRestore>();
    auto Window = SNew(SWindow).ClientSize(FVector2D(1280, 720)).UseOSWindowBorder(false).CreateTitleBar(false);
    FSlateApplication::Get().AddWindow(Window);
    FRequestPlaySessionParams Params; Params.CustomPIEWindow = Window; Params.bAllowOnlineSubsystem = false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICPlaythroughCommand>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    FAutomationTestFramework::Get().EnqueueLatentCommand(RestorePlayWindow);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFrontendSkipCheck,"IC.Frontend.PrologueSkipToPlay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICFrontendSkipCheck::RunTest(const FString& Parameters)
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
    auto RestorePlayWindow = MakeShared<FICPlayWindowSettingsRestore>();
    auto Window = SNew(SWindow).ClientSize(FVector2D(1280, 720)).UseOSWindowBorder(false).CreateTitleBar(false);
    FSlateApplication::Get().AddWindow(Window);
    FRequestPlaySessionParams Params; Params.CustomPIEWindow = Window; Params.bAllowOnlineSubsystem = false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICPlaythroughCommand>(this,true));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    FAutomationTestFramework::Get().EnqueueLatentCommand(RestorePlayWindow);
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
