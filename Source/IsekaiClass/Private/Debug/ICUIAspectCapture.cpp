#include "Debug/ICCharacterSetupLibrary.h"
#if WITH_EDITOR
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/PanelWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "UI/Settings/ICSettingsWidget.h"
#include "Slate/WidgetRenderer.h"
#include "UI/Framework/ICDisplayPolicy.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "Framework/Application/SlateApplication.h"
#endif

FString UICCharacterSetupLibrary::CaptureUIAspectSamples()
{
#if WITH_EDITOR
    if(!FSlateApplication::IsInitialized())return FString();
    UWorld* PreviousWorld=GWorld;
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("FixedAspectReview/Screens"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    auto* GI=NewObject<UGameInstance>(GEngine);GI->AddToRoot();GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=GEngine->GetWorldContextFromWorldChecked(World);
    auto* PreviewViewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=PreviewViewport;PreviewViewport->Init(Context,GI,false);
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();
    auto* Inventory=NewObject<UICInventoryComponent>();Inventory->BeginStaging(D,0);
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("화면비 확인");Initial.FirstSkill=TEXT("SKL_NPC_R01_002");Initial.Inventory=Inventory->GetSnapshot();
    FICCharacterState PlayerState;PlayerState.CharacterId=UICWorldStateSubsystem::PlayerId();PlayerState.Kind=EICCharacterKind::Player;
    PlayerState.UnlockedSkillIDs.Add(Initial.FirstSkill);Initial.Characters.Add(PlayerState.CharacterId,PlayerState);State->RestoreState(Initial);
    World->InitializeActorsForPlay(FURL(),false);
    auto* Player=World->SpawnActor<AICPlayerCharacter>();Player->Inventory->BindWorldState(State);Player->InitializeState(PlayerState.CharacterId);
    auto* PC=World->SpawnActor<AICPlayerController>();PC->Possess(Player);PC->PlayerName=Initial.PlayerName;PC->bCreated=true;
    auto* LocalPlayer=NewObject<ULocalPlayer>(GEngine);GI->AddLocalPlayer(LocalPlayer,FPlatformUserId::CreateFromInternalId(0));PC->SetPlayer(LocalPlayer);
    ensure(LocalPlayer->GetPlayerController(World)==PC);
    auto* NPC=World->SpawnActor<AICNPCBase>();NPC->InitializeNPC(TEXT("NPC_R01_009"));
    const FIntPoint Sizes[]={{1280,720},{1920,1080},{2560,1440},{1280,800},{1920,1200},{2560,1080},{3440,1440}};
    const FString Screens[]={TEXT("MainMenu"),TEXT("CharacterCreate"),TEXT("HUD"),TEXT("Pause"),TEXT("Dialogue"),TEXT("Settings")};
    FString CSV=TEXT("Screen,Width,Height,DPI,VisibleControls,OutsideControls,ImageSaved\n");
    for(const FString& Screen:Screens)
    {
        UClass* Type=nullptr;
        if(Screen==TEXT("Settings"))Type=UICSettingsWidget::StaticClass();
        else
        {
            const FString Asset=Screen==TEXT("MainMenu")?TEXT("WBP_MainMenu"):Screen==TEXT("CharacterCreate")?TEXT("WBP_CharacterCreate"):Screen==TEXT("Dialogue")?TEXT("WBP_IC_NPCDialogue"):TEXT("WBP_GameHUD");
            Type=LoadClass<UUserWidget>(nullptr,*(TEXT("/Game/IsekaiClass/UI/Widget/")+Asset+TEXT(".")+Asset+TEXT("_C")));
        }
        auto* Widget=Type?CreateWidget<UUserWidget>(PC,Type):nullptr;
        if(!Widget){CSV+=Screen+TEXT(",0,0,0,0,0,false\n");continue;}
        Widget->AddToRoot();Widget->SetVisibility(ESlateVisibility::Visible);
        const auto Slate=Widget->TakeWidget();
        PC->Menu=Screen==TEXT("Pause")?TEXT("Pause"):TEXT("");
        if(auto* HUD=Cast<UICGameplayWidget>(Widget))HUD->RefreshCurrentScreen();
        if(auto* Dialogue=Cast<UICNPCDialogueWidget>(Widget))Dialogue->OpenForNPC(NPC);
        for(const auto Size:Sizes)
        {
            const float DPI=GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(Size);
            const auto Rect=ICDisplayPolicy::ContentRect(Size);
            auto Root=SNew(SBorder).Padding(0).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor::Black).HAlign(HAlign_Center).VAlign(VAlign_Center)
                [SNew(SBox).WidthOverride(Rect.Width()).HeightOverride(Rect.Height())
                [SNew(SDPIScaler).DPIScale(DPI)[Slate]]];
            FWidgetRenderer Renderer(true,true);
            auto* Target=Renderer.DrawWidget(Root,FVector2D(Size));
            if(Target)Renderer.DrawWidget(Target,Root,FVector2D(Size),.1f);
            FlushRenderingCommands();
            TArray<FColor> Pixels;bool Saved=false;
            FReadSurfaceDataFlags ReadFlags;ReadFlags.SetLinearToGamma(false);
            if(Target&&Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,ReadFlags))
            {
                for(auto& Pixel:Pixels)Pixel.A=255;
                TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
                Saved=FFileHelper::SaveArrayToFile(PNG,*(Folder/FString::Printf(TEXT("%s_%dx%d.png"),*Screen,Size.X,Size.Y)));
            }
            TArray<UWidget*> Widgets;Widget->WidgetTree->GetAllWidgets(Widgets);int32 Controls=0,Outside=0;
            for(int32 Index=0;Index<Widgets.Num();++Index)
                if(auto* Nested=Cast<UUserWidget>(Widgets[Index]))if(Nested->WidgetTree)
                {TArray<UWidget*> Children;Nested->WidgetTree->GetAllWidgets(Children);for(auto* Child:Children)Widgets.AddUnique(Child);}
            for(auto* Child:Widgets)
            {
                if(!Child->IsA<UButton>()&&!Child->IsA<UTextBlock>())continue;
                bool Visible=Child->IsVisible(),Scrolled=false;
                for(auto* Parent=Child->GetParent();Parent;Parent=Parent->GetParent()){Visible&=Parent->IsVisible();Scrolled|=Parent->IsA<UScrollBox>();}
                const auto Geometry=Child->GetCachedGeometry();const auto Extent=Geometry.GetAbsoluteSize();
                if(!Visible||Scrolled||Extent.X<1||Extent.Y<1)continue;
                ++Controls;const auto Position=Geometry.GetAbsolutePosition();
                if(Position.X < Rect.Min.X-1 || Position.Y < Rect.Min.Y-1 || Position.X+Extent.X>Rect.Max.X+1 || Position.Y+Extent.Y>Rect.Max.Y+1)++Outside;
            }
            CSV+=FString::Printf(TEXT("%s,%d,%d,%.4f,%d,%d,%s\n"),*Screen,Size.X,Size.Y,DPI,Controls,Outside,Saved?TEXT("true"):TEXT("false"));
        }
        Widget->RemoveFromRoot();Widget->ReleaseSlateResources(true);
    }
    const FString Report=Folder/TEXT("LayoutChecks.csv");FFileHelper::SaveStringToFile(CSV,*Report);
    Player->ReleaseForLoad();NPC->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);GI->RemoveFromRoot();GWorld=PreviousWorld;
    return Report;
#else
    return FString();
#endif
}
