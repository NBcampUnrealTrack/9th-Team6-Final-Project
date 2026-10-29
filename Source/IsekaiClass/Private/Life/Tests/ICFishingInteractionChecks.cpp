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
#include "Inventory/Actors/ICChestPresentationComponent.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "Components/StaticMeshComponent.h"
#include "UI/Dialogue/ICNPCDialogueWidget.h"
#include "Editor.h"
#include "EngineUtils.h"

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
    ~FSession(){Gather->CancelGather();Pawn->ReleaseForLoad();GI->Shutdown();GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICHiddenFishingRegionsCheck,"IC.Life.Fishing.HiddenAndMultipleRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICHiddenFishingRegionsCheck::RunTest(const FString&)
{
    ICFishingChecks::FSession F;
    auto* Zone=F.World->SpawnActor<AICLifeActor>();Zone->LifeType=EICDefinitionType::FishingPoint;Zone->DefinitionID=TEXT("FPT_R01_RANDOM_001");
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    Zone->VisualMesh->SetStaticMesh(Plane);Zone->VisualMesh->SetWorldLocation(FVector(600,-500,60));Zone->VisualMesh->SetWorldScale3D(FVector(3,3,1));
    auto* Extra=NewObject<UStaticMeshComponent>(Zone);Zone->AddInstanceComponent(Extra);Extra->SetStaticMesh(Plane);Extra->SetCollisionEnabled(ECollisionEnabled::NoCollision);Extra->RegisterComponent();Extra->AttachToComponent(Zone->Shape,FAttachmentTransformRules::KeepWorldTransform);Extra->SetWorldLocation(FVector(700,500,60));Extra->SetWorldScale3D(FVector(3,3,1));
    Zone->SetActorHiddenInGame(true);Zone->SetActorEnableCollision(false);
    const FVector Target(700,500,60),Camera(-250,0,280);
    F.Pawn->Camera->SetWorldLocationAndRotation(Camera,(Target-Camera).Rotation());
    FVector Landing;TestEqual(TEXT("Hidden actor's second mesh is a usable fishing region"),F.Pawn->LifeActions->ResolveFishingCast(Landing),Zone);
    TestTrue(TEXT("A float is accepted in the added mesh"),Zone->ContainsFishingLanding(Target));
    TestFalse(TEXT("Space between separated meshes is not a fishing region"),Zone->ContainsFishingLanding(FVector(650,0,60)));
    // A visible surface exactly at the float should not count as an intervening wall.
    auto* Water=F.World->SpawnActor<AActor>();auto* WaterBox=NewObject<UBoxComponent>(Water);Water->AddInstanceComponent(WaterBox);Water->SetRootComponent(WaterBox);WaterBox->SetBoxExtent(FVector(100,100,5));WaterBox->SetCollisionResponseToAllChannels(ECR_Block);WaterBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);WaterBox->RegisterComponent();Water->SetActorLocation(Target-FVector(0,0,5));
    TestEqual(TEXT("The landing surface itself does not block a cast"),F.Pawn->LifeActions->ResolveFishingCast(Landing),Zone);
    Water->SetActorLocation(Target);
    TestEqual(TEXT("A shallow cast tolerates water five centimetres above the zone"),F.Pawn->LifeActions->ResolveFishingCast(Landing),Zone);
    TestTrue(TEXT("The float uses the real water height"),FMath::IsNearlyEqual(Landing.Z,Target.Z+5.f,.1f));
    F.PC->HandleInputCommand(EICInputCommand::Fishing,0);TestTrue(TEXT("H starts fishing on the hidden additional region"),F.Gather->IsGathering());
    auto* Line=F.Pawn->LifeActions->GetFishingLine();
    if(TestNotNull(TEXT("Shallow shore cast owns a fishing line"),Line))TestTrue(TEXT("Line float rests on real water"),Line->GetLandingPoint().Equals(Landing,.1f));
    F.Step(20);TestEqual(TEXT("Hidden additional region grants the normal catch"),F.FishCount(),1);
    Water->SetActorLocation((Camera+Target)*.5);WaterBox->SetBoxExtent(FVector(80,80,200));
    TestNull(TEXT("A real obstacle before the float still blocks casting"),F.Pawn->LifeActions->ResolveFishingCast(Landing));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICPlacedFishingRegionsCheck,"IC.Life.Fishing.PlacedOpenWorldRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICPlacedFishingRegionsCheck::RunTest(const FString&)
{
    auto* World=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
    if(!World||World->GetName()!=TEXT("L_OpenWorld")){AddInfo(TEXT("Open L_OpenWorld to validate its placed fishing areas."));return true;}
    TestFalse(TEXT("Current open world supports Actor Foliage painting"),World->IsPartitionedWorld());
    int32 Zones=0,Regions=0;
    for(TActorIterator<AICLifeActor> It(World);It;++It)
    {
        if(It->LifeType!=EICDefinitionType::FishingPoint)continue;++Zones;
        const int32 BeforeRegions=Regions;
        TInlineComponentArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
        for(auto* Mesh:Meshes)
        {
            if(Mesh==It->Shape||!Mesh->GetStaticMesh())continue;++Regions;
            const auto Bounds=Mesh->GetStaticMesh()->GetBoundingBox();
            FVector Local=Bounds.GetCenter();Local.Z=Bounds.Max.Z;
            const FVector Point=Mesh->GetComponentTransform().TransformPosition(Local);
            const FVector Camera=Point+Mesh->GetUpVector()*450+FVector(-600,-200,0);FVector Landing;
            TestTrue(FString::Printf(TEXT("Placed mesh %s accepts its surface, including hidden actors"),*Mesh->GetName()),It->ContainsFishingLanding(Point));
            TestTrue(FString::Printf(TEXT("Placed mesh %s is reached by aimed casting"),*Mesh->GetName()),It->ResolveFishingLanding(Camera,(Point-Camera).GetSafeNormal(),Landing));
        }
        TestTrue(FString::Printf(TEXT("Fishing actor %s has a usable water region"),*It->GetName()),Regions>BeforeRegions);
    }
    TestTrue(TEXT("Saved open world has a fishing actor"),Zones>0);
    AddInfo(FString::Printf(TEXT("Validated all %d water regions across %d placed fishing actors."),Regions,Zones));
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
    TestFalse(TEXT("Dialogue blocks player gameplay actions without pausing the world"),F.PC->CanHandleGameplayInput());
    TestFalse(TEXT("Dialogue never forwards W to character movement"),F.PC->ForwardDialogueMovementKey(EKeys::W,true));
    TestFalse(TEXT("Dialogue keeps the world running"),F.World->IsPaused());
    const FVector InitialCamera=F.PC->NPCDialogueCamera->GetActorLocation();NPC->SetActorLocation(NPC->GetActorLocation()+FVector(30,0,0));F.PC->UpdateNPCDialogue(.2f);
    TestFalse(TEXT("Live dialogue camera follows NPC instead of freezing a frame"),F.PC->NPCDialogueCamera->GetActorLocation().Equals(InitialCamera,.1));
    F.Pawn->SetActorLocation(NPC->GetActorLocation()+FVector(1000,0,0));F.PC->UpdateNPCDialogue(.2f);
    TestTrue(TEXT("Walking beyond DT distance closes dialogue"),F.PC->Menu.IsEmpty());TestEqual(TEXT("Distance close restores gameplay camera"),F.PC->GetViewTarget(),static_cast<AActor*>(F.Pawn));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICFurnitureCollisionCheck,"IC.Life.Furniture.PlayerCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FICFurnitureCollisionCheck::RunTest(const FString&)
{
    ICFishingChecks::FSession F;
    auto* BedClass=LoadClass<AICLifeObjectActor>(nullptr,TEXT("/Game/IsekaiClass/NPC/Blueprints/Interaction/BP_IC_NPCBed.BP_IC_NPCBed_C"));
    auto* Bed=BedClass?F.World->SpawnActor<AICLifeObjectActor>(BedClass,FVector(5000,0,200),FRotator::ZeroRotator):nullptr;
    if(TestNotNull(TEXT("NPC bed WBP-independent interaction actor loads"),Bed))
    {
        TestEqual(TEXT("NPC bed blocks the player's capsule channel"),Bed->Shape->GetCollisionResponseToChannel(ECC_Pawn),ECR_Block);
        FHitResult Hit;const FVector Centre=Bed->Shape->Bounds.Origin;
        TestTrue(TEXT("Capsule sweep cannot pass through the NPC bed"),F.World->SweepSingleByChannel(Hit,Centre+FVector(400,0,0),Centre,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88))&&Hit.GetActor()==Bed);
    }
    for(const auto* Path:{TEXT("/Game/IsekaiClass/Life/Home/Blueprints/BP_IC_PlayerStorage.BP_IC_PlayerStorage_C"),TEXT("/Game/IsekaiClass/NPC/Blueprints/Containers/BP_IC_NPCChest.BP_IC_NPCChest_C")})
    {
        auto* Type=LoadClass<AActor>(nullptr,Path);auto* Chest=Type?F.World->SpawnActor<AActor>(Type,FVector(8000,0,200),FRotator::ZeroRotator):nullptr;
        if(!TestNotNull(TEXT("Storage chest loads"),Chest))continue;
        auto* Hinge=Chest->FindComponentByClass<UICChestPresentationComponent>();if(!TestNotNull(TEXT("Chest owns animated lid"),Hinge))continue;
        auto* Lid=Hinge->Lid.Get();TestTrue(TEXT("Lid collision is enabled"),Lid->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
        FCollisionQueryParams Query;TInlineComponentArray<UPrimitiveComponent*> Parts;Chest->GetComponents(Parts);
        for(auto* Part:Parts)if(Part!=Lid)Query.AddIgnoredComponent(Part);
        for(int32 Open=0;Open<2;++Open)
        {
            Hinge->SetRelativeRotation(Open?Hinge->OpenRotation:FRotator::ZeroRotator);
            const FVector Centre=Lid->Bounds.Origin;FHitResult Hit;
            TestTrue(Open?TEXT("Open lid blocks player sweeps"):TEXT("Closed lid blocks player sweeps"),F.World->SweepSingleByChannel(Hit,Centre+FVector(0,0,200),Centre,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(8),Query)&&Hit.GetComponent()==Lid);
        }
        Chest->Destroy();
    }
    return !HasAnyErrors();
}
#endif
