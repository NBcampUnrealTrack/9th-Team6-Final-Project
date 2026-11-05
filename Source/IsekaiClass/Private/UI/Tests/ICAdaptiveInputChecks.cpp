#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "PlayInEditorDataTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Slate/SceneViewport.h"
#include "EngineUtils.h"
#include "CommonActivatableWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/SWindow.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Core/Controller/ICPlayerController.h"
#include "NPC/Character/ICNPCBase.h"
#include "Save/ICSaveSubsystem.h"
#include "Settings/ICGameUserSettings.h"
#include "Time/ICTimeSubsystem.h"
#include "UI/MainMenu/ICMainMenuWidget.h"
#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "UI/Settings/ICSettingsWidget.h"
#include "UI/Save/ICSaveLoadWidget.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "UI/Gameplay/ICGameMenuWidget.h"
#include "UI/Gameplay/ICQuickSlotWidget.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "UI/Management/ICUIActionRouter.h"
#include "UI/Framework/ICUIRootWidget.h"
#include "UI/Framework/ICInputPresentation.h"
#include "UI/Tests/ICPlayWindowSettingsRestore.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace ICAdaptiveInputChecks
{
template<class T> T* Mounted(UWorld* World)
{
    TArray<UUserWidget*> Widgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,T::StaticClass(),false);
    for(auto* Widget:Widgets)
    {
        const auto* Active=Cast<UCommonActivatableWidget>(Widget);
        if(Widget->GetParent()||Widget->IsInViewport()||(Active&&Active->IsActivated()))return Cast<T>(Widget);
    }
    return nullptr;
}

class FReview : public IAutomationLatentCommand
{
public:
    FReview(FAutomationTestBase* InTest,bool InTitle):Test(InTest),Title(InTitle),Started(FPlatformTime::Seconds())
    {
        auto* Settings=UICGameUserSettings::Get();PreviousMode=Settings->InputDeviceMode;PreviousLayout=Settings->GamepadLayout;
        Settings->InputDeviceMode=0;Settings->GamepadLayout=1;
    }
    ~FReview(){RestoreSettings();}
    virtual bool Update() override
    {
        if(FPlatformTime::Seconds()-Started>150){Test->AddError(FString::Printf(TEXT("Adaptive input review timed out at stage %d"),Stage));return Finish();}
        UWorld* World=GEditor->PlayWorld;
        auto* PC=World?Cast<AICPlayerController>(World->GetFirstPlayerController()):nullptr;
        auto* Client=PC&&PC->GetLocalPlayer()?PC->GetLocalPlayer()->ViewportClient.Get():nullptr;
        if(!PC||!Client||!Client->GetWindow())return false;
        if(!Activated)
        {
            auto Window=Client->GetWindow();Window->BringToFront(true);
            FSlateApplication::Get().ProcessWindowActivatedEvent(FWindowActivateEvent(FWindowActivateEvent::EA_Activate,Window.ToSharedRef()));
            FSlateApplication::Get().ReleaseAllPointerCapture();Activated=true;
        }
        if(FPlatformTime::Seconds()-StageAt<.25)return false;
        return Title?ReviewTitle(PC,Client):ReviewGameplay(PC,Client);
    }
private:
    void RestoreSettings()
    {
        if(Restored)return;
        auto* Settings=UICGameUserSettings::Get();Settings->InputDeviceMode=PreviousMode;Settings->GamepadLayout=PreviousLayout;Restored=true;
    }
    bool Finish(){RestoreSettings();return true;}
    bool SizeForCapture(UGameViewportClient* Client)
    {
        if(Sized)return true;
        const auto Window=Client->GetWindow();const auto* Viewport=Client->GetGameViewport();
        Window->Resize(FVector2D(1600,900)+FVector2D(Window->GetClientSizeInScreen())-FVector2D(Viewport->GetSizeXY()));
        Sized=true;StageAt=FPlatformTime::Seconds();return false;
    }
    void Next(int32 Value){Stage=Value;StageAt=FPlatformTime::Seconds();}
    void Press(FKey Key)
    {
        auto& Slate=FSlateApplication::Get();
        Slate.ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
        Slate.ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
    }
    void Stick(FKey Axis,float Value)
    {FSlateApplication::Get().ProcessAnalogInputEvent(FAnalogInputEvent(Axis,FModifierKeysState(),0,false,0,0,Value));}
    bool Click(UGameViewportClient* Client,UUserWidget* Screen,FName Name)
    {
        auto* Target=Screen?Screen->GetWidgetFromName(Name):nullptr;
        if(!Test->TestNotNull(*FString::Printf(TEXT("Clickable widget %s exists"),*Name.ToString()),Target))return false;
        const auto Geometry=Target->GetCachedGeometry();
        if(!Test->TestTrue(TEXT("The real click target has a laid-out area"),Geometry.GetLocalSize().GetMin()>1))return false;
        const FVector2D Position=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
        auto& Slate=FSlateApplication::Get();const FVector2D Before=Slate.GetCursorPos();Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0,Position,Before,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState()));
        Slate.ProcessMouseButtonDownEvent(Client->GetWindow()->GetNativeWindow(),FPointerEvent(0,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(0,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));
        return true;
    }
    void CheckPointer(AICPlayerController* PC,UGameViewportClient* Client,bool Visible=true)
    {
        Test->TestEqual(TEXT("Controller cursor follows UI context, independently of device"),PC->ShouldShowMouseCursor(),Visible);
        auto* UI=PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
        Test->TestEqual(TEXT("Shared pointer policy follows UI context"),UI->IsPointerUIActive(),Visible);
        if(const auto User=FSlateApplication::Get().GetUser(0))Test->TestEqual(FString::Printf(TEXT("Slate cursor follows UI (stage %d, menu %s, pad %d)"),Stage,*PC->Menu,PC->IsUsingGamepad()),User->IsCursorVisible(),Visible);
        if(Visible)
        {
            Test->TestEqual(TEXT("UI does not capture the mouse"),Client->GetMouseCaptureMode(),EMouseCaptureMode::NoCapture);
            Test->TestFalse(TEXT("Clicking UI never hides the captured pointer"),Client->HideCursorDuringCapture());
            Test->TestTrue(TEXT("UI supports pointer click and hover events"),PC->bEnableClickEvents&&PC->bEnableMouseOverEvents);
        }
    }
    void Capture(UGameViewportClient* Client,const TCHAR* Name)
    {
        TArray<FColor> Pixels;FIntVector Size;
        if(!Test->TestTrue(TEXT("Native Slate screenshot captured"),FSlateApplication::Get().TakeScreenshot(Client->GetWindow().ToSharedRef(),Pixels,Size)))return;
        for(auto& Pixel:Pixels)Pixel.A=255;
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("AdaptiveInputUI_20261105/Captures");IFileManager::Get().MakeDirectory(*Folder,true);
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
        Test->TestTrue(TEXT("Native screenshot saved"),FFileHelper::SaveArrayToFile(PNG,*(Folder/FString(Name)+TEXT(".png"))));
    }
    bool ReviewTitle(AICPlayerController* PC,UGameViewportClient* Client)
    {
        auto* Main=Mounted<UICMainMenuWidget>(PC->GetWorld());
        if(Stage==0)
        {
            if(!Main||!Main->IsActivated())return false;
            if(!SizeForCapture(Client))return false;
            Test->TestNotNull(TEXT("Project-specific CommonUI cursor policy is active"),PC->GetLocalPlayer()->GetSubsystem<UICUIActionRouter>());
            CheckPointer(PC,Client);Capture(Client,TEXT("Title_Keyboard"));Press(EKeys::Gamepad_DPad_Down);Next(1);return false;
        }
        if(Stage==1)
        {
            Test->TestTrue(TEXT("A gamepad key consumed by the title UI still switches prompts"),PC->IsUsingGamepad());CheckPointer(PC,Client);
            const FVector2D Before=FSlateApplication::Get().GetCursorPos();Press(EKeys::Gamepad_DPad_Down);Stick(EKeys::Gamepad_LeftY,-.9f);Stick(EKeys::Gamepad_LeftY,0);
            Test->TestTrue(TEXT("Gamepad focus navigation never warps the physical mouse"),Before.Equals(FSlateApplication::Get().GetCursorPos(),.1));
            Capture(Client,TEXT("Title_Gamepad"));Click(Client,Main,TEXT("BTN_Settings"));Next(2);return false;
        }
        if(Stage==2)
        {
            if(!PC->SettingsScreen)return false;
            Test->TestEqual(TEXT("A real mouse click opens settings after gamepad navigation"),PC->Menu,FString(TEXT("Settings")));
            Test->TestFalse(TEXT("Real mouse input restores keyboard/mouse hints"),PC->IsUsingGamepad());CheckPointer(PC,Client);
            Capture(Client,TEXT("Settings_Keyboard"));Press(EKeys::Gamepad_DPad_Down);Next(3);return false;
        }
        if(Stage==3)
        {
            Test->TestTrue(TEXT("Settings key handling switches to pad hints"),PC->IsUsingGamepad());CheckPointer(PC,Client);
            auto* Help=Cast<UTextBlock>(PC->SettingsScreen->GetWidgetFromName(TEXT("TXT_SettingsKeys")));
            Test->TestTrue(TEXT("Settings help shows actual gamepad navigation"),Help&&Help->GetText().ToString().Contains(TEXT("LB/RB")));
            Capture(Client,TEXT("Settings_Gamepad"));Press(UICGameUserSettings::GamepadFaceKey(2));Next(4);return false;
        }
        if(Stage==4){if(!Main||!Main->IsActivated())return false;Click(Client,Main,TEXT("BTN_SaveList"));Next(5);return false;}
        auto* Load=Mounted<UICSaveLoadWidget>(PC->GetWorld());
        if(Stage==5)
        {
            if(!Load)return false;
            Test->TestEqual(TEXT("Title save list opens from a real click"),PC->GetSessionMenu(),FString(TEXT("Load")));CheckPointer(PC,Client);
            Capture(Client,TEXT("TitleLoad_Keyboard"));Press(EKeys::Gamepad_DPad_Down);Next(6);return false;
        }
        if(Stage==6)
        {
            Test->TestTrue(TEXT("Title save list detects gamepad input"),PC->IsUsingGamepad());CheckPointer(PC,Client);
            Capture(Client,TEXT("TitleLoad_Gamepad"));Click(Client,Load,TEXT("IC_SaveLoad_Close"));Next(7);return false;
        }
        Test->TestEqual(TEXT("Visible close button returns to title by mouse"),PC->Menu,FString(TEXT("MainMenu")));CheckPointer(PC,Client);return Finish();
    }
    bool ReviewGameplay(AICPlayerController* PC,UGameViewportClient* Client)
    {
        auto* Saves=PC->GetSaves();
        if(Stage==0)
        {
            if(!Saves||!PC->CharacterCreateScreen||!PC->CharacterCreateScreen->IsActivated()||!Saves->OnSaveFinished.Contains(PC,TEXT("SaveFinished")))return false;
            if(!SizeForCapture(Client))return false;
            if(!Test->TestTrue(TEXT("Review saves use an isolated prefix"),Saves->ConfigureServices(PC->GetWorldState(),PC->GetTime(),TEXT("IC_AdaptiveInput_")+FGuid::NewGuid().ToString(EGuidFormats::Digits))))return Finish();
            PC->PlayerName=TEXT("입력 UI 검증");Test->TestTrue(TEXT("Create playable review character"),PC->DebugCreatePlayer());Next(1);return false;
        }
        if(Stage==1)
        {
            if(!PC->bCreated||Saves->bSaving||!Saves->GetSlotList()[0].bValid)return false;
            const double Minute=FMath::Fmod(PC->GetTime()->GetClock().GameMinute,1440.);
            if(Minute<480.||Minute>1080.)PC->GetTime()->JumpGameMinutes(FMath::Fmod(720.-Minute+1440.,1440.));
            Saves->SetAutoSaveSuspended(true);PC->SetMenu(TEXT(""));Next(2);return false;
        }
        if(Stage==2){CheckPointer(PC,Client,false);Capture(Client,TEXT("HUD_Keyboard"));Stick(EKeys::Gamepad_RightX,.8f);Stick(EKeys::Gamepad_RightX,0);Next(3);return false;}
        if(Stage==3)
        {
            Test->TestTrue(TEXT("Gameplay detects the real pad axis"),PC->IsUsingGamepad());CheckPointer(PC,Client,false);Capture(Client,TEXT("HUD_Gamepad"));
            PC->SetMenu(TEXT("Inventory"));Next(4);return false;
        }
        auto* Menu=Mounted<UICGameMenuWidget>(PC->GetWorld());
        if(Stage==4)
        {
            if(!Menu)return false;CheckPointer(PC,Client);Capture(Client,TEXT("Menu_BeforePadInput"));Press(EKeys::Gamepad_RightShoulder);Next(5);return false;
        }
        if(Stage==5)
        {
            auto* Pages=Cast<UWidgetSwitcher>(Menu->GetWidgetFromName(TEXT("MenuPages")));
            Test->TestEqual(TEXT("RB changes the actual menu tab"),Pages->GetActiveWidgetIndex(),2);
            Test->TestTrue(TEXT("A gamepad key consumed by the game menu switches its prompts"),PC->IsUsingGamepad());
            CheckPointer(PC,Client);Capture(Client,TEXT("Menu_Gamepad"));
            Press(EKeys::W);Stick(EKeys::Gamepad_LeftY,.05f);Stick(EKeys::Gamepad_LeftY,0);
            Next(51);return false;
        }
        if(Stage==51)
        {
            Test->TestFalse(TEXT("Stick drift does not replace keyboard prompts"),PC->IsUsingGamepad());CheckPointer(PC,Client);Capture(Client,TEXT("Menu_Keyboard"));
            // Hint-only overrides preserve the actual pointer and both input devices.
            UICGameUserSettings::Get()->InputDeviceMode=2;ICInputPresentation::RefreshHints(PC->GetLocalPlayer());
            Test->TestTrue(TEXT("Forced pad hint setting works"),PC->IsUsingGamepad());CheckPointer(PC,Client);
            UICGameUserSettings::Get()->InputDeviceMode=1;Test->TestFalse(TEXT("Forced keyboard hint setting works"),PC->IsUsingGamepad());
            UICGameUserSettings::Get()->InputDeviceMode=0;
            Click(Client,Menu,TEXT("TAB_Settings"));Next(6);return false;
        }
        if(Stage==6){Click(Client,Menu,TEXT("BTN_Resume"));Next(7);return false;}
        if(Stage==7)
        {
            Test->TestTrue(TEXT("Mouse resume returns to gameplay"),PC->Menu.IsEmpty());CheckPointer(PC,Client,false);
            AICNPCBase* NPC=nullptr;
            for(TActorIterator<AICNPCBase> It(PC->GetWorld());It;++It)if(It->GetCharacterID()==TEXT("NPC_R01_003")){NPC=*It;break;}
            if(!Test->TestNotNull(TEXT("Use the actual spawned NPC, preserving unique registration"),NPC))return Finish();
            NPC->SetActorLocation(PC->GetPawn()->GetActorLocation()+FVector(100,0,0),false,nullptr,ETeleportType::TeleportPhysics);
            FScriptDelegate Interaction;Interaction.BindUFunction(PC,TEXT("HandleNPCInteractionStarted"));NPC->OnInteractionStarted.AddUnique(Interaction);
            Test->TestTrue(TEXT("Real NPC interaction opens dialogue"),NPC->TryInteract(PC));Next(8);return false;
        }
        if(Stage==8)
        {
            if(!PC->DialogueScreen)return false;
            CheckPointer(PC,Client);Capture(Client,TEXT("Dialogue_Keyboard"));Press(EKeys::Gamepad_DPad_Down);Next(9);return false;
        }
        if(Stage==9)
        {
            Test->TestTrue(TEXT("Consumed NPC dialogue pad input changes hints"),PC->IsUsingGamepad());CheckPointer(PC,Client);Capture(Client,TEXT("Dialogue_Gamepad"));
            auto* Entries=Cast<UVerticalBox>(PC->DialogueScreen->GetWidgetFromName(TEXT("Entries")));
            auto* Entry=Entries&&Entries->GetChildrenCount()?Cast<UUserWidget>(Entries->GetChildAt(0)):nullptr;
            Click(Client,Entry,TEXT("EntryButton"));Next(10);return false;
        }
        if(Stage==10)
        {
            auto* Entries=PC->DialogueScreen?Cast<UVerticalBox>(PC->DialogueScreen->GetWidgetFromName(TEXT("Entries"))):nullptr;
            Test->TestTrue(TEXT("Mouse click advances dialogue to interaction choices"),Entries&&Entries->GetChildrenCount()>1);CheckPointer(PC,Client);
            Press(EKeys::Escape);Next(11);return false;
        }
        Test->TestTrue(TEXT("Closing dialogue restores gameplay"),PC->Menu.IsEmpty()&&!PC->bInDialogue);CheckPointer(PC,Client,false);
        if(!Saves->bSaving)Saves->DeleteSlot(0);return Finish();
    }
    FAutomationTestBase* Test;
    bool Title=false,Activated=false,Restored=false,Sized=false;
    int32 Stage=0,PreviousMode=0,PreviousLayout=1;
    double Started,StageAt=0;
};

bool Start(FAutomationTestBase* Test,bool Title)
{
    if(GEditor->PlayWorld){Test->AddError(TEXT("Close the existing PIE session before reviewing input UI."));return false;}
    auto Restore=MakeShared<FICPlayWindowSettingsRestore>();
    if(!FEditorFileUtils::LoadMap(Title?TEXT("/Game/IsekaiClass/World/Maps/L_Title"):TEXT("/Game/IsekaiClass/World/L_TestMap"),false,true))return false;
    auto Window=SNew(SWindow).ClientSize(FVector2D(1600,900)).UseOSWindowBorder(false).CreateTitleBar(false);FSlateApplication::Get().AddWindow(Window);
    FRequestPlaySessionParams Params;Params.CustomPIEWindow=Window;Params.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FReview>(Test,Title));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    FAutomationTestFramework::Get().EnqueueLatentCommand(Restore);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICAdaptiveTitleInputCheck,"IC.UI.InputDevices.TitleMouseAndPad",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICAdaptiveTitleInputCheck::RunTest(const FString&){return ICAdaptiveInputChecks::Start(this,true);}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICAdaptiveGameplayInputCheck,"IC.UI.InputDevices.GameplayMenusAndDialogue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICAdaptiveGameplayInputCheck::RunTest(const FString&){return ICAdaptiveInputChecks::Start(this,false);}
#endif
