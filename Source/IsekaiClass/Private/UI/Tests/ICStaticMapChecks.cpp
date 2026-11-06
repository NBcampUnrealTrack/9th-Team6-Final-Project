#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/Map/ICMapWidget.h"
#include "UI/Map/ICMapTextureImage.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "AssetCompilingManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICStaticMapProjectionCheck,"IC.UI.Map.StaticOpenWorld",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICStaticMapProjectionCheck::RunTest(const FString&)
{
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/IsekaiClass/UI/Map/DT_IC_LevelMap.DT_IC_LevelMap"));
    if(!TestNotNull(TEXT("Level map table is packaged as a UI asset"),Table))return false;
    const auto* Row=Table->FindRow<FICMapRow>(TEXT("L_OpenWorld"),TEXT("Map test"));
    if(!TestNotNull(TEXT("OpenWorld projection exists"),Row)||!TestEqual(TEXT("One captured layer"),Row->Layers.Num(),1))return false;
    auto* Texture=Row->Layers[0].MapTexture.LoadSynchronous();
    if(!TestNotNull(TEXT("Captured texture loads"),Texture))return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    TestEqual(TEXT("Capture width"),Texture->GetSizeX(),3072);
    TestEqual(TEXT("Capture height"),Texture->GetSizeY(),3072);

    // A PIE name must select the same projection as a packaged L_OpenWorld level.
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,FName(TEXT("UEDPIE_0_L_OpenWorld")),CreatePackage(TEXT("/Temp/UIMapReview/UEDPIE_0_L_OpenWorld")));
    World->StreamingLevelsPrefix=TEXT("UEDPIE_0_");
    auto* Map=NewObject<UICMapWidget>(World);
    Map->bMinimap=true;Map->RefreshMap();
    TestEqual(TEXT("PIE selects the level map"),Map->ActiveLevelMap,FName(TEXT("L_OpenWorld")));
    TestEqual(TEXT("Runtime selects the captured texture"),Map->MapTexture.Get(),Texture);
    TestTrue(TEXT("Capture centre is at image centre"),Map->WorldToMapUV({-56000,610000,0}).Equals({.5,.5},1.e-6));
    TestTrue(TEXT("Top-left is +X and -Y"),Map->WorldToMapUV({-23500,577500,0}).Equals({0,0},1.e-6));
    TestTrue(TEXT("Bottom-right is -X and +Y"),Map->WorldToMapUV({-88500,642500,0}).Equals({1,1},1.e-6));
    TestTrue(TEXT("Elevation does not shift the map"),Map->WorldToMapUV({-56000,610000,10000}).Equals({.5,.5},1.e-6));
    const FVector2D PlayerUV=Map->WorldToMapUV({-56000,610000,0});
    Map->MinimapCenterUV=PlayerUV;
    TestTrue(TEXT("Moving player stays centred"),Map->DisplayUV({-56000,610000,0}).Equals({.5,.5},1.e-6));
    TestTrue(TEXT("Positive Y moves right by matching image zoom"),Map->DisplayUV({-56000,611000,0}).Equals({.5+4000./65000.,.5},1.e-6));
    Map->WidgetTree=NewObject<UWidgetTree>(Map);
    UICMapWidget::BuildLayout(Map->WidgetTree,true);
    Map->RefreshVisuals(FGeometry());
    auto* Image=Cast<UICMapTextureImage>(Map->GetWidgetFromName(TEXT("IMG_Map")));
    if(TestNotNull(TEXT("Minimap image exists"),Image))
    {
        Image->bFadeEdges=true;Image->SynchronizeProperties();
        Map->RefreshVisuals(FGeometry());
        TestFalse(TEXT("Saved legacy fade setting is disabled at runtime"),Image->bFadeEdges);
        TestEqual(TEXT("Minimap image is visible"),Image->GetVisibility(),ESlateVisibility::HitTestInvisible);
        TestEqual(TEXT("Minimap uses the captured texture"),Image->Texture.Get(),Texture);
        TestTrue(TEXT("Image samples around the same player centre"),((Image->UVMin+Image->UVMax)*.5).Equals(PlayerUV,1.e-6));
    }
    auto* WarpList=NewObject<UICMapWidget>(World);
    WarpList->WidgetTree=NewObject<UWidgetTree>(WarpList);
    UICMapWidget::BuildLayout(WarpList->WidgetTree,false);
    TestNotNull(TEXT("Dev warp destinations remain available"),WarpList->GetWidgetFromName(TEXT("WarpDestinations")));
    TestNotNull(TEXT("Dev warp confirmation remains available"),WarpList->GetWidgetFromName(TEXT("WarpConfirm")));
    // Render the actual UMG map at a known viewpoint without changing any saved level.
    if(FSlateApplication::IsInitialized()&&!GUsingNullRHI)
    {
        FAssetCompilingManager::Get().FinishAllCompilation();
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("OpenWorldMapReview");
        IFileManager::Get().MakeDirectory(*Folder,true);
        auto Render=[&](bool Mini,FIntPoint Size,const TCHAR* Filename)
        {
            auto* Preview=NewObject<UICMapWidget>(World);
            Preview->bMinimap=Mini;Preview->Initialize();
            UICMapWidget::BuildLayout(Preview->WidgetTree,Mini);
            Preview->MinimapCenterUV=PlayerUV;
            Preview->RefreshMap();Preview->RefreshLabels();
            auto Surface=Preview->TakeWidget();
            FWidgetRenderer Renderer(true,true);
            auto* Target=Renderer.DrawWidget(Surface,FVector2D(Size));
            if(!Target)return false;
            Preview->RefreshVisuals(Preview->GetPaintSpaceGeometry());
            Renderer.DrawWidget(Target,Surface,FVector2D(Size),.1f);
            FlushRenderingCommands();
            TArray<FColor> Pixels;FReadSurfaceDataFlags Flags;Flags.SetLinearToGamma(false);
            if(!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))return false;
            for(auto& Pixel:Pixels)Pixel.A=255;
            TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
            const bool Saved=FFileHelper::SaveArrayToFile(PNG,*(Folder/Filename));
            Preview->ReleaseSlateResources(true);
            return Saved;
        };
        TestTrue(TEXT("Render dev warp list preview"),Render(false,{1920,1080},TEXT("WarpList.png")));
        TestTrue(TEXT("Render village minimap preview"),Render(true,{320,320},TEXT("Minimap.png")));
    }
    World->DestroyWorld(false);

    FVector2D LocalMin,LocalMax;
    TestTrue(TEXT("Partially off-map region still renders"),UICMapTextureImage::ClipUVRegion({-.1,.4},{.1,.6},LocalMin,LocalMax));
    TestTrue(TEXT("Left half stays empty rather than stretching the texture"),LocalMin.Equals({.5,0})&&LocalMax.Equals({1,1}));
    TestTrue(TEXT("Bottom-right edge clips at its exact local position"),UICMapTextureImage::ClipUVRegion({.9,.9},{1.1,1.1},LocalMin,LocalMax)&&LocalMin.Equals({0,0})&&LocalMax.Equals({.5,.5}));
    TestFalse(TEXT("Completely off-map region draws no repeated image"),UICMapTextureImage::ClipUVRegion({1.1,.2},{1.3,.4},LocalMin,LocalMax));
    TestFalse(TEXT("Zero-size UV window is rejected"),UICMapTextureImage::ClipUVRegion({.5,.5},{.5,.5},LocalMin,LocalMax));
    return !HasAnyErrors();
}
#endif
