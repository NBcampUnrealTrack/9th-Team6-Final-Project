#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Enemy/Character/ICEnemyBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "World/Warp/ICPartyWarpComponent.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "UI/Gameplay/ICGameplayWidget.h"
#include "UI/Gameplay/ICControlGuide.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/BoxComponent.h"
#include "InputKeyEventArgs.h"
#include "AI/ICAIController.h"
#include "Time/ICTimeSubsystem.h"
#include "Settings/ICGameUserSettings.h"
#include "UI/Map/ICMapWidget.h"
#include "UI/Map/ICMapTextureImage.h"
#include "UI/Management/ICUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DataTable.h"
#include "AssetCompilingManager.h"
#include "UObject/Package.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "World/Warp/ICWarpStone.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/WidgetComponent.h"
#include "UI/Gameplay/ICCombatFeedbackWidget.h"
#include "Data/Rows/World/ICAnchorRow.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"
#include "RenderingThread.h"
#include "RHI.h"

namespace
{
bool CaptureAdventureHUD(UICGameplayWidget* HUD,const TCHAR* Filename)
{
    if(GUsingNullRHI)return true;
    FWidgetRenderer Renderer(true);
    auto Surface=SNew(SBorder).Padding(0).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(.035f,.055f,.075f))[HUD->TakeWidget()];
    auto* Target=Renderer.DrawWidget(Surface,FVector2D(1920,1080));if(!Target)return false;
    Renderer.DrawWidget(Target,Surface,FVector2D(1920,1080),.1f);FlushRenderingCommands();
    TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);
    if(!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))return false;
    TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1920,1080,Pixels,PNG);
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("RequestedHUDWarps/Preview");IFileManager::Get().MakeDirectory(*Directory,true);
    return FFileHelper::SaveArrayToFile(PNG,*(Directory/Filename));
}

struct FOpenWorldUIFixture
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);
    UWorld* World=nullptr;
    AICPlayerController* PC=nullptr;
    AICPlayerCharacter* Player=nullptr;
    UICWorldStateSubsystem* State=nullptr;
    UICDefinitionSubsystem* Definitions=nullptr;
    FOpenWorldUIFixture()
    {
        GI->InitializeStandalone();World=GI->GetWorld();
        auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
        Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
        PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
        Player=World->SpawnActor<AICPlayerCharacter>();Player->EnableCombat(true,true);PC->Possess(Player);
        State=GI->GetSubsystem<UICWorldStateSubsystem>();Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();
        FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("워프검증");Initial.Gold=2000;Initial.FirstSkill=TEXT("SKL_NPC_R01_002");
        FICCharacterState P;P.CharacterId=UICWorldStateSubsystem::PlayerId();P.Kind=EICCharacterKind::Player;Initial.Characters.Add(P.CharacterId,P);
        State->RestoreState(Initial);Player->InitializeState(P.CharacterId);Player->Inventory->BindWorldState(State);
        PC->bCreated=true;PC->Menu=TEXT("");
    }
    UICGameplayWidget* HUD()
    {
        auto* Class=LoadClass<UICGameplayWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_GameHUD.WBP_GameHUD_C"));
        PC->Screen=Class?CreateWidget<UICGameplayWidget>(PC,Class):nullptr;return PC->Screen;
    }
    ~FOpenWorldUIFixture()
    {
        if(PC->Screen){PC->Screen->ReleaseSlateResources(true);PC->Screen=nullptr;}
        Player->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICOpenWorldGuideCheck,"IC.OpenWorld.GuideInputAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICOpenWorldGuideCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;auto* HUD=F.HUD();if(!TestNotNull(TEXT("Saved HUD class loads"),HUD))return false;
    HUD->RefreshControlGuide();TestFalse(TEXT("Existing saves do not start a new-game guide"),HUD->IsControlGuideVisible());
    auto Next=F.State->ExportState();Next.Flags.Add(TEXT("TUT_CONTROLS_PENDING"),true);F.State->ApplyWorldProgress(Next);HUD->RefreshControlGuide();
    TestTrue(TEXT("Pending new-game guide appears on gameplay HUD"),HUD->IsControlGuideVisible());
    TestTrue(TEXT("Guide HUD renders for visual review"),CaptureAdventureHUD(HUD,TEXT("ControlGuide.png")));
    auto* Title=Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("TXT_GuideTitle")));if(!TestNotNull(TEXT("Guide title exists"),Title))return false;
    const FText Before=Title->GetText();
    F.PC->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),EKeys::F,IE_Pressed,0));
    TestNotEqual(TEXT("F advances one guide page through the player input route"),Title->GetText().ToString(),Before.ToString());
    F.PC->Menu=TEXT("Inventory");TestFalse(TEXT("Guide is hidden while a menu is open"),HUD->IsControlGuideVisible());TestFalse(TEXT("Hidden guide cannot consume a page"),HUD->AdvanceControlGuide());F.PC->Menu=TEXT("");
    for(int32 I=1;I<ICControlGuide::Pages(F.PC).Num();++I)HUD->AdvanceControlGuide();
    TestFalse(TEXT("Last page closes the guide"),HUD->IsControlGuideVisible());
    TestTrue(TEXT("Completion is persisted in world state"),F.State->ExportState().Flags.FindRef(TEXT("TUT_CONTROLS_DONE")));
    auto* Reloaded=F.HUD();Reloaded->RefreshControlGuide();TestFalse(TEXT("Recreated HUD does not restart the completed guide"),Reloaded->IsControlGuideVisible());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICOpenWorldWarpCheck,"IC.OpenWorld.WarpUnlockAndCharge",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICOpenWorldWarpCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;auto* Warp=F.Player->PartyWarp.Get();const FName Home=TEXT("WRP_OW_01_HOME"),Inn=TEXT("WRP_OW_02_INN");FString Reason;
    TestTrue(TEXT("Home is unlocked without visiting"),Warp->IsWarpUnlocked(Home));TestFalse(TEXT("Inn starts locked"),Warp->IsWarpUnlocked(Inn));
    const auto Before=F.Player->GetActorLocation();TestFalse(TEXT("Locked destination rejects travel"),Warp->WarpParty(Inn,500,Reason));
    TestEqual(TEXT("Rejected travel does not charge gold"),F.State->ExportState().Gold,2000);TestEqual(TEXT("Rejected travel does not move player"),F.Player->GetActorLocation(),Before);
    TestFalse(TEXT("Home still requires its loaded destination anchor"),Warp->CanWarp(Home,500,Reason));
    auto* Anchor=F.World->SpawnActor<AICWorldAnchor>();Anchor->AnchorID=TEXT("ANC_WRP_OW_ARRIVAL_01_HOME");Anchor->SetActorLocation(FVector(500,100,200));
    TestTrue(TEXT("Arrival anchor registers against authored DT"),F.World->GetSubsystem<UICSpawnSubsystem>()->RegisterAnchor(Anchor));
    TestTrue(TEXT("Home travel succeeds once its anchor is ready"),Warp->WarpParty(Home,500,Reason));
    TestTrue(TEXT("Player arrives at the anchor rather than at stone origin"),F.Player->GetActorLocation().Equals(Anchor->GetActorLocation(),1.f));
    TestEqual(TEXT("Successful travel charges once"),F.State->ExportState().Gold,1500);
    TestFalse(TEXT("Remote activation is rejected"),Warp->UnlockWarp(Inn,F.Player->GetActorLocation()+FVector(1000,0,0),Reason));
    TestTrue(TEXT("Nearby stone interaction unlocks the inn"),Warp->UnlockWarp(Inn,F.Player->GetActorLocation(),Reason));
    TestTrue(TEXT("Inn unlock persists in world progress"),F.State->ExportState().UnlockedWarps.Contains(Inn));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICOpenWorldSlopeCheck,"IC.OpenWorld.CompanionArrivalOnSlope",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICOpenWorldSlopeCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;
    auto* Ground=F.World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Ground);
    Ground->SetRootComponent(Box);Ground->AddInstanceComponent(Box);Box->SetBoxExtent(FVector(3000,3000,20));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
    Ground->SetActorLocationAndRotation(FVector(0,0,-25),FRotator(24,0,13));F.Player->SetActorLocation(FVector(0,0,100));
    FTransform Placement;
    TestTrue(TEXT("Walkable sloped ground can receive a companion"),F.World->GetSubsystem<UICSpawnSubsystem>()->FindSafePartyPlacement(TEXT("NPC_R01_001"),Placement));
    FCollisionQueryParams Query;Query.AddIgnoredActor(F.Player);
    TestFalse(TEXT("Arrival capsule clears the slope rather than penetrating it"),F.World->OverlapBlockingTestByChannel(Placement.GetLocation(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,90),Query));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICOpenWorldBossCheck,"IC.OpenWorld.BossHUDAndAttackAudio",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICOpenWorldBossCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;auto* HUD=F.HUD();if(!TestNotNull(TEXT("HUD loads"),HUD))return false;
    TestNotNull(TEXT("Long boss health bar is authored"),HUD->GetWidgetFromName(TEXT("BAR_BossHP")));
    TestNotNull(TEXT("Guide is embedded in the gameplay HUD"),HUD->GetWidgetFromName(TEXT("HUD_ControlGuide")));
    TArray<FName> Monsters;F.Definitions->GetDefinitionIDs(EICDefinitionType::Monster,Monsters);FName BossID;
    for(FName ID:Monsters){FICDefinitionResult R;const auto* Row=F.Definitions->Find<FICMonsterRow>(ID,R);if(Row&&Row->Kind==EICMonsterKind::RegionalBoss){BossID=ID;break;}}
    if(!TestFalse(TEXT("A regional boss exists in the authored DT"),BossID.IsNone()))return false;
    auto* Boss=F.World->SpawnActor<AICEnemyBase>();Boss->SetActorLocation(F.Player->GetActorLocation()+FVector(300,0,0));FICSpawnSlotDefinition Slot;Slot.MonsterID=BossID;
    if(TestTrue(TEXT("Regional boss initializes from the existing DT"),Boss->InitializeEnemy(TEXT("PRESENTATION_BOSS"),Slot,Boss->GetActorTransform())))
    {
        F.PC->NotifyPlayerHitEnemy(Boss);HUD->RefreshTargetInfo();TestEqual(TEXT("Boss target resolves to the regional boss"),F.PC->GetHUDBossTarget(),Boss);
        TestEqual(TEXT("Boss panel appears while fighting"),HUD->GetWidgetFromName(TEXT("HUD_BossBar"))->GetVisibility(),ESlateVisibility::HitTestInvisible);
        Boss->GetICAbilitySystem()->ChangeResource(TEXT("HP"),-10);HUD->RefreshTargetInfo();
        TestTrue(TEXT("Boss bar follows actual HP loss"),CastChecked<UProgressBar>(HUD->GetWidgetFromName(TEXT("BAR_BossHP")))->GetPercent()<1.f);
        TestTrue(TEXT("Boss HP HUD renders for visual review"),CaptureAdventureHUD(HUD,TEXT("BossHP.png")));
        Boss->BeginReturn();HUD->RefreshTargetInfo();TestEqual(TEXT("Returning boss hides the fight bar"),HUD->GetWidgetFromName(TEXT("HUD_BossBar"))->GetVisibility(),ESlateVisibility::Collapsed);
    }
    TArray<FName> Classes;F.Definitions->GetDefinitionIDs(EICDefinitionType::Class,Classes);int32 AudioStages=0;
    for(FName ID:Classes){FICDefinitionResult R;if(const auto* Row=F.Definitions->Find<FICClassRow>(ID,R))for(const auto& Stage:Row->NormalAttackStages){TestNotNull(TEXT("Basic attack stage has a real sound asset"),Stage.AttackSound.LoadSynchronous());++AudioStages;}}
    TestTrue(TEXT("All four weapon families have three audible attack stages"),AudioStages>=12);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMonsterEngagementBarCheck,"IC.OpenWorld.MonsterHealthOnHitOrChase",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMonsterEngagementBarCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;auto* Settings=UICGameUserSettings::Get();const bool Before=Settings->bEnemyHealthBars;Settings->bEnemyHealthBars=true;
    auto* Time=F.GI->GetSubsystem<UICTimeSubsystem>();Time->SetRunning(true);Time->SetMenuPaused(false);
    TArray<FName> IDs;F.Definitions->GetDefinitionIDs(EICDefinitionType::Monster,IDs);bool Normal=false,Boss=false;
    for(FName ID:IDs)
    {
        FICDefinitionResult R;const auto* Row=F.Definitions->Find<FICMonsterRow>(ID,R);if(!Row)continue;
        const bool IsBoss=Row->Kind!=EICMonsterKind::Normal;if(IsBoss?Boss:Normal)continue;
        if(IsBoss)Boss=true;else Normal=true;
        auto* Enemy=F.World->SpawnActor<AICEnemyBase>();FICSpawnSlotDefinition Slot;Slot.MonsterID=ID;
        if(!TestTrue(TEXT("Monster initializes"),Enemy->InitializeEnemy(FName(*FString::Printf(TEXT("HEALTH_%s"),*ID.ToString())),Slot,Enemy->GetActorTransform())))continue;
        auto* AI=CastChecked<AICAIController>(Enemy->GetController());AI->StopExecution();
        Enemy->Tick(0);TestFalse(TEXT("Idle normal/boss monster has no overhead bar"),Enemy->HealthBar->IsVisible());
        auto* Widget=Cast<UICMonsterHealthWidget>(Enemy->HealthBar->GetUserWidgetObject());
        if(Widget)TestEqual(TEXT("The underlying widget also starts hidden"),Widget->GetVisibility(),ESlateVisibility::Collapsed);
        AI->SetMonsterBehaviorTarget(F.Player);Enemy->Tick(0);TestTrue(TEXT("Chasing displays the bar without requiring damage"),Enemy->HealthBar->IsVisible());
        if(Widget)TestEqual(TEXT("Collapsed widget wakes when the chase starts"),Widget->GetVisibility(),ESlateVisibility::HitTestInvisible);
        AI->SetMonsterBehaviorTarget(nullptr);Enemy->Tick(0);TestFalse(TEXT("Unhit monster hides after losing its chase target"),Enemy->HealthBar->IsVisible());
        FICAttackCapture Hit;Hit.Magnitude=1;Enemy->GetICAbilitySystem()->ApplyDamage(Hit,F.Player->GetICAbilitySystem(),false,false,false,false);Enemy->Tick(0);
        TestTrue(TEXT("An actual incoming hit reveals the bar"),Enemy->HealthBar->IsVisible());
        Time->AdvanceClock(8.1,false);Enemy->Tick(0);TestFalse(TEXT("Hit-only display expires outside combat"),Enemy->HealthBar->IsVisible());
        AI->SetMonsterBehaviorTarget(F.Player);Enemy->BeginReturn();Enemy->Tick(0);TestFalse(TEXT("Returning hides even a formerly engaged bar"),Enemy->HealthBar->IsVisible());
        Enemy->Destroy();
    }
    TestTrue(TEXT("Normal and boss rules are both covered"),Normal&&Boss);Settings->bEnemyHealthBars=Before;return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICWarpRequestCheck,"IC.OpenWorld.WarpRequestGating",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICWarpRequestCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;const FName Home=TEXT("WRP_OW_01_HOME"),Inn=TEXT("WRP_OW_02_INN");
    auto* Anchor=F.World->SpawnActor<AICWorldAnchor>();Anchor->AnchorID=TEXT("ANC_WRP_OW_ARRIVAL_01_HOME");Anchor->SetActorLocation(FVector(500,100,200));F.World->GetSubsystem<UICSpawnSubsystem>()->RegisterAnchor(Anchor);
    auto* Stone=F.World->SpawnActor<AICWarpStone>();Stone->WarpID=Home;Stone->InteractionText=FText::FromString(TEXT("집"));
    auto* LockedStone=F.World->SpawnActor<AICWarpStone>();LockedStone->WarpID=Inn;
    auto* Class=LoadClass<UICMapWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_Map.WBP_IC_Map_C"));
    if(!TestNotNull(TEXT("Warp list BP loads"),Class))return false;
    auto* Map=CreateWidget<UICMapWidget>(F.PC,Class);
    auto* Host=NewObject<UCanvasPanel>(F.PC);Host->AddChild(Map);F.PC->Menu=TEXT("Map");
    TestNull(TEXT("Warp selection contains no map image"),Map->GetWidgetFromName(TEXT("IMG_Map")));
    TestNotNull(TEXT("Vertical destination list exists"),Map->GetWidgetFromName(TEXT("WarpDestinations")));
    auto* Loading=Map->GetWidgetFromName(TEXT("MapLoading"));TestNotNull(TEXT("Travel loading cover exists"),Loading);
    const auto Position=F.Player->GetActorLocation();
    TestFalse(TEXT("Unknown destinations are rejected"),Map->RequestTravel(TEXT("WRP_UNKNOWN")));
    TestFalse(TEXT("A placed but locked stone cannot start travel"),Map->RequestTravel(Inn));
    F.PC->Menu.Empty();TestFalse(TEXT("Closed map rejects travel"),Map->RequestTravel(Home));F.PC->Menu=TEXT("Map");
    F.PC->bInCombat=true;TestFalse(TEXT("Combat rejects travel"),Map->RequestTravel(Home));F.PC->bInCombat=false;
    Map->bMinimap=true;TestFalse(TEXT("The minimap cannot directly start travel"),Map->RequestTravel(Home));Map->bMinimap=false;
    TestTrue(TEXT("An unlocked destination opens the confirmation popup"),Map->RequestTravel(Home));
    TestTrue(TEXT("Confirmation is open"),Map->IsConfirmingTravel());TestFalse(TEXT("Selecting does not begin travel"),Map->IsTraveling());
    auto* Question=Cast<UTextBlock>(Map->GetWidgetFromName(TEXT("TXT_WarpQuestion")));
    TestTrue(TEXT("Question names the selected destination"),Question&&Question->GetText().ToString().Contains(TEXT("집으로 이동하겠습니까?")));
    TestEqual(TEXT("Opening the popup does not charge"),F.State->ExportState().Gold,2000);TestEqual(TEXT("Opening the popup does not move the player"),F.Player->GetActorLocation(),Position);
    auto* Confirm=Cast<UButton>(Map->GetWidgetFromName(TEXT("BTN_WarpConfirm")));auto* Cancel=Cast<UButton>(Map->GetWidgetFromName(TEXT("BTN_WarpCancel")));
    if(!TestNotNull(TEXT("Confirm button exists"),Confirm)||!TestNotNull(TEXT("Cancel button exists"),Cancel))return false;
    Cancel->OnClicked.Broadcast();TestFalse(TEXT("Cancel closes the popup"),Map->IsConfirmingTravel());TestFalse(TEXT("Cancel does not travel"),Map->IsTraveling());
    TestEqual(TEXT("Cancel does not charge"),F.State->ExportState().Gold,2000);TestEqual(TEXT("Cancel leaves the player in place"),F.Player->GetActorLocation(),Position);
    Map->RequestTravel(Home);Map->NativeOnPreviewKeyDown(FGeometry(),FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
    TestFalse(TEXT("Escape cancels the popup"),Map->IsConfirmingTravel());TestEqual(TEXT("Escape keeps the destination list open"),F.PC->Menu,FString(TEXT("Map")));
    Map->RequestTravel(Home);Map->NativeOnPreviewKeyDown(FGeometry(),FKeyEvent(EKeys::Right,FModifierKeysState(),0,false,0,0));Map->NativeOnPreviewKeyDown(FGeometry(),FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));
    TestFalse(TEXT("Keyboard can choose Cancel"),Map->IsConfirmingTravel());TestFalse(TEXT("Keyboard cancel does not travel"),Map->IsTraveling());
    TestTrue(TEXT("Test player balance changes through the economy API"),F.State->SetPlayerGold(100));
    TestTrue(TEXT("Even a player with too little gold can open the question"),Map->RequestTravel(Home));Confirm->OnClicked.Broadcast();
    TestFalse(TEXT("Insufficient gold cannot start travel"),Map->IsTraveling());TestTrue(TEXT("Failed confirmation keeps the popup open"),Map->IsConfirmingTravel());
    auto* Error=Cast<UTextBlock>(Map->GetWidgetFromName(TEXT("TXT_WarpConfirmError")));TestTrue(TEXT("The popup explains the failed request"),Error&&!Error->GetText().IsEmpty());
    TestEqual(TEXT("Failed confirmation does not charge"),F.State->ExportState().Gold,100);Cancel->OnClicked.Broadcast();
    TestTrue(TEXT("Test player can afford the confirmed trip"),F.State->SetPlayerGold(2000));
    Map->RequestTravel(Home);Confirm->OnClicked.Broadcast();
    TestTrue(TEXT("Confirm starts travel"),Map->IsTraveling());TestFalse(TEXT("Confirm closes the popup"),Map->IsConfirmingTravel());TestTrue(TEXT("Travel blocks gameplay through the event state"),F.PC->bInEvent);
    if(Loading)TestEqual(TEXT("Loading cover appears before movement"),Loading->GetVisibility(),ESlateVisibility::Visible);
    TestEqual(TEXT("Request does not charge before arrival"),F.State->ExportState().Gold,2000);TestEqual(TEXT("Request does not teleport before the loading delay"),F.Player->GetActorLocation(),Position);
    TestFalse(TEXT("Repeated requests cannot start a second trip"),Map->RequestTravel(Home));
    Confirm->OnClicked.Broadcast();TestEqual(TEXT("A duplicate confirmation does not charge again"),F.State->ExportState().Gold,2000);
    Map->RemoveFromParent();Map->ReleaseSlateResources(true);F.PC->bInEvent=false;F.PC->Menu.Empty();return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPlacedOpenWorldWarpCheck,"IC.OpenWorld.PlacedWarpDestinations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPlacedOpenWorldWarpCheck::RunTest(const FString&)
{
    auto* World=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
    if(!World||World->GetName()!=TEXT("L_OpenWorld")){AddInfo(TEXT("Open L_OpenWorld to validate the placed warp destinations."));return true;}
    FOpenWorldUIFixture F;
    struct FDestination{const TCHAR* Code;FVector2D XY;};
    const FDestination Destinations[]={{TEXT("01_HOME"),{-53740,629630}},{TEXT("02_INN"),{-55695,608350}},
        {TEXT("03_BRIDGE"),{-60715,561595}},{TEXT("04_RUINS"),{-52830,505280}}};
    for(const auto& Destination:Destinations)
    {
        const FName ID(*(FString(TEXT("WRP_OW_"))+Destination.Code));AICWarpStone* Stone=nullptr;int32 Count=0;
        for(TActorIterator<AICWarpStone> It(World);It;++It)if(It->WarpID==ID){Stone=*It;++Count;}
        TestEqual(FString::Printf(TEXT("%s has exactly one placed stone"),*ID.ToString()),Count,1);
        if(!Stone)continue;
        TestTrue(TEXT("Stone uses the newly requested negative-X coordinates"),FVector2D(Stone->GetActorLocation()).Equals(Destination.XY,.1f));
        FICDefinitionResult Result;const auto* Warp=F.Definitions->Find<FICWarpRow>(ID,Result);
        if(!TestNotNull(TEXT("Placed stone has a runtime warp definition"),Warp))continue;
        TestEqual(TEXT("Only the house is unlocked by default"),Warp->bUnlockedByDefault,FString(Destination.Code)==TEXT("01_HOME"));
        AICWorldAnchor* Arrival=nullptr;for(TActorIterator<AICWorldAnchor> It(World);It;++It)if(It->AnchorID==Warp->ArrivalAnchorID)Arrival=*It;
        if(!TestNotNull(TEXT("Destination arrival anchor is loaded in the actual map"),Arrival))continue;
        const auto* Row=F.Definitions->Find<FICAnchorRow>(Warp->ArrivalAnchorID,Result);
        if(TestNotNull(TEXT("Arrival has a matching anchor DT row"),Row))
            TestTrue(TEXT("Saved anchor position matches its DT"),Arrival->GetActorLocation().Equals(Row->WorldTransform.GetLocation(),.1f));
        FCollisionQueryParams Query;Query.AddIgnoredActor(Stone);
        TestFalse(TEXT("Actual arrival has player capsule clearance"),World->OverlapBlockingTestByChannel(Arrival->GetActorLocation(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96),Query));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICOpenWorldDefinitionsCheck,"IC.OpenWorld.RuntimeDefinitions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICOpenWorldDefinitionsCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;
    for(int32 I=1;I<=11;++I)
    {
        const FName ID(*FString::Printf(TEXT("NPC_R01_%03d"),I));FICDefinitionResult R;
        const auto* NPC=F.Definitions->Find<FICNPCRow>(ID,R);if(!TestNotNull(TEXT("Numbered NPC keeps its persistent definition"),NPC))continue;
        const auto Name=FText::FromStringTable(TEXT("/Game/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.ST_IC_Definitions"),NPC->NameKey);
        TestEqual(TEXT("NPC display number matches its ID"),Name.ToString(),FString::Printf(TEXT("NPC %d"),I));
        TestTrue(TEXT("NPC mesh BP number matches its ID"),NPC->ActorClass.ToString().Contains(FString::Printf(TEXT("BP_NPC_R01_%03d"),I)));
    }
    TArray<FName> Warps;F.Definitions->GetDefinitionIDs(EICDefinitionType::Warp,Warps);TestEqual(TEXT("Runtime has exactly the four open-world destinations"),Warps.Num(),4);
    TArray<FName> Monsters;F.Definitions->GetDefinitionIDs(EICDefinitionType::Monster,Monsters);TestEqual(TEXT("Runtime has ten normal monsters and one regional boss"),Monsters.Num(),11);
    FICDefinitionResult R;TestNotNull(TEXT("Runtime spawn group has a production ID"),F.Definitions->Find<FICSpawnerRow>(TEXT("SPN_R01_001"),R));
    TestNull(TEXT("Obsolete combat dummy is outside the runtime catalog"),F.Definitions->Find<FICMonsterRow>(TEXT("MON_COMBAT_TEST_001"),R));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICMinimapWheelCheck,"IC.UI.Map.MinimapWheelZoom",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICMinimapWheelCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;
    auto* HUD=F.HUD();
    if(!TestNotNull(TEXT("Actual HUD loads"),HUD))return false;
    auto Surface=HUD->TakeWidget();
    auto* Map=Cast<UICMapWidget>(HUD->GetWidgetFromName(TEXT("HUD_Minimap")));
    if(!TestNotNull(TEXT("Actual HUD minimap exists"),Map))return false;
    Map->NativeConstruct();
    TestTrue(TEXT("Minimap input processor registers"),Map->MinimapWheelInput.IsValid());
    const auto Input=Map->MinimapWheelInput;
    Map->NativeConstruct();
    TestTrue(TEXT("Reconstructing does not duplicate input registration"),Input==Map->MinimapWheelInput);
    Map->MapMinimumZoom=1.;Map->MapMaximumZoom=8.;Map->MinimapZoom=4.f;
    const FVector PlayerPosition=F.Player->GetActorLocation();
    const auto Wheel=[](float Delta,bool Alt)
    {
        return FPointerEvent(0,FVector2D(100,100),FVector2D(100,100),TSet<FKey>{},EKeys::Invalid,Delta,
            FModifierKeysState(false,false,false,false,Alt,false,false,false,false));
    };
    TestFalse(TEXT("Ordinary wheel stays available for camera and interaction"),Map->HandleMinimapWheel(Wheel(1,false)));
    TestEqual(TEXT("Ordinary wheel leaves zoom unchanged"),Map->MinimapZoom,4.f);
    TestTrue(TEXT("Alt wheel up is consumed"),Map->HandleMinimapWheel(Wheel(1,true)));
    TestEqual(TEXT("One notch enlarges by 25 percent"),Map->MinimapZoom,5.f);
    TestTrue(TEXT("Player remains centred after zoom"),Map->DisplayUV(PlayerPosition).Equals({.5,.5},1.e-6));
    if(auto* Image=Cast<UICMapTextureImage>(Map->GetWidgetFromName(TEXT("IMG_Map"))))
    {
        TestTrue(TEXT("Background updates immediately"),(Image->UVMax-Image->UVMin).Equals({.2,.2},1.e-6));
        TestTrue(TEXT("Background and markers share centre"),((Image->UVMin+Image->UVMax)*.5).Equals(Map->WorldToMapUV(PlayerPosition),1.e-6));
    }
    else AddError(TEXT("Actual minimap image is missing"));
    TestTrue(TEXT("Alt wheel down is consumed"),Map->HandleMinimapWheel(Wheel(-1,true)));
    TestEqual(TEXT("Opposite notch restores zoom"),Map->MinimapZoom,4.f);
    Map->HandleMinimapWheel(Wheel(100,true));TestEqual(TEXT("Zoom stops at maximum"),Map->MinimapZoom,8.f);
    TestTrue(TEXT("Wheel at limit remains consumed to prevent camera movement"),Map->HandleMinimapWheel(Wheel(1,true)));
    Map->HandleMinimapWheel(Wheel(-100,true));TestEqual(TEXT("Zoom stops at minimum"),Map->MinimapZoom,1.f);
    Map->HandleMinimapWheel(Wheel(.5f,true));TestTrue(TEXT("Fractional wheel input is supported"),FMath::IsNearlyEqual(Map->MinimapZoom,FMath::Sqrt(1.25f)));
    const float Before=Map->MinimapZoom;
    F.PC->Menu=TEXT("Map");TestFalse(TEXT("Warp menu does not zoom minimap"),Map->HandleMinimapWheel(Wheel(1,true)));F.PC->Menu.Empty();
    F.PC->bInDialogue=true;TestFalse(TEXT("Dialogue does not zoom minimap"),Map->HandleMinimapWheel(Wheel(1,true)));F.PC->bInDialogue=false;
    Map->SetVisibility(ESlateVisibility::Collapsed);TestFalse(TEXT("Hidden minimap does not consume input"),Map->HandleMinimapWheel(Wheel(1,true)));Map->SetVisibility(ESlateVisibility::HitTestInvisible);
    Map->bMinimap=false;TestFalse(TEXT("Warp list does not zoom"),Map->HandleMinimapWheel(Wheel(1,true)));Map->bMinimap=true;
    TestEqual(TEXT("Blocked inputs leave zoom unchanged"),Map->MinimapZoom,Before);
    TestFalse(TEXT("No focused captured game viewport leaves editor wheel untouched"),Input->HandleMouseWheelOrGestureEvent(FSlateApplication::Get(),Wheel(1,true),nullptr));
    Map->NativeDestruct();TestFalse(TEXT("Closing HUD unregisters input processor"),Map->MinimapWheelInput.IsValid());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICVillageMapCheck,"IC.UI.Map.VillageMapScreen",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICVillageMapCheck::RunTest(const FString&)
{
    FOpenWorldUIFixture F;
    F.World->Rename(TEXT("L_OpenWorld"),CreatePackage(TEXT("/Temp/UIMapReview/Village/L_OpenWorld")),REN_DontCreateRedirectors|REN_NonTransactional);
    auto* HUD=F.HUD();if(!TestNotNull(TEXT("HUD loads for N shortcut"),HUD))return false;
    auto HUDSlate=HUD->TakeWidget();
    auto* Mini=Cast<UICMapWidget>(HUD->GetWidgetFromName(TEXT("HUD_Minimap")));
    if(!TestNotNull(TEXT("HUD minimap owns N shortcut"),Mini))return false;
    auto* UI=F.PC->GetLocalPlayer()->GetSubsystem<UICUIManagerSubsystem>();
    const auto Key=[](FKey Value,bool Repeat=false){return FKeyEvent(Value,FModifierKeysState(),0,Repeat,0,0);};
    TestFalse(TEXT("M is left to the existing warp binding"),Mini->HandleVillageMapShortcut(Key(EKeys::M)));
    TestTrue(TEXT("N opens the village map"),Mini->HandleVillageMapShortcut(Key(EKeys::N)));
    auto* Map=UI->VillageMapScreen.Get();
    if(!TestNotNull(TEXT("Village map is mounted"),Map))return false;
    TestEqual(TEXT("Village screen selects the actual level projection"),Map->ActiveLevelMap,FName(TEXT("L_OpenWorld")));
    TestTrue(TEXT("Village screen uses the village-only capture"),Map->MapTexture&&Map->MapTexture->GetName()==TEXT("T_OpenWorld_Village_UI"));
    FAssetCompilingManager::Get().FinishAllCompilation();
    TestEqual(TEXT("Map has a distinct menu context"),F.PC->Menu,FString(TEXT("VillageMap")));
    TestTrue(TEXT("Opening the map exposes the cursor"),F.PC->bShowMouseCursor);
    TestNotNull(TEXT("Village image exists"),Map->GetWidgetFromName(TEXT("IMG_Map")));
    TestNull(TEXT("Village map is separate from warp destinations"),Map->GetWidgetFromName(TEXT("WarpDestinations")));
    TestEqual(TEXT("Village starts fully zoomed out"),Map->VillageZoom,1.);
    TestFalse(TEXT("Map viewing cannot request a warp"),Map->RequestTravel(TEXT("WRP_OW_01_HOME")));
    if(!GUsingNullRHI)
    {
        FWidgetRenderer Renderer(true,true);auto Slate=Map->TakeWidget();
        auto* Target=Renderer.DrawWidget(Slate,FVector2D(1920,1080));
        TestEqual(TEXT("Village zoom bound comes from level data"),Map->MapMaximumZoom,8.);Map->RefreshVisuals(Map->GetPaintSpaceGeometry());
        Renderer.DrawWidget(Target,Slate,FVector2D(1920,1080),.1f);
        const auto G=Map->GetPaintSpaceGeometry();const auto Rect=Map->ImageRect(G);
        TestTrue(TEXT("Map has a laid-out visible area"),Rect.Right>Rect.Left&&Rect.Bottom>Rect.Top);
        const FVector2D CursorUV(.6,.4);
        const FVector2D Local=FVector2D(Rect.Left,Rect.Top)+CursorUV*FVector2D(Rect.Right-Rect.Left,Rect.Bottom-Rect.Top);
        const auto Pos=G.LocalToAbsolute(Local);
        const auto Wheel=[&](float Delta){return FPointerEvent(0,Pos,Pos,TSet<FKey>{},EKeys::Invalid,Delta,FModifierKeysState());};
        TestTrue(TEXT("Ordinary wheel on map is consumed"),Map->NativeOnMouseWheel(G,Wheel(1)).IsEventHandled());
        TestEqual(TEXT("Wheel up zooms without Alt"),Map->VillageZoom,1.25);
        const auto UnderCursor=Map->VillageCenterUV+(CursorUV-FVector2D(.5,.5))/Map->VillageZoom;
        TestTrue(TEXT("Cursor keeps pointing to the same map location"),UnderCursor.Equals(CursorUV,1.e-6));
        Map->NativeOnMouseWheel(G,Wheel(100));TestEqual(TEXT("Map zoom has an upper bound"),Map->VillageZoom,8.);
        const FVector2D MapSize(Rect.Right-Rect.Left,Rect.Bottom-Rect.Top);
        const auto Pointer=[](FVector2D At,FVector2D Previous,bool Held,FKey Button=EKeys::Invalid)
        {return FPointerEvent(0,At,Previous,Held?TSet<FKey>{EKeys::LeftMouseButton}:TSet<FKey>{},Button,0,FModifierKeysState());};
        Map->VillageCenterUV=FVector2D(.5,.5);Map->RefreshVisuals(G);
        const auto Down=Map->NativeOnMouseButtonDown(G,Pointer(Pos,Pos,true,EKeys::LeftMouseButton));
        TestTrue(TEXT("Left drag captures mouse on enlarged map"),Down.IsEventHandled()&&Down.GetMouseCaptor().IsValid()&&Map->bDraggingVillageMap);
        const FVector2D LocalDelta(40,-24);const auto Moved=G.LocalToAbsolute(Local+LocalDelta);
        const FVector MarkerWorld(Map->Layer.WorldOriginCm.X,Map->Layer.WorldOriginCm.Y,0);
        const FVector2D MarkerBefore=Map->DisplayUV(MarkerWorld);
        Map->NativeOnMouseMove(G,Pointer(Moved,Pos,true));
        TestTrue(TEXT("Map follows drag at the current zoom and DPI"),Map->VillageCenterUV.Equals(FVector2D(.5,.5)-LocalDelta/(MapSize*Map->VillageZoom),1.e-6));
        TestTrue(TEXT("Marker moves with the image"),(Map->DisplayUV(MarkerWorld)-MarkerBefore).Equals(LocalDelta/MapSize,1.e-6));
        auto* PannedImage=Cast<UICMapTextureImage>(Map->GetWidgetFromName(TEXT("IMG_Map")));
        TestTrue(TEXT("Pan updates texture window immediately"),PannedImage&&((PannedImage->UVMin+PannedImage->UVMax)*.5).Equals(Map->VillageCenterUV,1.e-6));
        const auto Far=G.LocalToAbsolute(Local+FVector2D(100000,-100000));
        Map->NativeOnMouseMove(G,Pointer(Far,Moved,true));
        const double Half=.5/Map->VillageZoom;
        TestTrue(TEXT("Dragging outside the widget stops at map edges"),Map->VillageCenterUV.Equals({Half,1.-Half},1.e-6));
        const auto FarOther=G.LocalToAbsolute(Local+FVector2D(-100000,100000));
        Map->NativeOnMouseMove(G,Pointer(FarOther,Far,true));
        TestTrue(TEXT("Opposite map edges are also bounded"),Map->VillageCenterUV.Equals({1.-Half,Half},1.e-6));
        const auto Up=Map->NativeOnMouseButtonUp(G,Pointer(FarOther,FarOther,false,EKeys::LeftMouseButton));
        TestTrue(TEXT("Release outside the image ends capture"),Up.ShouldReleaseMouse()&&!Map->bDraggingVillageMap);
        const FVector2D AfterRelease=Map->VillageCenterUV;
        Map->NativeOnMouseMove(G,Pointer(Pos,FarOther,false));
        TestTrue(TEXT("Pointer movement after release does not pan"),Map->VillageCenterUV.Equals(AfterRelease));
        Map->NativeOnMouseButtonDown(G,Pointer(Pos,Pos,true,EKeys::LeftMouseButton));
        Map->NativeOnMouseCaptureLost(FCaptureLostEvent(0,0));
        TestFalse(TEXT("Losing mouse capture cancels drag"),Map->bDraggingVillageMap);
        Map->NativeOnMouseButtonDown(G,Pointer(Pos,Pos,true,EKeys::RightMouseButton));
        TestFalse(TEXT("Right click does not start a pan"),Map->bDraggingVillageMap);
        const auto Outside=G.LocalToAbsolute(FVector2D(20,20));
        Map->NativeOnMouseButtonDown(G,Pointer(Outside,Outside,true,EKeys::LeftMouseButton));
        TestFalse(TEXT("Header and margins do not start a pan"),Map->bDraggingVillageMap);
        Map->NativeOnMouseButtonDown(G,Pointer(Pos,Pos,true,EKeys::LeftMouseButton));
        Map->NativeOnMouseWheel(G,Wheel(-100));TestEqual(TEXT("Zooming out restores the overview"),Map->VillageZoom,1.);
        TestFalse(TEXT("Returning to full view cancels dragging"),Map->bDraggingVillageMap);
        const auto FullViewDown=Map->NativeOnMouseButtonDown(G,Pointer(Pos,Pos,true,EKeys::LeftMouseButton));
        TestTrue(TEXT("Overview cannot be dragged into blank space"),!Map->bDraggingVillageMap&&!FullViewDown.GetMouseCaptor().IsValid());
        TestTrue(TEXT("Overview restores its centre"),Map->VillageCenterUV.Equals({.5,.5},1.e-6));
        Renderer.DrawWidget(Target,Slate,FVector2D(1920,1080),.1f);FlushRenderingCommands();
        TArray<FColor> Pixels;FReadSurfaceDataFlags Flags;Flags.SetLinearToGamma(false);
        if(Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))
        {
            for(auto& Pixel:Pixels)Pixel.A=255;TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1920,1080,Pixels,PNG);
            const FString Folder=FPaths::ProjectSavedDir()/TEXT("OpenWorldMapReview");IFileManager::Get().MakeDirectory(*Folder,true);
            FFileHelper::SaveArrayToFile(PNG,*(Folder/TEXT("VillageMapN.png")));
        }
    }
    Map->NativeOnPreviewKeyDown(FGeometry(),Key(EKeys::N,true));TestNotNull(TEXT("Holding N does not close the map immediately"),UI->VillageMapScreen.Get());
    Map->NativeOnPreviewKeyDown(FGeometry(),Key(EKeys::N));
    TestFalse(TEXT("Closing the map clears drag state"),Map->bDraggingVillageMap);
    TestNull(TEXT("N closes the map"),UI->VillageMapScreen.Get());TestTrue(TEXT("Closing restores gameplay context"),F.PC->Menu.IsEmpty());TestFalse(TEXT("Closing restores hidden gameplay cursor"),F.PC->bShowMouseCursor);
    Mini->HandleVillageMapShortcut(Key(EKeys::N));Map=UI->VillageMapScreen.Get();
    if(Map)Map->NativeOnPreviewKeyDown(FGeometry(),Key(EKeys::Escape));
    TestNull(TEXT("Escape closes map"),UI->VillageMapScreen.Get());
    Mini->HandleVillageMapShortcut(Key(EKeys::N));Map=UI->VillageMapScreen.Get();
    if(Map)if(auto* Button=Cast<UButton>(Map->GetWidgetFromName(TEXT("BTN_MapClose"))))Button->OnClicked.Broadcast();
    TestNull(TEXT("Close button closes map"),UI->VillageMapScreen.Get());
    Mini->HandleVillageMapShortcut(Key(EKeys::N));Map=UI->VillageMapScreen.Get();
    if(Map)Map->NativeOnPreviewKeyDown(FGeometry(),Key(EKeys::M));
    TestNull(TEXT("M closes village view"),UI->VillageMapScreen.Get());TestEqual(TEXT("M opens existing warp menu"),F.PC->Menu,FString(TEXT("Map")));
    TestFalse(TEXT("N does not open underneath another menu"),Mini->HandleVillageMapShortcut(Key(EKeys::N)));
    F.PC->SetMenu(TEXT(""));Mini->HandleVillageMapShortcut(Key(EKeys::N));F.PC->SetMenu(TEXT("Loading"));UI->Tick(.01f);
    TestNull(TEXT("Other screen transitions release village map"),UI->VillageMapScreen.Get());
    F.PC->SetMenu(TEXT(""));UI->CloseAllScreens();return !HasAnyErrors();
}
#endif
