#include "Quest/ICQuestGuideComponent.h"

#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Debug/ICLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "ICQuestGuidePath.h"
#include "Quest/ICQuestGuideLight.h"
#include "Quest/ICQuestSubsystem.h"
#include "ICQuestLifeTarget.h"
#include "Life/World/ICLifeActor.h"
#include "Life/World/ICLifeTargetSubsystem.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Core/Controller/ICPlayerController.h"
#include "Time/ICTimeSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"

UICQuestGuideComponent::UICQuestGuideComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    GuideLightClass = AICQuestGuideLight::StaticClass();
}

void UICQuestGuideComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) return;
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        State = GI->GetSubsystem<UICWorldStateSubsystem>();
        if (State)
        {
            State->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshGuide);
            State->OnWorldRestored.AddUniqueDynamic(this, &ThisClass::HandleWorldRestored);
        }
    }
    GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &ThisClass::RefreshGuide, .5f, true, .1f);
}

void UICQuestGuideComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (State)
    {
        State->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshGuide);
        State->OnWorldRestored.RemoveDynamic(this, &ThisClass::HandleWorldRestored);
    }
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
    ResetPulse();
    State = nullptr;
    Super::EndPlay(Reason);
}

void UICQuestGuideComponent::SetQuestGuideEnabled(FName QuestID, bool bEnabled)
{
    if (QuestID.IsNone()) return;
    if (bEnabled)
    {
        bGuideEnabled = true;
        PausedQuestIDs.Remove(QuestID);
    }
    else PausedQuestIDs.Add(QuestID);
    if (GuidedQuestID == QuestID) ResetPulse();
    RefreshGuide();
}

void UICQuestGuideComponent::ResetPulse(bool bImmediate)
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PulseTimer);
    StopActiveLights(bImmediate);
    GuidedQuestID = NAME_None;
    GuidedTargetKey = NAME_None;
    NextPulseTime = 0;
}

void UICQuestGuideComponent::StopActiveLights(bool bImmediate)
{
    for (AICQuestGuideLight* Light : ActiveLights)
    {
        if (!IsValid(Light)) continue;
        Light->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleLightDestroyed);
        if (bImmediate)
        {
            Light->Stop();
            Light->Destroy();
        }
        else
        {
            Light->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleLightDestroyed);
            Light->FadeOut();
        }
    }
    if (bImmediate) ActiveLights.Reset();
}

void UICQuestGuideComponent::HandleLightDestroyed(AActor* Light)
{
    ActiveLights.RemoveAll([Light](const TObjectPtr<AICQuestGuideLight>& Entry)
    {
        return !IsValid(Entry) || Entry.Get() == Light;
    });
    if (ActiveLights.IsEmpty() && !GuidedQuestID.IsNone())
        SchedulePulse(FMath::IsFinite(RepeatInterval) ? FMath::Max(.1f, RepeatInterval) : .25f);
}

void UICQuestGuideComponent::HandleWorldRestored()
{
    ResetPulse();
    RefreshGuide();
}

bool UICQuestGuideComponent::ResolveTarget(const FICQuestRow& Definition, const FICQuestState& Quest,
    const FICWorldState& World, FVector& Location, FName& TargetKey) const
{
    UWorld* Level = GetWorld();
    UGameInstance* GI = Level ? Level->GetGameInstance() : nullptr;
    UICSpawnSubsystem* Spawn = Level ? Level->GetSubsystem<UICSpawnSubsystem>() : nullptr;
    UICQuestSubsystem* Quests = GI ? GI->GetSubsystem<UICQuestSubsystem>() : nullptr;
    if (!Spawn || !Quests) return false;
    auto ResolveAnchor = [Spawn, &Location](FName ID)
    {
        FTransform Transform;
        if (ID.IsNone() || !Spawn->ResolveLoadedAnchor(ID, Transform)) return false;
        Location = Transform.GetLocation();
        return !Location.ContainsNaN();
    };
    auto ResolveNPC = [Spawn, &Location](FName ID)
    {
        if (ID.IsNone()) return false;
        if (const AICCharacterBase* NPC = Spawn->FindCharacter(ID))
        {
            Location = NPC->GetActorLocation();
            return !Location.ContainsNaN();
        }
        return false;
    };
    const bool bReady = Quests->AreQuestObjectivesReady(Definition.ID);
    if (bReady && Definition.CompletionMode == EICQuestCompletionMode::SystemEvent) return false;
    if (bReady && Definition.CompletionMode == EICQuestCompletionMode::NPCInteraction)
    {
        if (ResolveNPC(Definition.TurnInNPCID))
        {
            TargetKey = Definition.TurnInNPCID;
            return true;
        }
        if (ResolveAnchor(Definition.TurnInAnchorID))
        {
            TargetKey = Definition.TurnInAnchorID;
            return true;
        }
        return false;
    }
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        TArray<bool, TInlineAllocator<8>> CompletedObjectives;
        bool bSatisfied = Group.Policy == EICConditionPolicy::All;
        for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
        {
            const bool bComplete = Quests->IsObjectiveSatisfied(Objective, Quest, World);
            CompletedObjectives.Add(bComplete);
            bSatisfied = Group.Policy == EICConditionPolicy::All ? bSatisfied && bComplete : bSatisfied || bComplete;
        }
        // Satisfying any group already satisfies an Any quest; wait for its normal completion.
        if (bSatisfied && Definition.ObjectiveGroupPolicy == EICConditionPolicy::Any) return false;
        if (bSatisfied) continue;
        for (int32 Index = 0; Index < Group.Objectives.Num(); ++Index)
        {
            if (CompletedObjectives[Index]) continue;
            const FICQuestObjectiveDefinition& Objective = Group.Objectives[Index];
            int32 Progress = 0;
            if (Objective.Type == EICQuestObjectiveType::DeliverItem
                && Quests->GetObjectiveProgress(Objective, Quest, World, Progress)
                && Progress >= Objective.RequiredCount) continue;
            TargetKey = Objective.ObjectiveID;
            // NPCs can leave their authored anchor as their schedule changes.
            // Follow the live actor first; the anchor is a fallback while it is unloaded.
            if ((Objective.Type == EICQuestObjectiveType::Talk || Objective.Type == EICQuestObjectiveType::Affinity)
                && Objective.Target.Type == EICDefinitionType::NPC)
            {
                if (ResolveNPC(Objective.Target.ID)) return true;
            }
            if (ResolveLifeTarget(Objective, World, Location, TargetKey)) return true;
            if (!Objective.GuidanceAnchorID.IsNone())
            {
                if (ResolveAnchor(Objective.GuidanceAnchorID)) return true;
            }
            if (Objective.Type == EICQuestObjectiveType::Visit && Objective.Target.Type == EICDefinitionType::Anchor)
            {
                if (ResolveAnchor(Objective.Target.ID)) return true;
            }
        }
    }
    return false;
}

bool UICQuestGuideComponent::ResolveLifeTarget(const FICQuestObjectiveDefinition& Objective,
    const FICWorldState& Snapshot, FVector& Location, FName& TargetKey) const
{
    if (Objective.Type != EICQuestObjectiveType::DeliverItem
        && Objective.Type != EICQuestObjectiveType::PossessItem
        && Objective.Type != EICQuestObjectiveType::Collect
        && Objective.Type != EICQuestObjectiveType::Craft) return false;
    UWorld* World = GetWorld();
    APawn* Pawn = Cast<APawn>(GetOwner());
    auto* Definitions = World && World->GetGameInstance()
        ? World->GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    auto* Targets = World ? World->GetSubsystem<UICLifeTargetSubsystem>() : nullptr;
    auto* Controller = Pawn ? Cast<AICPlayerController>(Pawn->GetController()) : nullptr;
    if (!Definitions || !Targets || !Pawn) return false;

    struct FCandidate { AICLifeActor* Actor; FVector Goal; double Distance; };
    TArray<FCandidate> Candidates;
    TArray<FName> CropIDs;
    // Planting definitions come from the catalog, not a plot's last planted crop.
    if (Objective.Type == EICQuestObjectiveType::Craft
        && (!Objective.bFilterLifeActivity || Objective.LifeActivity == EICLifeActivity::Planting))
        Definitions->GetDefinitionIDs(EICDefinitionType::Crop, CropIDs);
    for (const auto& Weak : Targets->GetTargets())
    {
        AICLifeActor* Actor = Weak.Get();
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed()) continue;
        bool bMatches = false;
        FICDefinitionResult Result;
        if (Actor->LifeType == EICDefinitionType::Resource)
        {
            if (const auto* Row = Definitions->Find<FICResourceRow>(Actor->DefinitionID, Result))
                bMatches = ICQuestLifeTarget::Matches(Objective, Actor->LifeType, Actor->DefinitionID,
                    Row->ProductionID, Row->Activity, Row->Yields) && Actor->IsResourceAvailable();
        }
        else if (Actor->LifeType == EICDefinitionType::Crop)
        {
            const FName PointID = Actor->PointID.IsNone() ? Actor->GetFName() : Actor->PointID;
            const FICCropState* Planted = Snapshot.Crops.Find(PointID);
            const FName CropID = Planted ? Planted->CropID : Actor->DefinitionID;
            if (Planted && !CropID.IsNone())
            {
                if (const auto* Row = Definitions->Find<FICCropRow>(CropID, Result))
                    bMatches = ICQuestLifeTarget::Matches(Objective, EICDefinitionType::Crop, CropID,
                        Row->HarvestProductionID, EICLifeActivity::Harvesting, Row->HarvestYields)
                        && Actor->IsHarvestable();
            }
            if (!bMatches && Actor->bPlantingPlot && !Planted && Controller)
            {
                for (FName ID : CropIDs)
                {
                    const auto* Row = Definitions->Find<FICCropRow>(ID, Result);
                    FText Failure;
                    if (Row && ICQuestLifeTarget::Matches(Objective, EICDefinitionType::Crop, ID,
                        Row->PlantingProductionID, EICLifeActivity::Planting, {})
                        && Actor->CanPlantCrop(Controller, ID, Failure))
                    { bMatches = true; break; }
                }
            }
        }
        else if (Actor->LifeType == EICDefinitionType::FishingPoint)
        {
            if (const auto* Row = Definitions->Find<FICFishingPointRow>(Actor->DefinitionID, Result))
            {
                if (Row->SuccessChance <= 0) continue;
                for (const auto& Selection : Row->FishSelections)
                {
                    if (Selection.Weight <= 0 || Selection.CatchCount <= 0) continue;
                    const auto* Fish = Definitions->Find<FICFishRow>(Selection.FishID, Result);
                    if (!Fish) continue;
                    FICItemAmount Yield; Yield.ItemID = Fish->ItemID; Yield.Count = Selection.CatchCount;
                    const TArray<FICItemAmount> Yields = {Yield};
                    bMatches = ICQuestLifeTarget::Matches(Objective, Actor->LifeType, Actor->DefinitionID,
                        Row->ProductionID, EICLifeActivity::Fishing, Yields)
                        || ICQuestLifeTarget::Matches(Objective, EICDefinitionType::Fish, Selection.FishID,
                            Row->ProductionID, EICLifeActivity::Fishing, Yields);
                    if (bMatches) break;
                }
            }
        }
        if (!bMatches) continue;
        if (Actor->LifeType != EICDefinitionType::FishingPoint)
        {
            const FVector Goal = Actor->GetActorLocation();
            if (!Goal.ContainsNaN()) Candidates.Add({Actor, Goal, FVector::DistSquared(Pawn->GetActorLocation(), Goal)});
            continue;
        }
        // Water centers need not be walkable. Probe the water perimeter and project onto shore.
        auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        if (!Navigation) continue;
        const auto* Player = Cast<AICPlayerCharacter>(Pawn);
        const double CastRange = Player && Player->LifeActions ? Player->LifeActions->FishingCastRangeCm : 1200.;
        TArray<FVector> WaterSamples;
        Actor->GetFishingGuideSamples(Pawn->GetActorLocation(), WaterSamples);
        for (const FVector& Water : WaterSamples)
        {
            if (!Actor->ContainsFishingLanding(Water)) continue;
            FNavLocation Shore;
            const FNavAgentProperties& Agent = Pawn->GetNavAgentPropertiesRef();
            if (!Navigation->ProjectPointToNavigation(Water, Shore,
                FVector(400, 400, 1000), &Agent)) continue;
            if (FVector::DistSquared(Shore.Location, Water) > FMath::Square(CastRange)) continue;
            Candidates.Add({Actor, Shore.Location, FVector::DistSquared(Pawn->GetActorLocation(), Shore.Location)});
        }
    }
    Candidates.Sort([](const FCandidate& A, const FCandidate& B)
    {
        return A.Distance == B.Distance ? A.Actor->GetPathName() < B.Actor->GetPathName() : A.Distance < B.Distance;
    });
    for (const FCandidate& Candidate : Candidates)
    {
        TArray<FVector> Points;
        if (Candidate.Distance > FMath::Square(FMath::Max(0.f, ArrivalRadius))
            && !BuildGuidePath(*Pawn, Candidate.Goal, Points)) continue;
        Location = Candidate.Goal;
        TargetKey = FName(*(Objective.ObjectiveID.ToString() + TEXT(":") + Candidate.Actor->GetPathName()));
        return true;
    }
    return false;
}

bool UICQuestGuideComponent::SelectTarget(const FICWorldState& Snapshot, UICDefinitionSubsystem& Definitions,
    FName& QuestID, FName& TargetKey, FVector& Location) const
{
    QuestID = TargetKey = NAME_None;
    Location = FVector::ZeroVector;
    TArray<FName> Candidates;
    for (const auto& Entry : Snapshot.Quests)
    {
        if (IsQuestGuideEnabled(Entry.Key) && Entry.Value.bAccepted && Entry.Value.bStarted && !Entry.Value.bCompleted && !Entry.Value.bAbandoned)
            Candidates.Add(Entry.Key);
    }
    Candidates.Sort([&Snapshot](FName A, FName B)
    {
        const double AcceptedA = Snapshot.Quests.FindChecked(A).AcceptedAtGameMinute;
        const double AcceptedB = Snapshot.Quests.FindChecked(B).AcceptedAtGameMinute;
        return AcceptedA == AcceptedB ? A.LexicalLess(B) : AcceptedA > AcceptedB;
    });
    for (FName ID : Candidates)
    {
        FICQuestRow Definition;
        if (Definitions.GetQuest(ID, Definition).bSuccess
            && ResolveTarget(Definition, Snapshot.Quests.FindChecked(ID), Snapshot, Location, TargetKey))
        {
            QuestID = ID;
            break;
        }
    }
    return !QuestID.IsNone();
}

bool UICQuestGuideComponent::BuildGuidePath(APawn& Pawn, const FVector& Goal, TArray<FVector>& Points) const
{
    Points.Reset();
    UWorld* World = GetWorld();
    if (!World) return false;
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!Navigation)
    {
        UE_LOG(LogICWorldState, Verbose, TEXT("[Quest.Guide.NoNavigation] Quest=%s World=%s"),
            *GuidedQuestID.ToString(), *World->GetName());
        return false;
    }
    FNavLocation Start, End;
    if (NavigationProjectionExtent.ContainsNaN() || NavigationProjectionExtent.GetMin() <= 0) return false;
    const FVector Extent = NavigationProjectionExtent;
    const FNavAgentProperties& Agent = Pawn.GetNavAgentPropertiesRef();
    const ARecastNavMesh* Mesh = Cast<ARecastNavMesh>(Navigation->GetNavDataForProps(Agent, Pawn.GetNavAgentLocation()));
    auto Project = [&](const FVector& Location, FNavLocation& Result)
    {
        if (Navigation->ProjectPointToNavigation(Location, Result, Extent, &Agent)) return true;
        if (!bBridgeNavigationSteps || !Mesh) return false;
        // A visual guide can originate above/below a NavMesh layer. Preserve the
        // narrow horizontal extent so a nearby wall does not select a distant floor.
        const FBox Bounds = Mesh->GetNavMeshBounds();
        if (!Bounds.IsValid) return false;
        const double Height = FMath::Max(FMath::Abs(Location.Z - Bounds.Min.Z), FMath::Abs(Location.Z - Bounds.Max.Z)) + 1.;
        return Navigation->ProjectPointToNavigation(Location, Result, FVector(Extent.X, Extent.Y, Height), &Agent);
    };
    if (!Project(Pawn.GetNavAgentLocation(), Start) || !Project(Goal, End))
    {
        UE_LOG(LogICWorldState, Verbose, TEXT("[Quest.Guide.OffNavMesh] Quest=%s Target=%s Goal=%s"),
            *GuidedQuestID.ToString(), *GuidedTargetKey.ToString(), *Goal.ToCompactString());
        return false;
    }
    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Start.Location, End.Location, &Pawn);
    if (Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num() >= 2)
        Points = Path->PathPoints;
    else if (bBridgeNavigationSteps && Mesh)
        ICQuestGuidePath::FindAcrossSteps(*World, *Mesh, Start, End, StepBridgeMaxHorizontalGap,
            GuideHeight, &Pawn, Points);
    if (Points.Num() < 2)
    {
        UE_LOG(LogICWorldState, Verbose, TEXT("[Quest.Guide.NoPath] Quest=%s Target=%s"),
            *GuidedQuestID.ToString(), *GuidedTargetKey.ToString());
        return false;
    }
    return true;
}

void UICQuestGuideComponent::SpawnPulse(APawn& Pawn, const TArray<FVector>& Points)
{
    UWorld* World = GetWorld();
    if (!World || Points.Num() < 2) return;
    if (!GuideLightClass || GuideLightClass->HasAnyClassFlags(CLASS_Abstract)) return;
    FActorSpawnParameters Parameters;
    Parameters.Owner = &Pawn;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // The previous route, including its fading tail, must be gone first.
    if (!ActiveLights.IsEmpty()) return;
    AICQuestGuideLight* Light = World->SpawnActor<AICQuestGuideLight>(
        GuideLightClass, Points[0], FRotator::ZeroRotator, Parameters);
    if (IsValid(Light))
    {
        Light->TailLength = TailLength;
        Light->FadeOutDuration = FadeOutDuration;
        Light->PlayPath(Points, GuideDistance, GuideSpeed, GuideHeight);
        if (IsValid(Light))
        {
            ActiveLights.Add(Light);
            Light->OnDestroyed.AddUniqueDynamic(this, &ThisClass::HandleLightDestroyed);
        }
    }
}

void UICQuestGuideComponent::SchedulePulse(float Delay)
{
    UWorld* World = GetWorld();
    if (!World || !FMath::IsFinite(Delay) || Delay <= 0) return;
    NextPulseTime = World->GetTimeSeconds() + Delay;
    World->GetTimerManager().SetTimer(PulseTimer, this, &ThisClass::RefreshGuide, Delay, false);
}

void UICQuestGuideComponent::RefreshGuide()
{
    ActiveLights.RemoveAll([](const TObjectPtr<AICQuestGuideLight>& Light) { return !IsValid(Light); });
    APawn* Pawn = Cast<APawn>(GetOwner());
    UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    UICQuestSubsystem* Quests = GI ? GI->GetSubsystem<UICQuestSubsystem>() : nullptr;
    UICDefinitionSubsystem* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    UICTimeSubsystem* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr;
    if (!bGuideEnabled || !Pawn || !Pawn->IsLocallyControlled() || !State || !Definitions || !Quests
        || !Time || !Time->IsRunning())
    {
        ResetPulse();
        return;
    }
    const FICWorldState Snapshot = State->ExportState();
    if (!Snapshot.bPlayerCreated)
    {
        ResetPulse();
        return;
    }
    FName SelectedQuest, TargetKey;
    FVector Goal;
    const bool bHasTarget = SelectTarget(Snapshot, *Definitions, SelectedQuest, TargetKey, Goal);
    if (!bHasTarget)
    {
        // A completed quest may still have fading pulses. Do not cut that fade
        // short on the next refresh after clearing the tracked quest ID.
        if (!GuidedQuestID.IsNone())
        {
            const FICQuestState* Previous = Snapshot.Quests.Find(GuidedQuestID);
            ResetPulse(!(Previous && Previous->bCompleted));
        }
        return;
    }
    if (GuidedQuestID != SelectedQuest || GuidedTargetKey != TargetKey)
    {
        ResetPulse();
        GuidedQuestID = SelectedQuest;
        GuidedTargetKey = TargetKey;
    }
    const double Now = World->GetTimeSeconds();
    if (FVector::DistSquared(Pawn->GetActorLocation(), Goal) <= FMath::Square(FMath::Max(0.f, ArrivalRadius)))
    {
        StopActiveLights(false);
        World->GetTimerManager().ClearTimer(PulseTimer);
        NextPulseTime = 0;
        return;
    }
    if (!ActiveLights.IsEmpty()) return;
    if (Now < NextPulseTime)
    {
        // The dedicated timer preserves intervals shorter than the state poll.
        if (!World->GetTimerManager().IsTimerActive(PulseTimer))
            SchedulePulse(FMath::Max(.001f, float(NextPulseTime - Now)));
        return;
    }
    // Clamp both at authoring time and runtime. Invalid values must not create a busy loop.
    const float Interval = FMath::IsFinite(RepeatInterval) ? FMath::Max(.1f, RepeatInterval) : .25f;
    SchedulePulse(Interval);
    TArray<FVector> Points;
    if (BuildGuidePath(*Pawn, Goal, Points)) SpawnPulse(*Pawn, Points);
}
