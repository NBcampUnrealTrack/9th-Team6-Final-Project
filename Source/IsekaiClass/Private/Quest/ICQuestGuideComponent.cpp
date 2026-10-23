#include "Quest/ICQuestGuideComponent.h"

#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Debug/ICLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
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

void UICQuestGuideComponent::ResetPulse(bool bImmediate)
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PulseTimer);
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
    GuidedQuestID = NAME_None;
    GuidedTargetKey = NAME_None;
    NextPulseTime = 0;
}

void UICQuestGuideComponent::HandleWorldRestored()
{
    ResetPulse();
    RefreshGuide();
}

bool UICQuestGuideComponent::ResolveTarget(const FICQuestRow& Definition, const FICQuestState& Quest,
    const FICWorldState& World, FVector& Location, FName& TargetKey) const
{
    UICSpawnSubsystem* Spawn = GetWorld()->GetSubsystem<UICSpawnSubsystem>();
    UICQuestSubsystem* Quests = GetWorld()->GetGameInstance()->GetSubsystem<UICQuestSubsystem>();
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
        TargetKey = Definition.TurnInNPCID;
        return ResolveNPC(Definition.TurnInNPCID) || ResolveAnchor(Definition.TurnInAnchorID);
    }
    for (const FICQuestObjectiveGroup& Group : Definition.ObjectiveGroups)
    {
        bool bSatisfied = Group.Policy == EICConditionPolicy::All;
        for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
        {
            const bool bComplete = Quests->IsObjectiveSatisfied(Objective, Quest, World);
            bSatisfied = Group.Policy == EICConditionPolicy::All ? bSatisfied && bComplete : bSatisfied || bComplete;
        }
        // Satisfying any group already satisfies an Any quest; wait for its normal completion.
        if (bSatisfied && Definition.ObjectiveGroupPolicy == EICConditionPolicy::Any) return false;
        if (bSatisfied) continue;
        for (const FICQuestObjectiveDefinition& Objective : Group.Objectives)
        {
            if (Quests->IsObjectiveSatisfied(Objective, Quest, World)) continue;
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
    TArray<FName> Candidates;
    for (const auto& Entry : Snapshot.Quests)
    {
        if (Entry.Value.bAccepted && !Entry.Value.bCompleted) Candidates.Add(Entry.Key);
    }
    Candidates.Sort([&Snapshot](FName A, FName B)
    {
        const double AcceptedA = Snapshot.Quests.FindChecked(A).AcceptedAtGameMinute;
        const double AcceptedB = Snapshot.Quests.FindChecked(B).AcceptedAtGameMinute;
        return AcceptedA == AcceptedB ? A.LexicalLess(B) : AcceptedA > AcceptedB;
    });
    FName SelectedQuest, TargetKey;
    FVector Goal = FVector::ZeroVector;
    for (FName ID : Candidates)
    {
        FICQuestRow Definition;
        if (Definitions->GetQuest(ID, Definition).bSuccess
            && ResolveTarget(Definition, Snapshot.Quests.FindChecked(ID), Snapshot, Goal, TargetKey))
        {
            SelectedQuest = ID;
            break;
        }
    }
    if (SelectedQuest.IsNone())
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
        for (AICQuestGuideLight* Light : ActiveLights)
            if (IsValid(Light)) Light->FadeOut();
        World->GetTimerManager().ClearTimer(PulseTimer);
        NextPulseTime = 0;
        return;
    }
    if (Now < NextPulseTime)
    {
        // The dedicated timer preserves intervals shorter than the state poll.
        if (!World->GetTimerManager().IsTimerActive(PulseTimer))
            World->GetTimerManager().SetTimer(PulseTimer, this, &ThisClass::RefreshGuide,
                FMath::Max(.001f, float(NextPulseTime - Now)), false);
        return;
    }
    // Clamp both at authoring time and runtime. Invalid values must not create a busy loop.
    const float Interval = FMath::IsFinite(RepeatInterval) ? FMath::Max(.1f, RepeatInterval) : 3.f;
    NextPulseTime = Now + Interval;
    World->GetTimerManager().SetTimer(PulseTimer, this, &ThisClass::RefreshGuide, Interval, false);
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!Navigation)
    {
        UE_LOG(LogICWorldState, Verbose, TEXT("[Quest.Guide.NoNavigation] Quest=%s World=%s"),
            *SelectedQuest.ToString(), *World->GetName());
        return;
    }
    FNavLocation Start, End;
    const FVector Extent(100, 100, 200);
    const FNavAgentProperties& Agent = Pawn->GetNavAgentPropertiesRef();
    if (!Navigation->ProjectPointToNavigation(Pawn->GetNavAgentLocation(), Start, Extent, &Agent)
        || !Navigation->ProjectPointToNavigation(Goal, End, Extent, &Agent))
    {
        UE_LOG(LogICWorldState, Verbose, TEXT("[Quest.Guide.OffNavMesh] Quest=%s Target=%s Goal=%s"),
            *SelectedQuest.ToString(), *TargetKey.ToString(), *Goal.ToCompactString());
        return;
    }
    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Start.Location, End.Location, Pawn);
    if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num() < 2)
    {
        UE_LOG(LogICWorldState, Verbose, TEXT("[Quest.Guide.NoPath] Quest=%s Target=%s"),
            *SelectedQuest.ToString(), *TargetKey.ToString());
        return;
    }
    if (!GuideLightClass || GuideLightClass->HasAnyClassFlags(CLASS_Abstract)) return;
    FActorSpawnParameters Parameters;
    Parameters.Owner = Pawn;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Every pulse owns its route and Niagara simulation. Older pulses continue
    // along their original route until their own distance limit is reached.
    AICQuestGuideLight* Light = World->SpawnActor<AICQuestGuideLight>(
        GuideLightClass, Start.Location, FRotator::ZeroRotator, Parameters);
    if (IsValid(Light))
    {
        Light->TailLength = TailLength;
        Light->FadeOutDuration = FadeOutDuration;
        Light->PlayPath(Path->PathPoints, GuideDistance, GuideSpeed, GuideHeight);
        if (IsValid(Light)) ActiveLights.Add(Light);
    }
}
