#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "PlayInEditorDataTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/UserInterfaceSettings.h"
#include "SceneView.h"
#include "Slate/SceneViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Core/Controller/ICPlayerController.h"
#include "UI/MainMenu/ICMainMenuWidget.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "CommonGameViewportClient.h"
#include "UI/Tests/ICPlayWindowSettingsRestore.h"
#include "UI/CharacterCreate/ICCharacterCreateWidget.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
class FICAspectCommand : public IAutomationLatentCommand
{
public:
    FICAspectCommand(FAutomationTestBase* InTest):Test(InTest),Started(FPlatformTime::Seconds()){}
    bool Update() override
    {
        if(FPlatformTime::Seconds()-Started>60){Test->AddError(TEXT("Aspect resize/click check timed out"));return true;}
        UWorld* World=GEditor->PlayWorld;
        auto* PC=World?Cast<AICPlayerController>(World->GetFirstPlayerController()):nullptr;
        UGameViewportClient* Client=PC&&PC->GetLocalPlayer()?PC->GetLocalPlayer()->ViewportClient.Get():nullptr;
        auto* Viewport=Client?Client->GetGameViewport():nullptr;
        if(!PC||!Viewport||!Client->GetWindow())return false;
        TArray<UUserWidget*> Widgets;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,UICMainMenuWidget::StaticClass(),false);
        UICMainMenuWidget* Menu=nullptr;
        for(auto* Widget:Widgets)if(auto* Candidate=Cast<UICMainMenuWidget>(Widget))if(Candidate->IsActivated())Menu=Candidate;
        if(Stage==0)
        {
            if(!Menu)return false;
            Test->TestTrue(TEXT("Standard CommonUI viewport loaded"),Client->GetClass()==UCommonGameViewportClient::StaticClass());
            auto Window=Client->GetWindow();
            const FVector2D Border=FVector2D(Window->GetClientSizeInScreen())-FVector2D(Viewport->GetSizeXY());
            Window->Resize(FVector2D(Sizes[Index])+Border);
            Window->BringToFront(true);
            FSlateApplication::Get().ProcessWindowActivatedEvent(FWindowActivateEvent(FWindowActivateEvent::EA_Activate,Window.ToSharedRef()));
            FSlateApplication::Get().ReleaseAllPointerCapture();
            Stage=1;StageAt=FPlatformTime::Seconds();return false;
        }
        if(FPlatformTime::Seconds()-StageAt<.3)return false;
        if(Stage==1)
        {
            if(!Menu)return false;
            auto* Button=Cast<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Start")));
            if(!Button)return false;
            const FIntPoint Size=Viewport->GetSizeXY();
            // PIE can add a 3px Slate frame on each edge; all projection and hit tests below use actual viewport pixels.
            Test->TestTrue(TEXT("Actual PIE viewport follows the resized window"),FMath::Abs(Size.X-Sizes[Index].X)<=8&&FMath::Abs(Size.Y-Sizes[Index].Y)<=8);
            FSceneViewProjectionData Projection;
            if(!Test->TestTrue(TEXT("Actual local-player projection available"),PC->GetLocalPlayer()->GetProjectionData(Viewport,Projection)))return true;
            const auto Rect=ICDisplayPolicy::ContentRect(Size);
            const auto GameRect=Projection.GetConstrainedViewRect();
            Test->TestTrue(TEXT("Game view shares the UI aspect"),FVector2D(GameRect.Min).Equals(FVector2D(Rect.Min),1)&&FVector2D(GameRect.Max).Equals(FVector2D(Rect.Max),1));
            const auto& POV=PC->PlayerCameraManager->GetCameraCacheView();
            Test->AddInfo(FString::Printf(TEXT("Viewport=%dx%d Rect=%s Camera=%s Aspect=%.6f Constrained=%d"),Size.X,Size.Y,*Rect.ToString(),*PC->PlayerCameraManager->GetClass()->GetName(),POV.AspectRatio,POV.bConstrainAspectRatio));
            const auto RootGeometry=Menu->GetCachedGeometry();
            const auto ViewGeometry=Viewport->GetCachedGeometry();
            const FVector2D PixelScale=FVector2D(Size)/FVector2D(ViewGeometry.GetLocalSize());
            const auto RootMin=FVector2D(ViewGeometry.AbsoluteToLocal(RootGeometry.GetAbsolutePosition()))*PixelScale;
            const auto RootMax=FVector2D(ViewGeometry.AbsoluteToLocal(RootGeometry.LocalToAbsolute(RootGeometry.GetLocalSize())))*PixelScale;
            Test->AddInfo(FString::Printf(TEXT("UI min=%s max=%s"),*RootMin.ToString(),*RootMax.ToString()));
            Test->TestTrue(TEXT("WBP ScaleBox fits the 1920x1080 UI after resize"),RootMin.Equals(FVector2D(Rect.Min),2)&&RootMax.Equals(FVector2D(Rect.Max),2));
            const auto Geometry=Button->GetCachedGeometry();
            const auto Center=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
            const auto Local=FVector2D(ViewGeometry.AbsoluteToLocal(Center))*PixelScale;
            Test->TestTrue(TEXT("Button lies inside game view"),Rect.Contains(FIntPoint(Local.X,Local.Y)));
            TArray<FColor> Pixels;FIntVector ImageSize;
            if(FSlateApplication::Get().TakeScreenshot(Client->GetWindow().ToSharedRef(),Pixels,ImageSize))
            {
                const FString Folder=FPaths::ProjectSavedDir()/TEXT("FixedAspectReview");IFileManager::Get().MakeDirectory(*Folder,true);
                for(auto& Pixel:Pixels)Pixel.A=255;
                TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(ImageSize.X,ImageSize.Y,Pixels,PNG);
                FFileHelper::SaveArrayToFile(PNG,*(Folder/FString::Printf(TEXT("Title_%d_%dx%d.png"),Index,Size.X,Size.Y)));
            }
            // Hit testing and real pointer routing, without broadcasting UButton delegates.
            if(Rect.Min.X>0||Rect.Min.Y>0)
            {
                Click(Client->GetWindow(),ViewGeometry.LocalToAbsolute(FVector2D(2,2)/PixelScale));
                Test->TestTrue(TEXT("Black margin does not activate the start button"),PC->Menu!=TEXT("Create"));
            }
            Click(Client->GetWindow(),Center);
            Stage=2;StageAt=FPlatformTime::Seconds();return false;
        }
        Test->TestTrue(TEXT("Real mouse coordinates activate the visible button after resize"),PC->Menu==TEXT("Create")&&PC->CharacterCreateScreen!=nullptr);
        PC->SetMenu(TEXT("MainMenu"));
        if(++Index==UE_ARRAY_COUNT(Sizes))return true;
        Stage=0;return false;
    }
private:
    void Click(TSharedPtr<SWindow> Window,FVector2D Position)
    {
        auto& App=FSlateApplication::Get();App.SetCursorPos(Position);
        FPointerEvent Move(0,Position,Position,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState());
        App.ProcessMouseMoveEvent(Move);
        FPointerEvent Down(0,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
        App.ProcessMouseButtonDownEvent(Window->GetNativeWindow(),Down);
        FPointerEvent Up(0,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
        App.ProcessMouseButtonUpEvent(Up);
    }
    FAutomationTestBase* Test;
    double Started,StageAt=0;
    int32 Index=0,Stage=0;
    const FIntPoint Sizes[5]={{1280,720},{1280,800},{1680,720},{800,1000},{1280,720}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFixedAspectCheck,"IC.UI.ScaleBoxResizeAndClick",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICFixedAspectCheck::RunTest(const FString& Parameters)
{
    if(GEditor->PlayWorld){AddError(TEXT("Close the existing PIE session."));return false;}
    auto RestorePlayWindow=MakeShared<FICPlayWindowSettingsRestore>();
    if(!FEditorFileUtils::LoadMap(TEXT("/Game/IsekaiClass/World/Maps/L_Title"),false,true))return false;
    auto Window=SNew(SWindow).ClientSize(FVector2D(1280,720)).UseOSWindowBorder(false).CreateTitleBar(false);
    FSlateApplication::Get().AddWindow(Window);
    FRequestPlaySessionParams Params;Params.CustomPIEWindow=Window;Params.bAllowOnlineSubsystem=false;
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICAspectCommand>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    FAutomationTestFramework::Get().EnqueueLatentCommand(RestorePlayWindow);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
    return true;
}
#endif
