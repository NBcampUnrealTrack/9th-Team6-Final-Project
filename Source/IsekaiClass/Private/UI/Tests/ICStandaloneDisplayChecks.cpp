#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SceneViewport.h"
#include "SceneView.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Core/Controller/ICPlayerController.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "UI/MainMenu/ICMainMenuWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "RHIResources.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
class FICStandaloneResizeCommand : public IAutomationLatentCommand
{
public:
    explicit FICStandaloneResizeCommand(FAutomationTestBase* InTest)
        : Test(InTest), Started(FPlatformTime::Seconds()) {}

    bool Update() override
    {
        if(FPlatformTime::Seconds()-Started>60)
        {
            Test->AddError(TEXT("Standalone resize check timed out"));return true;
        }
        auto* Client=GEngine?GEngine->GameViewport.Get():nullptr;
        auto* Viewport=Client?Client->GetGameViewport():nullptr;
        auto* World=Client?Client->GetWorld():nullptr;
        auto* PC=World?Cast<AICPlayerController>(World->GetFirstPlayerController()):nullptr;
        if(!Viewport||!PC||!PC->GetLocalPlayer()||!Client->GetWindow())return false;

        if(Stage==0)
        {
            Test->TestEqual(TEXT("Test uses the standalone game world"),World->WorldType,EWorldType::Game);
            auto Window=Client->GetWindow();
            Window->SetWindowMode(Index==10?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
            if(Index==8)Window->Maximize();
            else if(Index!=10)
            {
                Window->Restore();
                Window->Resize(FVector2D(Sizes[Index]));
            }
            Window->BringToFront(true);
            FSlateApplication::Get().ProcessWindowActivatedEvent(FWindowActivateEvent(FWindowActivateEvent::EA_Activate,Window.ToSharedRef()));
            Stage=1;StageAt=FPlatformTime::Seconds();return false;
        }
        if(FPlatformTime::Seconds()-StageAt<.3)return false;

        FSceneViewProjectionData Projection;
        if(!PC->GetLocalPlayer()->GetProjectionData(Viewport,Projection))return false;
        const auto Size=Viewport->GetSizeXY();
        const auto Rect=Projection.GetConstrainedViewRect();
        const auto Expected=ICDisplayPolicy::ContentRect(Size);
        Test->AddInfo(FString::Printf(TEXT("Standalone resize %d: viewport=%dx%d rect=%s"),
            Index,Size.X,Size.Y,*Rect.ToString()));
        Test->TestTrue(TEXT("Projection remains inside the resized viewport"),
            Rect.Min.X>=0&&Rect.Min.Y>=0&&Rect.Max.X<=Size.X&&Rect.Max.Y<=Size.Y);
        Test->TestTrue(TEXT("Game view follows the shared 1920x1080 aspect"),
            FVector2D(Rect.Min).Equals(FVector2D(Expected.Min),1)&&FVector2D(Rect.Max).Equals(FVector2D(Expected.Max),1));

        TArray<UUserWidget*> Widgets;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,UICMainMenuWidget::StaticClass(),false);
        UICMainMenuWidget* Menu=nullptr;
        for(auto* Widget:Widgets)if(auto* Candidate=Cast<UICMainMenuWidget>(Widget))if(Candidate->IsActivated())Menu=Candidate;
        if(!Menu)return false;
        const auto VG=Viewport->GetCachedGeometry();const auto UI=Menu->GetCachedGeometry();
        const FVector2D PixelScale=FVector2D(Size)/FVector2D(VG.GetLocalSize());
        const auto Min=FVector2D(VG.AbsoluteToLocal(UI.GetAbsolutePosition()))*PixelScale;
        const auto Max=FVector2D(VG.AbsoluteToLocal(UI.LocalToAbsolute(UI.GetLocalSize())))*PixelScale;
        Test->AddInfo(FString::Printf(TEXT("UI min=%s max=%s expected=%s"),*Min.ToString(),*Max.ToString(),*Expected.ToString()));
        Test->TestTrue(TEXT("Standalone WBP scales its 1920x1080 design area"),
            Min.Equals(FVector2D(Expected.Min),2)&&Max.Equals(FVector2D(Expected.Max),2));

        TArray<FColor> Pixels;FIntVector ImageSize;
        auto Window=Client->GetWindow();
        if(FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(),Pixels,ImageSize)&&Pixels.Num()==ImageSize.X*ImageSize.Y)
        {
            const auto WG=Window->GetWindowGeometryInScreen();
            auto Black=[&](int32 X,int32 Y)
            {
                const auto Local=WG.AbsoluteToLocal(VG.LocalToAbsolute(FVector2D(X,Y)/PixelScale));
                const auto P=FVector2D(Local)/FVector2D(WG.GetLocalSize())*FVector2D(ImageSize.X,ImageSize.Y);
                const int32 SX=FMath::Clamp(FMath::RoundToInt(P.X),0,ImageSize.X-1),SY=FMath::Clamp(FMath::RoundToInt(P.Y),0,ImageSize.Y-1);
                const auto C=Pixels[SY*ImageSize.X+SX];return C.R<=3&&C.G<=3&&C.B<=3;
            };
            if(Expected.Min.X>1)Test->TestTrue(TEXT("Side letterbox is black"),Black(Expected.Min.X/2,Size.Y/2));
            if(Expected.Min.Y>1)Test->TestTrue(TEXT("Top letterbox is black"),Black(Size.X/2,Expected.Min.Y/2));
            if(Index==0||Index==4||Index==10||Index==11)
            {
                const FString Folder=FPaths::ProjectSavedDir()/TEXT("DisplayRootReview");IFileManager::Get().MakeDirectory(*Folder,true);
                for(auto& Pixel:Pixels)Pixel.A=255;
                TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(ImageSize.X,ImageSize.Y,Pixels,PNG);
                FFileHelper::SaveArrayToFile(PNG,*(Folder/FString::Printf(TEXT("Standalone_%d_%dx%d.png"),Index,Size.X,Size.Y)));
            }
        }
        else Test->AddError(TEXT("Could not read the rendered game viewport"));

        if(++Index<UE_ARRAY_COUNT(Sizes)){Stage=0;return false;}
        auto* Button=Cast<UButton>(Menu->GetWidgetFromName(TEXT("BTN_Start")));
        if(!Button){Test->AddError(TEXT("Standalone start button missing"));return true;}
        const auto G=Button->GetCachedGeometry();
        const FVector2D Position=G.LocalToAbsolute(G.GetLocalSize()*.5f);
        auto& App=FSlateApplication::Get();App.ReleaseAllPointerCapture();App.SetCursorPos(Position);
        FPointerEvent Move(0,Position,Position,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState());App.ProcessMouseMoveEvent(Move);
        FPointerEvent Down(0,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
        App.ProcessMouseButtonDownEvent(Client->GetWindow()->GetNativeWindow(),Down);
        FPointerEvent Up(0,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());App.ProcessMouseButtonUpEvent(Up);
        Test->TestEqual(TEXT("Standalone pointer click activates the visible button after resize"),PC->Menu,FString(TEXT("Create")));
        PC->SetMenu(TEXT("MainMenu"));Client->GetWindow()->EnableWindow(true);return true;
    }
private:
    FAutomationTestBase* Test;
    double Started,StageAt=0;
    int32 Index=0,Stage=0;
    const FIntPoint Sizes[12]={{1680,720},{1576,675},{1468,629},{1280,720},{1280,800},{800,1000},
        {1600,900},{1680,720},{1576,675},{1468,629},{1280,800},{1920,1080}};
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICStandaloneResizeCheck,"IC.Display.StandaloneResize",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FICStandaloneResizeCheck::RunTest(const FString& Parameters)
{
    if(auto* Client=GEngine?GEngine->GameViewport.Get():nullptr)
    {
        if(auto Window=Client->GetWindow())Window->EnableWindow(false);
        if(auto* World=Client->GetWorld())if(auto* PC=Cast<AICPlayerController>(World->GetFirstPlayerController()))
        {PC->FlushPressedKeys();PC->SetMenu(TEXT("MainMenu"));}
    }
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FICStandaloneResizeCommand>(this));
    return true;
}
#endif
