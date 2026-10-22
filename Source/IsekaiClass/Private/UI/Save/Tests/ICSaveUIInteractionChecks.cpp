#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "PlayInEditorDataTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SceneViewport.h"
#include "SceneView.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Core/Controller/ICPlayerController.h"
#include "Save/ICSaveSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "UI/Save/ICSaveLoadWidget.h"
#include "UI/Save/ICLoadConfirmWidget.h"
#include "UI/Save/ICSaveSlotWidget.h"
#include "UI/Tests/ICPlayWindowSettingsRestore.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "UI/Settings/ICSettingsWidget.h"
#include "UI/Quest/ICQuestTrackerWidget.h"
#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
template<class T> T* Mounted(UWorld* World)
{
    TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,T::StaticClass(),false);
    for(auto* Widget:Widgets)if(Widget->GetParent())return Cast<T>(Widget);
    return nullptr;
}
class FICSaveUICommand : public IAutomationLatentCommand
{
public:
    explicit FICSaveUICommand(FAutomationTestBase* InTest):Test(InTest),Started(FPlatformTime::Seconds()){}
    bool Update() override
    {
        if(FPlatformTime::Seconds()-Started>100){Test->AddError(FString::Printf(TEXT("Save UI timed out at stage %d"),Stage));Cleanup();return true;}
        UWorld* World=GEditor->PlayWorld;
        auto* PC=World?Cast<AICPlayerController>(World->GetFirstPlayerController()):nullptr;
        if(!PC||!PC->GetPawn()||!PC->GetLocalPlayer())return false;
        auto* Saves=PC->GetSaves();auto* Client=PC->GetLocalPlayer()->ViewportClient.Get();
        auto* Viewport=Client?Client->GetGameViewport():nullptr;
        if(!Saves||!Viewport||!Client->GetWindow())return false;
        if(Stage==0)
        {
            // Wait for gameplay initialization to bind its save completion handler.
            if(!PC->CharacterCreateScreen||!PC->CharacterCreateScreen->IsActivated())return false;
            GI=PC->GetGameInstance();
            if(!Test->TestTrue(TEXT("UI saves use an isolated prefix"),Saves->ConfigureServices(PC->GetWorldState(),PC->GetTime(),TEXT("IC_SaveUI_")+FGuid::NewGuid().ToString(EGuidFormats::Digits))))return true;
            PC->PlayerName=TEXT("저장UI검증");
            Test->TestTrue(TEXT("Create real playable test character"),PC->DebugCreatePlayer());
            Stage=1;return false;
        }
        if(Stage==1)
        {
            if(!PC->bCreated||Saves->bSaving||!Saves->GetSlotList()[0].bValid)return false;
            PC->SetMenu(TEXT("Pause"));PC->OpenSaveMenu();Stage=2;return false;
        }
        if(Stage==18)
        {
            if(FPlatformTime::Seconds()-StageAt<.3)return false;
            Test->TestEqual(TEXT("Close returns to previous pause menu"),PC->Menu,FString(TEXT("Pause")));
            PC->SetMenu(TEXT("Settings"));Next(24);return false;
        }
        if(Stage==24)
        {
            if(FPlatformTime::Seconds()-StageAt<.3)return false;
            auto* Settings=PC->SettingsScreen.Get();
            Test->TestTrue(TEXT("Settings uses the common C++ UI root"),Settings&&Settings->GetParent());
            Test->TestTrue(TEXT("Quest tracker uses the common HUD layer"),PC->QuestTrackerScreen&&PC->QuestTrackerScreen->GetParent());
            if(Settings)
            {
                const auto VG=Viewport->GetCachedGeometry();const auto G=Settings->GetCachedGeometry();
                const FVector2D Scale=FVector2D(Viewport->GetSizeXY())/FVector2D(VG.GetLocalSize());
                const auto Min=FVector2D(VG.AbsoluteToLocal(G.GetAbsolutePosition()))*Scale;
                const auto Max=FVector2D(VG.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize())))*Scale;
                const auto Rect=ICDisplayPolicy::ContentRect(Viewport->GetSizeXY());
                Test->TestTrue(TEXT("Settings fills the shared game/UI area"),Min.Equals(FVector2D(Rect.Min),2)&&Max.Equals(FVector2D(Rect.Max),2));
            }
            PC->SetMenu(TEXT(""));Next(19);return false;
        }
        if(Stage==19)
        {
            if(FPlatformTime::Seconds()-StageAt<.3)return false;
            Test->TestFalse(TEXT("Closing save UI releases pause requests"),World->IsPaused());
            Test->TestFalse(TEXT("Closing save UI restores gameplay cursor"),PC->bShowMouseCursor);
            SavedGold=PC->Gold;PC->Gold+=7;PC->GetWorldState()->SetPlayerGold(PC->Gold);
            PC->OpenLoadMenu();PC->SelectSessionSlot(0);Next(20);return false;
        }
        auto* View=Mounted<UICSaveLoadWidget>(World);
        if(!View)return false;
        if(Stage==2)
        {
            const auto Size=Sizes[SizeIndex];auto Window=Client->GetWindow();
            Window->Resize(FVector2D(Size)+FVector2D(Window->GetClientSizeInScreen())-FVector2D(Viewport->GetSizeXY()));
            Window->BringToFront(true);auto& App=FSlateApplication::Get();App.ReleaseAllPointerCapture();
            App.ProcessWindowActivatedEvent(FWindowActivateEvent(FWindowActivateEvent::EA_Activate,Window.ToSharedRef()));
            PC->SelectSessionSlot(1);
            if(auto* Row=FindRow(World,2))if(auto* Scroll=Cast<UScrollBox>(View->GetWidgetFromName(TEXT("IC_SaveLoad_Scroll"))))Scroll->ScrollWidgetIntoView(Row,false);
            Next(3);return false;
        }
        if(FPlatformTime::Seconds()-StageAt<.3)return false;
        if(Stage==3)
        {
            FSceneViewProjectionData Projection;PC->GetLocalPlayer()->GetProjectionData(Viewport,Projection);
            const auto Rect=ICDisplayPolicy::ContentRect(Viewport->GetSizeXY());const auto VG=Viewport->GetCachedGeometry();const auto UI=View->GetCachedGeometry();
            const auto GameRect=Projection.GetConstrainedViewRect();
            Test->TestTrue(TEXT("Gameplay projection shares the common UI content rect"),FVector2D(GameRect.Min).Equals(FVector2D(Rect.Min),1)&&FVector2D(GameRect.Max).Equals(FVector2D(Rect.Max),1));
            const FVector2D Scale=FVector2D(Viewport->GetSizeXY())/FVector2D(VG.GetLocalSize());
            const auto Min=FVector2D(VG.AbsoluteToLocal(UI.GetAbsolutePosition()))*Scale;
            const auto Max=FVector2D(VG.AbsoluteToLocal(UI.LocalToAbsolute(UI.GetLocalSize())))*Scale;
            Test->TestTrue(TEXT("Save UI follows the WBP 1920x1080 ScaleBox after resize"),Min.Equals(FVector2D(Rect.Min),2)&&Max.Equals(FVector2D(Rect.Max),2));
            auto* Row=FindRow(World,2);if(!Row)return false;
            if(!Click(Client,Row,TEXT("IC_SaveSlot_Button")))return false;
            Next(4);return false;
        }
        if(Stage==4)
        {
            Test->TestEqual(TEXT("Real pointer selects visible manual slot after resize"),PC->SelectedSaveSlot,2);
            Capture(Client,FString::Printf(TEXT("Save_%d"),SizeIndex));
            if(++SizeIndex<UE_ARRAY_COUNT(Sizes)){Stage=2;return false;}
            PC->SelectSessionSlot(1);Next(5);return false;
        }
        if(Stage==5){if(!Click(Client,View,TEXT("IC_SaveLoad_Save")))return false;Next(6);return false;}
        if(Stage==6)
        {
            if(Saves->bSaving||!Saves->GetSlotList()[1].bValid)return false;
            Test->TestTrue(TEXT("Saved file contains no screenshot"),Saves->GetSlotList()[1].Preview.ThumbnailPNG.IsEmpty());
            Test->TestNull(TEXT("Save screen has no screenshot widget"),View->GetWidgetFromName(TEXT("IC_SaveLoad_Thumbnail")));
            if(auto* Row=FindRow(World,1))
            {
                for(int32 I=1;I<4;++I)
                    Test->TestTrue(TEXT("Missing companion shows a question mark"),Row->GetWidgetFromName(FName(*FString::Printf(TEXT("IC_SaveSlot_Party%d_Empty"),I)))->GetVisibility()==ESlateVisibility::HitTestInvisible);
            }
            Test->TestTrue(TEXT("Menu keeps game paused during save UI"),World->IsPaused());Capture(Client,TEXT("Saved"));
            Click(Client,View,TEXT("IC_SaveLoad_Save"));Next(7);return false;
        }
        auto* Popup=Mounted<UICLoadConfirmWidget>(World);
        if(Stage==7)
        {
            if(!Popup)return false;
            Test->TestFalse(TEXT("Overwrite popup disables underlying save controls"),View->GetIsEnabled());Capture(Client,TEXT("Overwrite"));
            Click(Client,Popup,TEXT("IC_LoadConfirm_Cancel"));Next(8);return false;
        }
        if(Stage==8)
        {
            Test->TestTrue(TEXT("Cancel overwrite preserves save and restores controls"),PC->Menu==TEXT("Save")&&Saves->GetSlotList()[1].bValid&&View->GetIsEnabled());
            Click(Client,View,TEXT("IC_SaveLoad_Save"));Next(9);return false;
        }
        if(Stage==9){if(!Popup)return false;Click(Client,Popup,TEXT("IC_LoadConfirm_Confirm"));Next(10);return false;}
        if(Stage==10)
        {
            if(Saves->bSaving||PC->Menu==TEXT("Confirm"))return false;
            Test->TestTrue(TEXT("Confirmed overwrite leaves valid save"),Saves->GetSlotList()[1].bValid);
            Click(Client,View,TEXT("IC_SaveLoad_Delete"));Next(11);return false;
        }
        if(Stage==11){if(!Popup)return false;Click(Client,Popup,TEXT("IC_LoadConfirm_Cancel"));Next(12);return false;}
        if(Stage==12)
        {
            Test->TestTrue(TEXT("Cancel delete preserves file"),Saves->GetSlotList()[1].bValid);
            Click(Client,View,TEXT("IC_SaveLoad_Delete"));Next(13);return false;
        }
        if(Stage==13){if(!Popup)return false;Click(Client,Popup,TEXT("IC_LoadConfirm_Confirm"));Next(14);return false;}
        if(Stage==14)
        {
            Test->TestFalse(TEXT("Confirmed delete removes isolated slot"),Saves->GetSlotList()[1].bExists);
            PC->SetMenu(TEXT("Pause"));PC->OpenLoadMenu();PC->SelectSessionSlot(0);Next(15);return false;
        }
        if(Stage==15)
        {
            auto* Load=Cast<UButton>(View->GetWidgetFromName(TEXT("IC_SaveLoad_Load")));
            Test->TestTrue(TEXT("Load mode shows enabled load action"),Load&&Load->IsVisible()&&Load->GetIsEnabled());
            Capture(Client,TEXT("Load"));Click(Client,View,TEXT("IC_SaveLoad_Load"));Next(16);return false;
        }
        if(Stage==16){if(!Popup)return false;Capture(Client,TEXT("LoadConfirm"));Click(Client,Popup,TEXT("IC_LoadConfirm_Cancel"));Next(17);return false;}
        if(Stage==17)
        {
            Test->TestTrue(TEXT("Cancel load preserves current session"),PC->Menu==TEXT("Load")&&PC->bCreated);
            Click(Client,View,TEXT("IC_SaveLoad_Close"));Next(18);return false;
        }
        if(Stage==20){Click(Client,View,TEXT("IC_SaveLoad_Load"));Next(21);return false;}
        if(Stage==21){if(!Popup)return false;Click(Client,Popup,TEXT("IC_LoadConfirm_Confirm"));Next(23);return false;}
        return false;
    }
    bool FinishIfReady()
    {
        if(Stage!=23||FPlatformTime::Seconds()-StageAt<.3)return false;
        UWorld* World=GEditor->PlayWorld;auto* PC=World?Cast<AICPlayerController>(World->GetFirstPlayerController()):nullptr;
        if(!PC)return false;
        Test->TestFalse(TEXT("Closing save UI releases its pause requests"),World->IsPaused());
        Test->TestFalse(TEXT("Gameplay cursor restored"),PC->bShowMouseCursor);
        Test->TestEqual(TEXT("Load confirmation restores the saved player state"),PC->Gold,SavedGold);
        Test->TestTrue(TEXT("Load confirmation resumes gameplay"),PC->Menu.IsEmpty()&&PC->bCreated);Cleanup();return true;
    }
private:
    UICSaveSlotWidget* FindRow(UWorld* World,int32 Slot)
    {
        TArray<UUserWidget*> Rows;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Rows,UICSaveSlotWidget::StaticClass(),false);
        for(auto* Widget:Rows)if(auto* Row=Cast<UICSaveSlotWidget>(Widget))if(Row->GetParent()&&Row->GetSlotIndex()==Slot)return Row;
        return nullptr;
    }
    bool Click(UGameViewportClient* Client,UUserWidget* View,const TCHAR* Name)
    {
        auto* Button=Cast<UButton>(View->GetWidgetFromName(Name));if(!Button||!Button->GetIsEnabled())return false;
        const auto G=Button->GetCachedGeometry();if(G.GetLocalSize().X<1||G.GetLocalSize().Y<1)return false;
        const FVector2D Position=G.LocalToAbsolute(G.GetLocalSize()*.5f);auto& App=FSlateApplication::Get();App.SetCursorPos(Position);
        FPointerEvent Move(0,Position,Position,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState());App.ProcessMouseMoveEvent(Move);
        FPointerEvent Down(0,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());App.ProcessMouseButtonDownEvent(Client->GetWindow()->GetNativeWindow(),Down);
        FPointerEvent Up(0,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());App.ProcessMouseButtonUpEvent(Up);return true;
    }
    void Capture(UGameViewportClient* Client,const FString& Name)
    {
        TArray<FColor> Pixels;FIntVector Size;
        if(!FSlateApplication::Get().TakeScreenshot(Client->GetWindow().ToSharedRef(),Pixels,Size))return;
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("SaveUIReview");IFileManager::Get().MakeDirectory(*Folder,true);
        for(auto& Pixel:Pixels)Pixel.A=255;
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);FFileHelper::SaveArrayToFile(PNG,*(Folder/Name+TEXT(".png")));
    }
    void Next(int32 Value){Stage=Value;StageAt=FPlatformTime::Seconds();}
    void Cleanup(){if(auto* Instance=GI.Get())if(auto* Saves=Instance->GetSubsystem<UICSaveSubsystem>())if(!Saves->bSaving){Saves->DeleteSlot(0);Saves->DeleteSlot(1);}}
    FAutomationTestBase* Test;TWeakObjectPtr<UGameInstance> GI;
    int32 Stage=0,SizeIndex=0,SavedGold=0;double Started,StageAt=0;
    const FIntPoint Sizes[3]={{1280,720},{1280,800},{1680,720}};
};
class FICSaveUIRunner : public IAutomationLatentCommand
{
public:
    explicit FICSaveUIRunner(FAutomationTestBase* Test):Command(Test){}
    bool Update() override{return Command.FinishIfReady()||Command.Update();}
private:FICSaveUICommand Command;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICSaveUIInteractionCheck,"IC.UI.SaveSessionInteraction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICSaveUIInteractionCheck::RunTest(const FString& Parameters)
{
    if(GEditor->PlayWorld){AddError(TEXT("Close existing PIE before save UI test."));return false;}
    auto RestorePlayWindow=MakeShared<FICPlayWindowSettingsRestore>();
    if(!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/L_TestMap"),false,true))return false;
    auto Window=SNew(SWindow).ClientSize(FVector2D(1280,720)).UseOSWindowBorder(false).CreateTitleBar(false);FSlateApplication::Get().AddWindow(Window);
    FRequestPlaySessionParams Params;Params.CustomPIEWindow=Window;Params.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICSaveUIRunner>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    FAutomationTestFramework::Get().EnqueueLatentCommand(RestorePlayWindow);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));return true;
}
#endif
