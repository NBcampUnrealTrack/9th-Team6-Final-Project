#include "NPC/Behavior/ICNPCHomeBehaviorComponent.h"

#include "AI/ICAIController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "NPC/Character/ICNPCBase.h"
#include "NPC/LifeObject/ICLifeObjectUserComponent.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "NPC/LifeObject/ICLifeObjectTypes.h"
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
    // 집 주변을 생활감 있게 이동하는 것이 기본. 정지 행동은 짧고 드물게 둔다.
    Wander.Weight = 5.f;
    Wander.MinDurationSeconds = 10.f;
    Wander.MaxDurationSeconds = 20.f;
    FICNPCHomeActionConfig& Idle = ActionConfigs.AddDefaulted_GetRef();
    Idle.Action = EICNPCHomeAction::Idle;
    Idle.Weight = 1.f;
    Idle.MinDurationSeconds = 4.f;
    Idle.MaxDurationSeconds = 8.f;
    FICNPCHomeActionConfig& LookAround = ActionConfigs.AddDefaulted_GetRef();
    LookAround.Action = EICNPCHomeAction::LookAround;
    LookAround.Weight = 1.f;
    LookAround.MinDurationSeconds = 3.f;
    LookAround.MaxDurationSeconds = 6.f;

    auto AddObjectActivity = [this](FGameplayTag Tag, float Weight, float MinUse, float MaxUse)
    {
        FICNPCHomeObjectActivityConfig& Entry = ObjectActivityPool.AddDefaulted_GetRef();
        Entry.ActivityTag = Tag;
        Entry.Weight = Weight;
        Entry.MinUseSeconds = MinUse;
        Entry.MaxUseSeconds = MaxUse;
    };
    // 게임 내 1분은 PlayClock 2초다. 60~120초는 게임 내 30~60분의 한 활동을 의미한다.
    AddObjectActivity(ICLifeTags::Sit, 3.f, 60.f, 120.f);         // 방 의자·벤치
    AddObjectActivity(ICLifeTags::WorkFarm, 1.f, 60.f, 120.f);    // 화단 물주기 등
    AddObjectActivity(ICLifeTags::WorkCook, 1.f, 60.f, 120.f);    // 집 주방
    AddObjectActivity(ICLifeTags::LeisureView, 2.f, 60.f, 120.f); // 창가·풍경 벤치
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
    UpdateStationaryWatch(Now);
    if (bHomeObjectRetryRequested) return true;
    if (!bActive || Now >= NextDecisionAt) StartNextAction(Controller, HomeTransform, Now);
    return true;
}

void UICNPCHomeBehaviorComponent::StopBehavior(AICAIController* Controller)
{
    if (Controller) Controller->StopHomeBehaviorMove();
    CurrentAction = EICNPCHomeAction::None;
    NextDecisionAt = 0.0;
    LastMovementAt = 0.0;
    bHomeObjectRetryRequested = false;
    bActive = false;
}

double UICNPCHomeBehaviorComponent::GetPlaySecond() const
{
    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (const UICTimeSubsystem* Time = GameInstance ? GameInstance->GetSubsystem<UICTimeSubsystem>() : nullptr)
        return Time->GetClock().PlaySecond;
    return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

bool UICNPCHomeBehaviorComponent::ShouldUseHomeObjectPool()
{
    FTransform IgnoredHomeTransform;
    if (!CanRun(IgnoredHomeTransform)) return false;
    const AICNPCBase* NPC = GetNPC();
    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UICDefinitionSubsystem* Definitions = GameInstance ? GameInstance->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    const UICTimeSubsystem* Time = GameInstance ? GameInstance->GetSubsystem<UICTimeSubsystem>() : nullptr;
    if (!NPC || !Definitions || !Time) return false;
    FICDefinitionResult Result;
    const FICScheduleRow* Schedule = Definitions->Find<FICScheduleRow>(NPC->GetNPCDefinition().ScheduleID, Result);
    if (!Schedule) return false;
    const FICNPCScheduleResolution Segment = UICNPCScheduleLibrary::ResolveSchedule(*Schedule, Time->GetClock().GameMinute);
    // 집에서의 Idle 구간은 Leisure/Rest/Sit/Cook 등 어떤 생활 태그든 활동 풀을 순환한다.
    // Move는 집으로 이동하는 중이므로 제외하고, Work/Sleep은 CanRun 단계에서 이미 제외된다.
    return Segment.bFound && Segment.Activity == EICScheduleActivity::Idle;
}

const FICNPCHomeObjectActivityConfig* UICNPCHomeBehaviorComponent::FindObjectActivity(FGameplayTag ActivityTag) const
{
    return ObjectActivityPool.FindByPredicate([ActivityTag](const FICNPCHomeObjectActivityConfig& Entry)
    {
        return Entry.ActivityTag == ActivityTag;
    });
}

int32 UICNPCHomeBehaviorComponent::FindNextHomeObjectCandidates(UICLifeObjectUserComponent& User)
{
    if (!ShouldUseHomeObjectPool() || ObjectActivityPool.IsEmpty()) return 0;

    // 방금 쓴 오브젝트는 쿨다운 동안 활동 종류와 무관하게 후보에서 뺀다.
    // 다른 오브젝트가 없으면 0을 돌려 StateTree가 대기/자율 행동으로 가게 하고, 쿨다운 뒤에 다시 고른다.
    const AICLifeObjectActor* CooldownObject = nullptr;
    if (ObjectReuseCooldownSeconds > 0.f && LastHomeObjectEndedAt >= 0.0
        && GetPlaySecond() - LastHomeObjectEndedAt < ObjectReuseCooldownSeconds)
    {
        CooldownObject = User.GetLastUsedObject();
    }

    TArray<int32> Remaining;
    for (int32 Index = 0; Index < ObjectActivityPool.Num(); ++Index)
        if (ObjectActivityPool[Index].ActivityTag.IsValid() && ObjectActivityPool[Index].Weight > 0.f)
            Remaining.Add(Index);

    while (!Remaining.IsEmpty())
    {
        bool bCanAvoidPrevious = Remaining.ContainsByPredicate([this](int32 Index)
        {
            return ObjectActivityPool[Index].ActivityTag != PreviousHomeObjectActivity;
        });
        float TotalWeight = 0.f;
        for (int32 Index : Remaining)
            if (!bCanAvoidPrevious || ObjectActivityPool[Index].ActivityTag != PreviousHomeObjectActivity)
                TotalWeight += ObjectActivityPool[Index].Weight;
        float Roll = FMath::FRandRange(0.f, TotalWeight);
        int32 SelectedIndex = Remaining.Last();
        for (int32 Index : Remaining)
        {
            if (bCanAvoidPrevious && ObjectActivityPool[Index].ActivityTag == PreviousHomeObjectActivity) continue;
            Roll -= ObjectActivityPool[Index].Weight;
            if (Roll <= 0.f) { SelectedIndex = Index; break; }
        }

        const FICNPCHomeObjectActivityConfig& Selected = ObjectActivityPool[SelectedIndex];
        if (const int32 Count = User.FindCandidatesForActivity(Selected.ActivityTag, CooldownObject); Count > 0)
        {
            CurrentHomeObjectActivity = Selected.ActivityTag;
            bHomeObjectUseActive = false;
            return Count;
        }
        Remaining.RemoveSingleSwap(SelectedIndex);
    }

    CurrentHomeObjectActivity = FGameplayTag();
    bHomeObjectUseActive = false;
    return 0;
}

void UICNPCHomeBehaviorComponent::BeginHomeObjectUse()
{
    const FICNPCHomeObjectActivityConfig* Activity = FindObjectActivity(CurrentHomeObjectActivity);
    // 활동 풀이 후보를 골랐으면 그 활동의 시간, 그렇지 않아도 집 Idle 오브젝트라면
    // 안전한 기본 시간으로 반드시 해제 타이머를 건다. 기존 Rest/Sit 일정도 고착되지 않는다.
    if (!ShouldUseHomeObjectPool()) return;
    const float MinUse = Activity ? Activity->MinUseSeconds : 10.f;
    const float MaxUse = Activity ? Activity->MaxUseSeconds : 20.f;
    HomeObjectUseEndsAt = GetPlaySecond() + FMath::FRandRange(MinUse, FMath::Max(MinUse, MaxUse));
    bHomeObjectUseActive = true;
}

bool UICNPCHomeBehaviorComponent::IsHomeObjectUseFinished() const
{
    return bHomeObjectUseActive && GetPlaySecond() >= HomeObjectUseEndsAt;
}

void UICNPCHomeBehaviorComponent::EndHomeObjectUse()
{
    // 실제로 사용을 시작했던 경우에만 쿨다운을 건다(이동 중 중단 등은 제외).
    if (bHomeObjectUseActive) LastHomeObjectEndedAt = GetPlaySecond();
    if (CurrentHomeObjectActivity.IsValid()) PreviousHomeObjectActivity = CurrentHomeObjectActivity;
    CurrentHomeObjectActivity = FGameplayTag();
    HomeObjectUseEndsAt = 0.0;
    bHomeObjectUseActive = false;
}

double UICNPCHomeBehaviorComponent::GetMaxHomeObjectUseSeconds() const
{
    double MaxUse = 0.0;
    for (const FICNPCHomeObjectActivityConfig& Entry : ObjectActivityPool)
        MaxUse = FMath::Max(MaxUse, static_cast<double>(FMath::Max(Entry.MinUseSeconds, Entry.MaxUseSeconds)));
    return FMath::Max(1.0, MaxUse);
}

void UICNPCHomeBehaviorComponent::UpdateStationaryWatch(double Now)
{
    const AActor* Owner = GetOwner();
    if (!Owner) return;
    const FVector CurrentLocation = Owner->GetActorLocation();
    if (LastMovementAt <= 0.0)
    {
        LastObservedLocation = CurrentLocation;
        LastMovementAt = Now;
        return;
    }
    // Capsule의 미세한 회피 흔들림은 무시하고, 실제로 자리를 옮겼을 때만 시간을 갱신한다.
    if (FVector::DistSquared2D(CurrentLocation, LastObservedLocation) >= FMath::Square(35.f))
    {
        LastObservedLocation = CurrentLocation;
        LastMovementAt = Now;
        return;
    }
    // Idle/LookAround는 짧게 끝나지만, 그보다 긴 정체는 "쓸 오브젝트 재탐색"으로 복구한다.
    if (Now - LastMovementAt >= GetMaxHomeObjectUseSeconds())
        bHomeObjectRetryRequested = true;
}

bool UICNPCHomeBehaviorComponent::ConsumeHomeObjectRetryRequest()
{
    const bool bRequested = bHomeObjectRetryRequested;
    bHomeObjectRetryRequested = false;
    return bRequested;
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
