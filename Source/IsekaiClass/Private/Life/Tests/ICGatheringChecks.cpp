#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputKeyEventArgs.h"
#include "Widgets/SOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Controller/ICPlayerController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Life/Production/ICGatheringSubsystem.h"
#include "Life/World/ICLifeActor.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "UI/Gameplay/ICGatherProgressWidget.h"
#include "UI/Gameplay/ICCombatFeedbackWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICGatheringCheck,"IC.Life.GatheringProgress",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICGatheringCheck::RunTest(const FString&)
{
    auto* WidgetType=LoadClass<UICGatherProgressWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_GatherProgress.WBP_IC_GatherProgress_C"));
    if(!TestNotNull(TEXT("Gathering has its own authored WBP"),WidgetType))return false;
    auto* GI=NewObject<UGameInstance>(GEngine);GI->InitializeStandalone();auto* World=GI->GetWorld();
    auto& Context=*GEngine->GetWorldContextFromWorld(World);auto* Viewport=NewObject<UGameViewportClient>(GEngine);
    Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);World->InitializeActorsForPlay(FURL(),false);
    // Standalone automation worlds have no game window to create their viewport overlay.
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();auto* Time=GI->GetSubsystem<UICTimeSubsystem>();
    auto* D=GI->GetSubsystem<UICDefinitionSubsystem>();auto* Gather=GI->GetSubsystem<UICGatheringSubsystem>();
    auto* PCType=LoadClass<AICPlayerController>(nullptr,TEXT("/Game/IsekaiClass/Core/GameFramework/BP_ICPlayerController.BP_ICPlayerController_C"));
    auto* PC=World->SpawnActor<AICPlayerController>(PCType);FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
    auto* Pawn=World->SpawnActor<AICPlayerCharacter>();Pawn->EnableCombat(true,true);PC->Possess(Pawn);PC->bCreated=true;PC->Menu.Empty();
    FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("채집검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
    FICCharacterState Player;Player.CharacterId=UICWorldStateSubsystem::PlayerId();Player.Kind=EICCharacterKind::Player;Initial.Characters.Add(Player.CharacterId,Player);
    TestTrue(TEXT("Player state restores"),State->RestoreState(Initial));
    TestTrue(TEXT("Live player state initializes"),Pawn->InitializeState(Player.CharacterId));Pawn->Inventory->BindWorldState(State);
    Time->SetRunning(true);Time->SetMenuPaused(false);
    auto* FeedbackType=LoadClass<UICCombatFeedbackWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_CombatFeedback.WBP_IC_CombatFeedback_C"));
    auto* Feedback=CreateWidget<UICCombatFeedbackWidget>(PC,FeedbackType);const auto FeedbackSlate=Feedback->TakeWidget();
    auto* RewardCanvas=CastChecked<UCanvasPanel>(Feedback->GetWidgetFromName(TEXT("FeedbackCanvas")));
    FICDefinitionResult R;const auto* Resource=D->Find<FICResourceRow>(TEXT("RES_R01_GATHER_01"),R);
    if(!TestNotNull(TEXT("Real gathering resource DT"),Resource)){GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false;}
    auto* Plant=World->SpawnActor<AICLifeActor>(Resource->ResourceActorClass.LoadSynchronous(),Pawn->GetActorLocation()+FVector(180,0,0),FRotator::ZeroRotator);
    Plant->PointID=TEXT("TEST_GATHER_PROGRESS");
    auto* Inventory=NewObject<UICInventoryComponent>();Inventory->BeginStaging(D,0);
    auto Count=[&](){Inventory->ApplySnapshot(State->GetInventorySnapshot());return Inventory->CountItem(Resource->Yields[0].ItemID);};
    auto Step=[&](double Seconds){Time->AdvanceClock(Seconds,false);Gather->Tick(Seconds);};
    const double Started=FPlatformTime::Seconds();
    TestTrue(TEXT("F starts gathering immediately"),Plant->TryInteract(PC));
    AddInfo(FString::Printf(TEXT("Gathering start returned in %.3f ms"),(FPlatformTime::Seconds()-Started)*1000));
    TestTrue(TEXT("World keeps running while gathering"),Time->IsRunning()&&!World->IsPaused());
    TestEqual(TEXT("No upfront reward"),Count(),0);TestFalse(TEXT("No upfront resource depletion"),State->ExportState().ResourceReadyAt.Contains(Plant->PointID));
    TestFalse(TEXT("Repeated F cannot start a second pending reward"),Plant->TryInteract(PC));
    auto* Widget=Gather->GetProgressWidget();
    if(TestNotNull(TEXT("Progress overlay is visible"),Widget))
    {
        TestTrue(TEXT("Progress overlay belongs to the WBP"),Widget->IsA(WidgetType));
        TestTrue(TEXT("Progress overlay does not capture input"),Widget->GetVisibility()==ESlateVisibility::HitTestInvisible);
        TestEqual(TEXT("Gauge begins empty"),Widget->Progress,0.f);
    }
    Step(2.5);TestTrue(TEXT("Clock advances during gathering"),Time->GetClock().PlaySecond>=2.5);
    TestEqual(TEXT("Gauge is half full after 2.5 seconds"),Gather->GetProgress(),.5f);
    if(Widget)
        if(auto* Bar=Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("BAR_GatherProgress"))))TestEqual(TEXT("Actual WBP bar follows progress"),Bar->GetPercent(),.5f);
        else AddError(TEXT("Missing progress bar"));
    Step(2.49);TestEqual(TEXT("No reward before five seconds"),Count(),0);
    Step(.01);TestFalse(TEXT("Action completes at five seconds"),Gather->IsGathering());
    TestNull(TEXT("Progress overlay is removed on completion"),Gather->GetProgressWidget());
    TestEqual(TEXT("Completion grants the real resource yield once"),Count(),Resource->Yields[0].Count);
    TestEqual(TEXT("Gathering creates exactly one HUD reward notice"),RewardCanvas->GetChildrenCount(),1);
    if(auto* Toast=Cast<UICRewardNoticeWidget>(RewardCanvas->GetChildAt(0)))
    {
        const auto* Item=D->Find<FICItemRow>(Resource->Yields[0].ItemID,R);
        const auto Text=CastChecked<UTextBlock>(Toast->GetWidgetFromName(TEXT("TXT_Items")))->GetText().ToString();
        TestEqual(TEXT("Gathering HUD uses the actual item name and count"),Text,FString(TEXT("+ "))+Item->DisplayName.ToString()+TEXT(" 1개"));
        TestEqual(TEXT("Gathering does not show zero gold"),Toast->GetWidgetFromName(TEXT("TXT_Gold"))->GetVisibility(),ESlateVisibility::Collapsed);
        TestEqual(TEXT("Gathering does not show monster essence"),Toast->GetWidgetFromName(TEXT("TXT_Essence"))->GetVisibility(),ESlateVisibility::Collapsed);
    }
    TestTrue(TEXT("Completion starts the saved resource cooldown"),State->ExportState().ResourceReadyAt.Contains(Plant->PointID));
    TestFalse(TEXT("Depleted resource cannot start another gather"),Plant->TryInteract(PC));
    Step(1);TestEqual(TEXT("Further ticks cannot duplicate rewards"),Count(),Resource->Yields[0].Count);
    TestEqual(TEXT("Further ticks cannot duplicate HUD notices"),RewardCanvas->GetChildrenCount(),1);

    auto Reset=[&](){Gather->CancelGather();PC->Menu.Empty();TestTrue(TEXT("Fresh resource state restores"),State->RestoreState(Initial));Time->SetRunning(true);Time->SetMenuPaused(false);};
    for(auto Command:{EICInputCommand::Interact,EICInputCommand::Menu})
    {
        Reset();TestTrue(TEXT("Start for input cancellation"),Plant->TryInteract(PC));Step(2);
        PC->HandleInputCommand(Command);TestFalse(TEXT("F and ESC cancel gathering"),Gather->IsGathering());
        TestTrue(TEXT("Cancellation keeps gameplay active"),PC->Menu.IsEmpty()&&Time->IsRunning());
        Step(6);TestEqual(TEXT("Cancellation grants no reward"),Count(),0);
        TestFalse(TEXT("Cancellation consumes no resource"),State->ExportState().ResourceReadyAt.Contains(Plant->PointID));
        TestEqual(TEXT("Cancellation displays no reward notice"),RewardCanvas->GetChildrenCount(),0);
    }
    Reset();Plant->TryInteract(PC);Pawn->SetActorLocation(Pawn->GetActorLocation()+FVector(25,0,0));Gather->Tick(0);
    TestFalse(TEXT("Moving cancels gathering"),Gather->IsGathering());TestFalse(TEXT("Movement remains enabled"),PC->IsMoveInputIgnored());
    if(!PC->InputComponent)PC->InitInputSystem();
    auto* Input=CastChecked<UEnhancedPlayerInput>(PC->PlayerInput);
    FModifyContextOptions Options;Options.bForceImmediately=true;Options.bIgnoreAllPressedKeysUntilRelease=false;
    PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->AddMappingContext(PC->DefaultMappingContext,0,Options);
    const TArray<UInputComponent*> InputStack{PC->InputComponent};
    Reset();TestTrue(TEXT("Gather begins before WASD"),Plant->TryInteract(PC));
    Input->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),EKeys::W,IE_Pressed,1.f,false,FPlatformTime::Cycles64()));
    Input->ProcessInputStack(InputStack,.016f,false);
    TestFalse(TEXT("Actual mapped W input immediately cancels gathering"),Gather->IsGathering());
    TestFalse(TEXT("Mapped W still moves the player"),Pawn->GetPendingMovementInputVector().IsNearlyZero());
    Input->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),EKeys::W,IE_Released,0.f,false,FPlatformTime::Cycles64()));
    Input->ProcessInputStack(InputStack,.016f,false);Pawn->ConsumeMovementInputVector();
    Reset();Plant->TryInteract(PC);State->OnCombatDamageReceived.Broadcast(Pawn,10);
    TestFalse(TEXT("Taking damage cancels gathering"),Gather->IsGathering());
    Reset();Plant->TryInteract(PC);State->RestoreState(Initial);
    TestFalse(TEXT("Loading cancels an unfinished reward"),Gather->IsGathering());TestEqual(TEXT("Loading cannot grant pending loot"),Count(),0);
    Reset();Plant->TryInteract(PC);PC->SetMenu(TEXT("Pause"));
    TestFalse(TEXT("Opening a menu removes progress immediately"),Gather->IsGathering());PC->SetMenu(TEXT(""));

    Reset();Inventory->ApplySnapshot(State->GetInventorySnapshot());
    int32 FillAttempts=0;while(Inventory->AddItem(TEXT("ITM_R01_SEED_01"),99,false)&&++FillAttempts<1000){}
    TestTrue(TEXT("Full inventory fixture commits"),State->CommitInventoryState(Inventory->GetSnapshot()));
    TestTrue(TEXT("Gather starts before final capacity check"),Plant->TryInteract(PC));Step(5);
    TestEqual(TEXT("Full bag grants no partial loot"),Count(),0);
    TestFalse(TEXT("Full bag does not deplete the plant"),State->ExportState().ResourceReadyAt.Contains(Plant->PointID));
    TestTrue(TEXT("Full bag reports why reward failed"),PC->Notice.Contains(TEXT("가방")));
    TestEqual(TEXT("Full bag displays no reward notice"),RewardCanvas->GetChildrenCount(),0);

    Reset();TestTrue(TEXT("Final gathering starts"),Plant->TryInteract(PC));Plant->Destroy();Gather->Tick(0);
    TestNull(TEXT("Destroying target removes the overlay"),Gather->GetProgressWidget());TestEqual(TEXT("Destroyed target grants no reward"),Count(),0);
    Reset();const auto* OreRow=D->Find<FICResourceRow>(TEXT("RES_R01_ORE_01"),R);
    auto* Ore=World->SpawnActor<AICLifeActor>(OreRow->ResourceActorClass.LoadSynchronous(),Pawn->GetActorLocation()+FVector(180,0,0),FRotator::ZeroRotator);
    Ore->PointID=TEXT("TEST_MINING_NOTICE");TestTrue(TEXT("Real ore begins mining"),Ore->TryInteract(PC));Step(5);
    TestEqual(TEXT("Mining creates one HUD reward notice"),RewardCanvas->GetChildrenCount(),1);
    if(auto* Toast=Cast<UICRewardNoticeWidget>(RewardCanvas->GetChildAt(0)))
        TestEqual(TEXT("Mining HUD names the real ore and quantity"),CastChecked<UTextBlock>(Toast->GetWidgetFromName(TEXT("TXT_Items")))->GetText().ToString(),FString(TEXT("+ 광석1 1개")));
    Feedback->RemoveFromParent();
    GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
