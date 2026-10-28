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
        if (bImmediate)
        {
            Light->Stop();
            Light->Destroy();
        }
        else Light->FadeOut();
    }
    if (bImmediate) ActiveLights.Reset();
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
            if (!Objective.GuidanceAnchorID.IsNone())
            {
                if (ResolveAnchor(Objective.GuidanceAnchorID)) return true;
            }
            if ((Objective.Type == EICQuestObjectiveType::Talk || Objective.Type == EICQuestObjectiveType::Affinity)
                && Objective.Target.Type == EICDefinitionType::NPC)
            {
                if (ResolveNPC(Objective.Target.ID)) return true;
            }
            else if (Objective.Type == EICQuestObjectiveType::Visit && Objective.Target.Type == EICDefinitionType::Anchor)
            {
                if (ResolveAnchor(Objective.Target.ID)) return true;
            }
        }
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
    // Every pulse owns its route and Niagara simulation. Older pulses continue
    // along their original route until their own distance limit is reached.
    AICQuestGuideLight* Light = World->SpawnActor<AICQuestGuideLight>(
        GuideLightClass, Points[0], FRotator::ZeroRotator, Parameters);
    if (IsValid(Light))
    {
        Light->TailLength = TailLength;
        Light->FadeOutDuration = FadeOutDuration;
        Light->PlayPath(Points, GuideDistance, GuideSpeed, GuideHeight);
        if (IsValid(Light)) ActiveLights.Add(Light);
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
    if (Now < NextPulseTime)
    {
        // The dedicated timer preserves intervals shorter than the state poll.
        if (!World->GetTimerManager().IsTimerActive(PulseTimer))
            SchedulePulse(FMath::Max(.001f, float(NextPulseTime - Now)));
        return;
    }
    // Clamp both at authoring time and runtime. Invalid values must not create a busy loop.
    const float Interval = FMath::IsFinite(RepeatInterval) ? FMath::Max(.1f, RepeatInterval) : 3.f;
    SchedulePulse(Interval);
    TArray<FVector> Points;
    if (BuildGuidePath(*Pawn, Goal, Points)) SpawnPulse(*Pawn, Points);
}
