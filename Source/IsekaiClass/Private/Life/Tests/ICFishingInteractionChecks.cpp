#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Widgets/SOverlay.h"
#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "NPC/Character/ICNPCBase.h"
#include "Life/World/ICLifeActor.h"
#include "Life/World/ICFishingLine.h"
#include "Life/Production/ICGatheringSubsystem.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Inventory/Components/ICInventoryComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "InputMappingContext.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"

namespace ICFishingChecks
{
struct FSession
{
    UGameInstance* GI=NewObject<UGameInstance>(GEngine);UWorld* World;
    AICPlayerController* PC;AICPlayerCharacter* Pawn;UICTimeSubsystem* Clock;UICGatheringSubsystem* Gather;
    FSession()
    {
        GI->InitializeStandalone();World=GI->GetWorld();auto& Context=*GEngine->GetWorldContextFromWorld(World);
        auto* Viewport=NewObject<UGameViewportClient>(GEngine);Context.GameViewport=Viewport;Viewport->Init(Context,GI,false);
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
        World->InitializeActorsForPlay(FURL(),false);
        PC=World->SpawnActor<AICPlayerController>();FString Error;PC->SetPlayer(GI->CreateLocalPlayer(0,Error,false));
        Pawn=World->SpawnActor<AICPlayerCharacter>();Pawn->EnableCombat(true,true);PC->Possess(Pawn);PC->bCreated=true;PC->Menu.Empty();
        FICWorldState Initial;Initial.bPlayerCreated=true;Initial.PlayerName=TEXT("낚시 검증");Initial.FirstSkill=TEXT("SKL_START_SWORD_001");
        FICCharacterState PlayerState;PlayerState.CharacterId=UICWorldStateSubsystem::PlayerId();PlayerState.Kind=EICCharacterKind::Player;
        Initial.Characters.Add(PlayerState.CharacterId,PlayerState);
        auto* State=GI->GetSubsystem<UICWorldStateSubsystem>();State->RestoreState(Initial);Pawn->InitializeState(PlayerState.CharacterId);Pawn->Inventory->BindWorldState(State);
        Clock=GI->GetSubsystem<UICTimeSubsystem>();Clock->SetRunning(true);Clock->SetMenuPaused(false);Gather=GI->GetSubsystem<UICGatheringSubsystem>();
        Pawn->Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    }
    ~FSession(){Gather->CancelGather();Pawn->ReleaseForLoad();GI->Shutdown();World->DestroyWorld(false);}
    int32 FishCount() const{int32 N=0;for(int32 I=1;I<=10;++I)N+=Pawn->Inventory->CountItem(FName(*FString::Printf(TEXT("ITM_R01_FISH_%02d"),I)));return N;}
    void Step(double Seconds){Clock->AdvanceClock(Seconds,false);Gather->Tick(Seconds);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFishingLandingCheck,"IC.Life.Fishing.AimedLineAndTwentySecondCatch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICFishingLandingCheck::RunTest(const FString&)
{
    ICFishingChecks::FSession F;FICDefinitionResult R;
    const auto* Point=F.GI->GetSubsystem<UICDefinitionSubsystem>()->Find<FICFishingPointRow>(TEXT("FPT_R01_RANDOM_001"),R);
    if(!TestNotNull(TEXT("Random fishing DT exists"),Point))return false;
    auto* Zone=F.World->SpawnActor<AICLifeActor>(Point->PointActorClass.LoadSynchronous(),FVector(600,0,60),FRotator::ZeroRotator);
    FVector Landing,Normal;Zone->GetFishingSurface(Landing,Normal);
    const FVector CameraPosition(-250,0,280);F.Pawn->Camera->SetWorldLocationAndRotation(CameraPosition,(Landing-CameraPosition).Rotation());
    F.PC->SetControlRotation(FRotator::ZeroRotator);
    FVector Resolved;TestEqual(TEXT("Screen center ray finds water surface"),F.Pawn->LifeActions->ResolveFishingCast(Resolved),Zone);
    TestTrue(TEXT("Resolved float lies inside the zone"),Zone->ContainsFishingLanding(Resolved));
    TestFalse(TEXT("F never fishes"),Zone->TryInteract(F.PC));TestFalse(TEXT("Generic gather cannot bypass aimed fishing"),F.Gather->StartGather(Zone,F.PC));
    TestFalse(TEXT("A float outside the water cannot start fishing"),F.Gather->StartFishing(Zone,F.PC,Landing+FVector(2000,0,0)));
    auto* IMC=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/IsekaiClass/Input/Contexts/IMC_Gameplay.IMC_Gameplay"));bool H=false;
    if(IMC)for(const auto& Mapping:IMC->GetMappings())if(Mapping.Action&&Mapping.Action->GetName()==TEXT("IA_Fishing"))H|=Mapping.Key==EKeys::H;
    TestTrue(TEXT("Authored fishing input is H"),H);
    F.PC->HandleInputCommand(EICInputCommand::Fishing,0);
    if(!TestTrue(TEXT("H starts a long cast farther than F interaction distance"),F.Gather->IsGathering()))return false;
    auto* Line=F.Pawn->LifeActions->GetFishingLine();TestTrue(TEXT("Real rod line and float are valid"),Line&&Line->HasValidLanding());
    F.Step(19.9);TestEqual(TEXT("No reward before twenty seconds"),F.FishCount(),0);TestTrue(TEXT("Fishing remains active"),F.Gather->IsGathering());
    F.Step(.1);TestEqual(TEXT("One fish after twenty seconds"),F.FishCount(),1);TestFalse(TEXT("Gauge closes on completion"),F.Gather->IsGathering());TestNull(TEXT("Rod line is removed on completion"),F.Pawn->LifeActions->GetFishingLine());
    F.PC->HandleInputCommand(EICInputCommand::Fishing,0);TestTrue(TEXT("No cooldown: H immediately starts next cast"),F.Gather->IsGathering());
    F.Step(5);F.PC->HandleInputCommand(EICInputCommand::Fishing,0);F.Step(25);
    TestFalse(TEXT("Second H cancels rather than restarting"),F.Gather->IsGathering());TestEqual(TEXT("Canceled cast grants no fish"),F.FishCount(),1);
    F.PC->HandleInputCommand(EICInputCommand::Fishing,0);Zone->SetActorLocation(Zone->GetActorLocation()+FVector(2000,0,0));F.Step(20);
    TestEqual(TEXT("A float that loses its zone cannot grant a catch"),F.FishCount(),1);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICNPCInteractionCameraCheck,"IC.UI.DialogueFaceCamera",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICNPCInteractionCameraCheck::RunTest(const FString&)
{
    ICFishingChecks::FSession F;auto* NPC=F.World->SpawnActor<AICNPCBase>(FVector(250,0,96),FRotator(0,180,0));
    F.PC->SetViewTarget(F.Pawn);const FTransform Before=F.Pawn->GetActorTransform();const FRotator Rotation=F.PC->GetControlRotation();
    F.PC->BeginNPCDialogueCamera(NPC);auto* Camera=Cast<ACameraActor>(F.PC->GetViewTarget());
    TestTrue(TEXT("Player cannot occlude NPC face in the owning view"),F.PC->HiddenActors.Contains(F.Pawn));
    if(TestNotNull(TEXT("NPC interaction uses a dedicated face camera"),Camera))
    {
        TestTrue(TEXT("Camera is close enough to see the face"),FVector::Distance(Camera->GetActorLocation(),NPC->GetActorLocation())<260.f);
        TestTrue(TEXT("Camera is in front of NPC"),FVector::DotProduct(Camera->GetActorLocation()-NPC->GetActorLocation(),NPC->GetActorForwardVector())>0);
        TestEqual(TEXT("Portrait framing FOV"),Camera->GetCameraComponent()->FieldOfView,40.f);
        if(F.PC->PlayerCameraManager)
        {
            TestTrue(TEXT("Camera cache updates immediately even when dialogue has paused world ticks"),F.PC->PlayerCameraManager->GetCameraLocation().Equals(Camera->GetActorLocation(),.1));
            TestEqual(TEXT("Rendered camera FOV updates immediately"),F.PC->PlayerCameraManager->GetFOVAngle(),40.f);
        }
    }
    F.PC->Menu=TEXT("Dialogue");F.PC->SetMenu(TEXT(""));
    TestFalse(TEXT("Player visibility restores after dialogue"),F.PC->HiddenActors.Contains(F.Pawn));
    TestEqual(TEXT("Dialogue close restores previous gameplay view"),F.PC->GetViewTarget(),static_cast<AActor*>(F.Pawn));
    TestTrue(TEXT("Player transform is preserved"),Before.Equals(F.Pawn->GetActorTransform()));TestTrue(TEXT("Look direction is preserved"),Rotation.Equals(F.PC->GetControlRotation()));
    F.PC->BeginNPCDialogueCamera(NPC);F.PC->Menu=TEXT("Dialogue");F.PC->SetMenu(TEXT("Inventory"));
    TestEqual(TEXT("Switching menu also restores camera"),F.PC->GetViewTarget(),static_cast<AActor*>(F.Pawn));
    F.PC->SetMenu(TEXT(""));F.PC->BeginNPCDialogueCamera(NPC);
    F.PC->DialogueScreen=CreateWidget<UICNPCDialogueWidget>(F.PC,LoadClass<UICNPCDialogueWidget>(nullptr,TEXT("/Game/IsekaiClass/UI/Widget/WBP_IC_NPCDialogue.WBP_IC_NPCDialogue_C")));
    F.PC->Menu=TEXT("Dialogue");F.PC->bInDialogue=true;
    const FVector InitialCamera=F.PC->NPCDialogueCamera->GetActorLocation();NPC->SetActorLocation(NPC->GetActorLocation()+FVector(30,0,0));F.PC->UpdateNPCDialogue(.2f);
    TestFalse(TEXT("Live dialogue camera follows NPC instead of freezing a frame"),F.PC->NPCDialogueCamera->GetActorLocation().Equals(InitialCamera,.1));
    F.Pawn->SetActorLocation(NPC->GetActorLocation()+FVector(1000,0,0));F.PC->UpdateNPCDialogue(.2f);
    TestTrue(TEXT("Walking beyond DT distance closes dialogue"),F.PC->Menu.IsEmpty());TestEqual(TEXT("Distance close restores gameplay camera"),F.PC->GetViewTarget(),static_cast<AActor*>(F.Pawn));
    return !HasAnyErrors();
}
#endif
