#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Components/PointLightComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraDataSetAccessor.h"
#include "Misc/App.h"
#include "NiagaraRibbonRendererProperties.h"
#include "Quest/ICQuestGuideLight.h"
#include "Quest/ICQuestGuidePath.h"
#include "Components/BoxComponent.h"

namespace
{
    // Intentional integration-test fixture; production code uses Blueprint references.
    UClass* LoadGuideFixtureClass()
    {
        constexpr TCHAR GuideFixtureClassPath[] = TEXT("/Game/IsekaiClass/Quest/BP_ICQuestGuideLight.BP_ICQuestGuideLight_C");
        return LoadClass<AICQuestGuideLight>(nullptr, GuideFixtureClassPath);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestGuidePulseCheck, "IC.Runtime.QuestGuidePulse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestGuidePulseCheck::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world exists"), World)) return false;
    AICQuestGuideLight* Light = World->SpawnActor<AICQuestGuideLight>();
    if (!TestNotNull(TEXT("Guide light spawns"), Light))
    {
        World->DestroyWorld(false);
        return false;
    }
    const TArray<FVector> Points = {FVector::ZeroVector, FVector(300, 0, 0), FVector(300, 400, 0)};
    Light->PlayPath(Points, 500, 100, 50, false);
    UNiagaraComponent* Trail = Cast<UNiagaraComponent>(Light->GetDefaultSubobjectByName(TEXT("TrailEffect")));
    if (!TestNotNull(TEXT("Guide has a Niagara component"), Trail))
    {
        World->DestroyWorld(false);
        return false;
    }
    TestTrue(TEXT("Pulse starts visibly"), Light->IsPlaying() && !Light->IsHidden());
    TestTrue(TEXT("The selected trail is the single visible guide system"), Trail->IsVisible());
    Light->Tick(4);
    TestTrue(TEXT("Head follows navigation corners"), Light->GetActorLocation().Equals(FVector(300, 100, 50), .01));
    TestNull(TEXT("The head is a VFX, not a real scene light"), Light->FindComponentByClass<UPointLightComponent>());
    bool bLifetimeValid = false;
    const float Lifetime = Trail->GetVariableFloat(TEXT("User.TrailLifetime"), bLifetimeValid);
    TestTrue(TEXT("Tail lifetime derives from its configured length and travel speed"), bLifetimeValid && FMath::IsNearlyEqual(Lifetime, Light->TailLength / 100.f));
    Light->Tick(1);
    TestTrue(TEXT("Endpoint begins a visible fade instead of popping off"), Light->IsFadingOut() && !Light->IsHidden());
    TestTrue(TEXT("Travel is capped at the configured distance"), Light->GetActorLocation().Equals(FVector(300, 200, 50), .01));
    Light->Tick(.3f);
    TestTrue(TEXT("Fade remains active halfway through"), Light->IsFadingOut() && !Light->IsHidden());
    Light->Tick(.31f);
    TestTrue(TEXT("Fade eventually removes all visuals"), Light->IsHidden() && !Light->IsPlaying() && !Trail->IsActive());
    Light->PlayPath(Points, 1000, 100, 50, false);
    Light->Tick(10);
    TestTrue(TEXT("Long frames account for travel and fade time"), Light->GetActorLocation().Equals(FVector(300, 400, 50), .01) && Light->IsHidden());
    Light->PlayPath(Points, 500, 0, 50, false);
    TestFalse(TEXT("Zero speed is rejected"), Light->IsPlaying());
    Light->PlayPath(Points, -1, 100, 50, false);
    TestFalse(TEXT("Negative distance is rejected"), Light->IsPlaying());
    Light->PlayPath({FVector::ZeroVector, FVector::ZeroVector, FVector(100, 0, 0)}, 500, 100, 50, false);
    Light->Tick(2);
    TestTrue(TEXT("Duplicate path nodes are safe"), Light->IsHidden());
    Light->PlayPath(Points, 500, 100, 50, false);
    Light->Tick(2);
    Light->FadeOut();
    Light->Tick(.2f);
    Light->PlayPath({FVector(1000, 0, 0), FVector(1000, 500, 0)}, 500, 100, 50, false);
    Light->Tick(1);
    TestTrue(TEXT("New head follows the redirected route"), Light->GetActorLocation().Equals(FVector(1000, 100, 50), .01f));
    Light->Stop();
    TestTrue(TEXT("Quest abandonment removes visuals immediately, including a fade"), Light->IsHidden() && !Light->IsPlaying() && !Trail->IsActive());
    World->DestroyWorld(false);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestGuideIndependentPulsesCheck, "IC.Runtime.QuestGuideIndependentPulses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestGuideIndependentPulsesCheck::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world exists"), World)) return false;
    AICQuestGuideLight* First = World->SpawnActor<AICQuestGuideLight>();
    AICQuestGuideLight* Second = World->SpawnActor<AICQuestGuideLight>();
    if (!TestNotNull(TEXT("First pulse spawns"), First) || !TestNotNull(TEXT("Second pulse spawns"), Second))
    {
        World->DestroyWorld(false);
        return false;
    }
    First->PlayPath({FVector::ZeroVector, FVector(300, 0, 0), FVector(300, 400, 0)}, 500, 100, 50);
    First->Tick(1.f);
    // A later pulse begins from a different player location while the first is active.
    Second->PlayPath({FVector(0, 200, 0), FVector(400, 200, 0)}, 200, 100, 50);
    TestTrue(TEXT("Two interval pulses can travel concurrently"), First->IsPlaying() && Second->IsPlaying());
    First->Tick(2.f);
    Second->Tick(2.f);
    TestTrue(TEXT("A newer route does not redirect the older pulse"), First->GetActorLocation().Equals(FVector(300, 0, 50), .01f));
    TestTrue(TEXT("New pulse stops at its own distance limit"), Second->IsFadingOut()
        && Second->GetActorLocation().Equals(FVector(200, 200, 50), .01f));
    Second->Tick(1.f);
    TestTrue(TEXT("An expired pulse destroys its actor instead of leaving a hidden instance"), Second->IsActorBeingDestroyed());
    TestFalse(TEXT("Expiring one pulse does not destroy the other"), First->IsActorBeingDestroyed());
    First->Tick(2.f);
    TestTrue(TEXT("Distance limit counts the bent route, not straight-line displacement"), First->IsFadingOut()
        && First->GetActorLocation().Equals(FVector(300, 200, 50), .01f));
    First->Tick(1.f);
    TestTrue(TEXT("The older pulse also destroys itself after its own fade"), First->IsActorBeingDestroyed());
    AICQuestGuideLight* Invalid = World->SpawnActor<AICQuestGuideLight>();
    if (TestNotNull(TEXT("Invalid pulse can be checked"), Invalid))
    {
        Invalid->PlayPath({FVector::ZeroVector, FVector(100, 0, 0)}, 500, 0, 50);
        TestTrue(TEXT("Invalid pulse settings do not leak a spawned actor"), Invalid->IsActorBeingDestroyed());
    }
    World->DestroyWorld(false);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestGuideAssetsCheck, "IC.Runtime.QuestGuideAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestGuideAssetsCheck::RunTest(const FString& Parameters)
{
    UGameInstance* GI = NewObject<UGameInstance>(GEngine);
    GI->InitializeStandalone();
    UICDefinitionSubsystem* Definitions = GI->GetSubsystem<UICDefinitionSubsystem>();
    FICQuestRow Quest;
    if (TestTrue(TEXT("Runtime catalog contains the navigation quest"),
        Definitions && Definitions->GetQuest(TEXT("QST_TEST_VISIT_001"), Quest).bSuccess))
    {
        // Follow the configured destination; merged content may relocate the test quest.
        bool bHasVisitGoal = false;
        for (const FICQuestObjectiveGroup& Group : Quest.ObjectiveGroups)
        {
            for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
            {
                if (Objective.Type != EICQuestObjectiveType::Visit) continue;
                const FName AnchorID = !Objective.GuidanceAnchorID.IsNone()
                    ? Objective.GuidanceAnchorID : Objective.Target.ID;
                FICAnchorRow Goal;
                TestTrue(TEXT("Configured visit destination exists in the runtime catalog"),
                    !AnchorID.IsNone() && Definitions->GetAnchor(AnchorID, Goal).bSuccess);
                bHasVisitGoal = true;
            }
        }
        TestTrue(TEXT("Navigation fixture contains a visit destination"), bHasVisitGoal);
    }
    UClass* GuideClass = LoadGuideFixtureClass();
    AICQuestGuideLight* Defaults = GuideClass ? GuideClass->GetDefaultObject<AICQuestGuideLight>() : nullptr;
    const UNiagaraComponent* Trail = Defaults ? Cast<UNiagaraComponent>(Defaults->GetDefaultSubobjectByName(TEXT("TrailEffect"))) : nullptr;
    if (TestNotNull(TEXT("Independent trail asset exists"), Trail ? Trail->GetAsset() : nullptr))
    {
        TestEqual(TEXT("The default guide uses the authored wisp"), Trail->GetAsset()->GetFName(), FName(TEXT("NS_IC_QuestGuideWisp")));
        TestEqual(TEXT("The wisp combines one core and one history emitter"), Trail->GetAsset()->GetEmitterHandles().Num(), 2);
        for (const auto& Handle : Trail->GetAsset()->GetEmitterHandles())
        {
            const auto* Data = Handle.GetInstance().GetEmitterData();
            const bool bCore = Handle.GetName() == TEXT("Core");
            TestTrue(TEXT("Only the core follows the emitter transform"), Data && Data->bLocalSpace == bCore);
            bool bHasRibbon = false;
            if (Data) for (const auto* Renderer : Data->GetRenderers()) bHasRibbon |= Renderer->IsA<UNiagaraRibbonRendererProperties>();
            TestTrue(TEXT("Only the world-space history has a ribbon renderer"), bHasRibbon == !bCore);
        }
    }
    UWorld* World = GI->GetWorld();
    GI->Shutdown();
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestGuideParticleHistoryCheck, "IC.VFX.QuestGuideParticleHistory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestGuideParticleHistoryCheck::RunTest(const FString& Parameters)
{
    if (!FApp::CanEverRender())
    {
        AddError(TEXT("Particle history must be checked with rendering enabled, without -nullrhi."));
        return false;
    }
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    UClass* GuideClass = LoadGuideFixtureClass();
    AICQuestGuideLight* Light = GuideClass ? World->SpawnActor<AICQuestGuideLight>(GuideClass) : nullptr;
    UNiagaraComponent* Trail = Light ? Cast<UNiagaraComponent>(Light->GetDefaultSubobjectByName(TEXT("TrailEffect"))) : nullptr;
    if (TestNotNull(TEXT("Trail component exists"), Trail))
    {
        Trail->SetForceSolo(true);
        Light->TailLength = 300;
        Light->PlayPath({FVector::ZeroVector, FVector(100, 0, 0), FVector(100, 500, 0)}, 600, 600, 50, false);
        for (int32 Index = 0; Index < 3; ++Index)
        {
            Light->Tick(.1f);
            Trail->AdvanceSimulation(6, 1.f / 60.f);
        }
        auto Controller = Trail->GetSystemInstanceController();
        if (TestTrue(TEXT("Niagara simulation is active"), Controller.IsValid()))
        {
            Controller->WaitForConcurrentTickAndFinalize();
            auto* Instance = Controller->GetSystemInstance_Unsafe();
            if (Instance && Instance->GetEmitters().Num() == 2)
            {
                const auto& Emitter = Instance->GetEmitters()[1];
                const auto& DataSet = Emitter->GetParticleData();
                auto Positions = FNiagaraDataSetAccessor<FNiagaraPosition>::CreateReader(DataSet, TEXT("Position"));
                auto Lifetimes = FNiagaraDataSetAccessor<float>::CreateReader(DataSet, TEXT("Lifetime"));
                auto Ages = FNiagaraDataSetAccessor<float>::CreateReader(DataSet, TEXT("Age"));
                TestTrue(TEXT("Particle position and age data are readable"), Positions.IsValid() && Ages.IsValid());
                bool bOldPositionRemains = false;
                if (Positions.IsValid() && Ages.IsValid())
                    for (int32 Index = 0; Index < Emitter->GetNumParticles(); ++Index)
                    {
                        const FVector3f Position = Positions.Get(Index);
                        if (Lifetimes.IsValid()) TestTrue(TEXT("User trail lifetime controls particle expiry"), FMath::IsNearlyEqual(Lifetimes.Get(Index), .5f, .01f));
                        bOldPositionRemains |= Ages.Get(Index) > .15f && FMath::Abs(Position.Y) < 1.f && Position.X > 1.f && Position.X < 95.f;
                    }
                TestTrue(TEXT("Particles emitted before the corner stay on the old segment after the head turns"), bOldPositionRemains);
                const auto& Core = Instance->GetEmitters()[0];
                auto CorePositions = FNiagaraDataSetAccessor<FNiagaraPosition>::CreateReader(Core->GetParticleData(), TEXT("Position"));
                TestTrue(TEXT("The core has live particles"), Core->GetNumParticles() > 0 && CorePositions.IsValid());
                if (CorePositions.IsValid())
                    for (int32 Index = 0; Index < Core->GetNumParticles(); ++Index)
                        TestTrue(TEXT("The core stays at the moving origin instead of branching off"), FVector3f(CorePositions.Get(Index)).IsNearlyZero(.01f));
            }
            else AddError(TEXT("Trail emitter did not create particle data."));
        }
        Light->Stop();
    }
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FICQuestGuideStepBridgeCheck, "IC.Runtime.QuestGuideStepBridge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FICQuestGuideStepBridgeCheck::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Step bridge test world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    TArray<FVector> Points;
    const FVector Lower(-60, 0, 0), Upper(60, 0, 900);
    TestTrue(TEXT("Visual bridge ignores a 900 cm step height"),
        ICQuestGuidePath::BuildStepBridge(*World, Lower, Upper, 50, nullptr, Points));
    TestTrue(TEXT("Ascending bridge rises beside the step before crossing"),
        Points.Num() == 3 && Points[0].Equals(Lower) && Points[1].Equals(FVector(-60, 0, 900)) && Points.Last().Equals(Upper));
    TestTrue(TEXT("The same step can be crossed downwards"),
        ICQuestGuidePath::BuildStepBridge(*World, Upper, Lower, 50, nullptr, Points));
    TestTrue(TEXT("Descending bridge crosses before descending"),
        Points.Num() == 3 && Points[1].Equals(FVector(-60, 0, 900)) && Points.Last().Equals(Lower));

    AActor* Obstacle = World->SpawnActor<AActor>();
    if (TestNotNull(TEXT("Collision obstacle spawns"), Obstacle))
    {
        UBoxComponent* Box = NewObject<UBoxComponent>(Obstacle);
        Obstacle->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(100, 100, 10));
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->RegisterComponentWithWorld(World);
        Obstacle->SetActorLocation(FVector(-60, 0, 400));
        TestFalse(TEXT("A ceiling blocks the vertical bridge"),
            ICQuestGuidePath::BuildStepBridge(*World, Lower, Upper, 50, nullptr, Points));
        TestTrue(TEXT("Rejected bridges leave no partial route"), Points.IsEmpty());
        Box->SetBoxExtent(FVector(10, 100, 100));
        Obstacle->SetActorLocation(FVector(0, 0, 50));
        TestFalse(TEXT("A horizontal wall is never replaced by a direct guide line"),
            ICQuestGuidePath::BuildStepBridge(*World, FVector(-60, 0, 0), FVector(60, 0, 0), 50, nullptr, Points));
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        TestTrue(TEXT("Removing the obstruction restores the bridge"),
            ICQuestGuidePath::BuildStepBridge(*World, Lower, Upper, 50, nullptr, Points));
    }
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
