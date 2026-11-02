#include "NPC/Behavior/ICNPCHomeBehaviorComponent.h"

#include "AI/ICAIController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "NPC/Character/ICNPCBase.h"
#include "NPC/Schedule/ICNPCScheduleLibrary.h"
#include "Time/ICTimeSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "NavigationSystem.h"
#include "Engine/GameInstance.h"

UICNPCHomeBehaviorComponent::UICNPCHomeBehaviorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    FICNPCHomeActionConfig& Wander = ActionConfigs.AddDefaulted_GetRef();
    Wander.Action = EICNPCHomeAction::Wander;
    Wander.Weight = 2.f;
    Wander.MinDurationSeconds = 5.f;
    Wander.MaxDurationSeconds = 12.f;
    FICNPCHomeActionConfig& Idle = ActionConfigs.AddDefaulted_GetRef();
    Idle.Action = EICNPCHomeAction::Idle;
    Idle.Weight = 1.f;
    Idle.MinDurationSeconds = 4.f;
    Idle.MaxDurationSeconds = 10.f;
    FICNPCHomeActionConfig& LookAround = ActionConfigs.AddDefaulted_GetRef();
    LookAround.Action = EICNPCHomeAction::LookAround;
    LookAround.Weight = 1.f;
    LookAround.MinDurationSeconds = 3.f;
    LookAround.MaxDurationSeconds = 8.f;
}

void UICNPCHomeBehaviorComponent::BeginPlay()
{
    Super::BeginPlay();
    // NPC Definition은 AICNPCBase::InitializeNPC에서 채워진다. CanRun에서 지연 해석한다.
}

void UICNPCHomeBehaviorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopBehavior();
    Super::EndPlay(EndPlayReason);
}

AICNPCBase* UICNPCHomeBehaviorComponent::GetNPC() const
{
    return Cast<AICNPCBase>(GetOwner());
}

bool UICNPCHomeBehaviorComponent::CanRun(FTransform& OutHomeTransform)
{
    AICNPCBase* NPC = GetNPC();
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UICWorldStateSubsystem* WorldState = GameInstance ? GameInstance->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    UICDefinitionSubsystem* Definitions = GameInstance ? GameInstance->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    UICTimeSubsystem* Time = GameInstance ? GameInstance->GetSubsystem<UICTimeSubsystem>() : nullptr;
    UICSpawnSubsystem* Spawns = GetWorld() ? GetWorld()->GetSubsystem<UICSpawnSubsystem>() : nullptr;
    if (!NPC || !WorldState || !Definitions || !Time || !Spawns) return false;
    if (ResolvedHomeAnchorID.IsNone())
        ResolvedHomeAnchorID = HomeAnchorID.IsNone() ? NPC->GetNPCDefinition().InitialAnchorID : HomeAnchorID;
    if (ResolvedHomeAnchorID.IsNone()) return false;

    const FICCharacterState State = WorldState->GetSnapshot(NPC->GetCharacterID());
    if (State.Mode != EICCharacterMode::Scheduled || State.IsDowned() || NPC->IsInteracting()) return false;

    FICDefinitionResult Result;
    const FICScheduleRow* Schedule = Definitions->Find<FICScheduleRow>(NPC->GetNPCDefinition().ScheduleID, Result);
    if (!Schedule) return false;
    const FICNPCScheduleResolution Segment = UICNPCScheduleLibrary::ResolveSchedule(*Schedule, Time->GetClock().GameMinute);
    // 집으로 귀가한 비근무 시간에만 개입한다. 다른 Idle 목적지를 집 행동으로 바꾸지 않는다.
    if (!Segment.bFound || Segment.Activity == EICScheduleActivity::Work || Segment.DestinationAnchorID != ResolvedHomeAnchorID) return false;
    return Spawns->ResolveLoadedAnchor(ResolvedHomeAnchorID, OutHomeTransform);
}

bool UICNPCHomeBehaviorComponent::CanRunHomeBehavior()
{
    FTransform IgnoredHomeTransform;
    return CanRun(IgnoredHomeTransform);
}

bool UICNPCHomeBehaviorComponent::TickBehavior(AICAIController& Controller)
{
    FTransform HomeTransform;
    if (!CanRun(HomeTransform))
    {
        if (bActive) StopBehavior(&Controller);
        return false;
    }

    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UICTimeSubsystem* Time = GameInstance ? GameInstance->GetSubsystem<UICTimeSubsystem>() : nullptr;
    const double Now = Time ? Time->GetClock().PlaySecond : GetWorld()->GetTimeSeconds();
    if (!bActive || Now >= NextDecisionAt) StartNextAction(Controller, HomeTransform, Now);
    return true;
}

void UICNPCHomeBehaviorComponent::StopBehavior(AICAIController* Controller)
{
    if (Controller) Controller->StopHomeBehaviorMove();
    CurrentAction = EICNPCHomeAction::None;
    NextDecisionAt = 0.0;
    bActive = false;
}

void UICNPCHomeBehaviorComponent::StartNextAction(AICAIController& Controller, const FTransform& HomeTransform, double Now)
{
    CurrentAction = ChooseAction();
    PreviousAction = CurrentAction;
    const FICNPCHomeActionConfig* Config = ActionConfigs.FindByPredicate([this](const FICNPCHomeActionConfig& Entry) { return Entry.Action == CurrentAction; });
    const float MinTime = Config ? Config->MinDurationSeconds : 4.f;
    const float MaxTime = Config ? FMath::Max(MinTime, Config->MaxDurationSeconds) : 8.f;
    NextDecisionAt = Now + FMath::FRandRange(MinTime, MaxTime);
    bActive = true;

    if (CurrentAction == EICNPCHomeAction::Wander)
    {
        FVector Goal;
        if (FindWanderGoal(HomeTransform.GetLocation(), Goal))
        {
            Controller.RequestHomeBehaviorMove(Goal, MoveAcceptanceRadiusCm);
            return;
        }
    }
    StartIdleAction(Controller, HomeTransform);
}

EICNPCHomeAction UICNPCHomeBehaviorComponent::ChooseAction() const
{
    const bool bCanAvoidPrevious = ActionConfigs.ContainsByPredicate([this](const FICNPCHomeActionConfig& Entry)
    {
        return Entry.Action != EICNPCHomeAction::None && Entry.Action != PreviousAction && Entry.Weight > 0.f;
    });
    float TotalWeight = 0.f;
    for (const FICNPCHomeActionConfig& Entry : ActionConfigs)
        if (Entry.Action != EICNPCHomeAction::None && (!bCanAvoidPrevious || Entry.Action != PreviousAction))
            TotalWeight += FMath::Max(0.f, Entry.Weight);
    if (TotalWeight <= 0.f) return EICNPCHomeAction::Idle;

    float Roll = FMath::FRandRange(0.f, TotalWeight);
    for (const FICNPCHomeActionConfig& Entry : ActionConfigs)
    {
        if (Entry.Action == EICNPCHomeAction::None || (bCanAvoidPrevious && Entry.Action == PreviousAction)) continue;
        Roll -= FMath::Max(0.f, Entry.Weight);
        if (Roll <= 0.f) return Entry.Action;
    }
    return EICNPCHomeAction::Idle;
}

void UICNPCHomeBehaviorComponent::StartIdleAction(AICAIController& Controller, const FTransform& HomeTransform)
{
    Controller.StopHomeBehaviorMove();
    // 앵커의 정면을 기준으로 약간씩 바꿔 보게 한다. 애니메이션은 AnimBP의 Idle 상태를 그대로 사용한다.
    const FRotator Facing = HomeTransform.Rotator() + FRotator(0.f, FMath::FRandRange(-85.f, 85.f), 0.f);
    Controller.FaceHomeBehavior(Facing);
}

bool UICNPCHomeBehaviorComponent::FindWanderGoal(const FVector& Center, FVector& OutGoal) const
{
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation Candidate;
    return Navigation && Navigation->GetRandomReachablePointInRadius(Center, WanderRadiusCm, Candidate)
        ? (OutGoal = Candidate.Location, true) : false;
}
