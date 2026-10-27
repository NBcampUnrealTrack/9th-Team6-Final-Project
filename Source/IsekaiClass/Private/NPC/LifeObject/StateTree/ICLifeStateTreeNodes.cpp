#include "NPC/LifeObject/StateTree/ICLifeStateTreeNodes.h"

#include "NPC/LifeObject/ICLifeObjectUserComponent.h"
#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Debug/ICLog.h"

namespace ICLifeNodes
{
    static UICLifeObjectUserComponent* GetUser(const AAIController* AI)
    {
        const APawn* Pawn = AI ? AI->GetPawn() : nullptr;
        return Pawn ? Pawn->FindComponentByClass<UICLifeObjectUserComponent>() : nullptr;
    }

    static void SetOrientToMovement(const AAIController* AI, bool bOrient)
    {
        if (const ACharacter* Character = AI ? Cast<ACharacter>(AI->GetPawn()) : nullptr)
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement()) Movement->bOrientRotationToMovement = bOrient;
    }

    static void StopIfMoving(AAIController* AI)
    {
        if (AI && AI->GetMoveStatus() != EPathFollowingStatus::Idle) AI->StopMovement();
    }

    static void FaceRotation(AAIController* AI, const FRotator& Rotation)
    {
        APawn* Pawn = AI ? AI->GetPawn() : nullptr;
        if (!Pawn) return;
        const FRotator Yaw(0.0, Rotation.Yaw, 0.0);
        SetOrientToMovement(AI, false);
        AI->SetControlRotation(Yaw);
        Pawn->SetActorRotation(Yaw);
    }

    static EPathFollowingRequestResult::Type MoveTo(AAIController* AI, const FVector& Goal, float AcceptanceRadius)
    {
        SetOrientToMovement(AI, true);
        FAIMoveRequest Request(Goal);
        Request.SetAcceptanceRadius(AcceptanceRadius);
        Request.SetUsePathfinding(true);
        Request.SetAllowPartialPath(true);
        Request.SetProjectGoalLocation(true);
        Request.SetCanStrafe(false);
        return AI->MoveTo(Request).Code;
    }
}

// ============================================================ 조건

bool FICSTCondition_LifeShouldHold::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    const UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    const bool bResult = !User || User->ShouldHoldLife();
    return bResult != bInvert;
}

bool FICSTCondition_LifeWantsObject::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    const UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    const bool bResult = User && User->GetRoutineRef().WantsObject();
    return bResult != bInvert;
}

bool FICSTCondition_LifeIsAtSlot::TestCondition(FStateTreeExecutionContext& Context) const
{
    const FInstanceDataType& Data = Context.GetInstanceData(*this);
    const UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    const bool bResult = User && User->IsAtClaimedSlot(ExtraToleranceCm);
    return bResult != bInvert;
}

// ============================================================ Hold

EStateTreeRunStatus FICSTTask_LifeHold::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    User->ReleaseAll(TEXT("Hold"));
    ICLifeNodes::StopIfMoving(Data.AIController);
    User->Say(FString::Printf(TEXT("[생활] %s 생활 행동 중단(대화/모드 변경)"), *User->GetUserID().ToString()), FColor::Orange);
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FICSTTask_LifeHold::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    const UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    return User->ShouldHoldLife() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

// ============================================================ Watch Routine

EStateTreeRunStatus FICSTTask_LifeWatchRoutine::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    // 이 상태로 새로 들어왔다는 것 자체가 재계산이므로 이전 요청은 소모한다.
    User->ConsumeRescheduleRequest();
    User->RefreshRoutine(true);
    const FICLifeRoutine& Routine = User->GetRoutineRef();
    Data.SegmentID = Routine.SegmentID;
    Data.ActivityTag = Routine.ActivityTag;
    User->Say(FString::Printf(TEXT("[생활] %s 일과: %s (%s, 앵커 %s)"), *User->GetUserID().ToString(),
        *Routine.SegmentID.ToString(), Routine.ActivityTag.IsValid() ? *Routine.ActivityTag.ToString() : TEXT("오브젝트 없음"),
        *Routine.DestinationAnchorID.ToString()), FColor::White);
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FICSTTask_LifeWatchRoutine::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    if (User->ShouldHoldLife()) return EStateTreeRunStatus::Succeeded;
    if (const FName Reason = User->ConsumeRescheduleRequest(); !Reason.IsNone())
    {
        UE_LOG(LogICLifeObject, Log, TEXT("[Life.Reschedule] NPC=%s Reason=%s"), *User->GetUserID().ToString(), *Reason.ToString());
        return EStateTreeRunStatus::Succeeded;
    }
    if (User->RefreshRoutine()) return EStateTreeRunStatus::Succeeded; // 시간표 구간 종료
    return EStateTreeRunStatus::Running;
}

// ============================================================ Find

FICSTTask_LifeFindObject::FICSTTask_LifeFindObject()
{
    bShouldCallTick = false;
}

EStateTreeRunStatus FICSTTask_LifeFindObject::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    Data.CandidateCount = User->FindCandidates();
    return Data.CandidateCount > 0 ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

// ============================================================ Reserve

FICSTTask_LifeReserveSlot::FICSTTask_LifeReserveSlot()
{
    // 같은 상태로 다시 선택돼도(Sustained) 예약을 풀었다 다시 잡지 않는다.
    bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FICSTTask_LifeReserveSlot::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    if (User->GetCandidates().IsEmpty()) User->FindCandidates();
    // 후보를 점수순으로 시도한다. 다른 NPC가 먼저 잡은 슬롯은 건너뛰고 다음 후보를 잡는다.
    return User->ClaimBestCandidate() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FICSTTask_LifeReserveSlot::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    const UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    return User && User->HasClaim() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

void FICSTTask_LifeReserveSlot::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    if (!bReleaseOnExit) return;
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    if (UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController)) User->ReleaseAll(TEXT("StateExit"));
}

// ============================================================ Move

EStateTreeRunStatus FICSTTask_LifeMoveToSlot::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    FTransform Slot;
    if (!User || !Data.AIController || !User->HasClaim() || !User->GetClaimTransform(Slot)) return EStateTreeRunStatus::Failed;
    if (User->IsAtClaimedSlot()) return EStateTreeRunStatus::Succeeded;

    Data.StartPlaySecond = User->GetPlaySecond();
    const EPathFollowingRequestResult::Type Result = ICLifeNodes::MoveTo(Data.AIController, Slot.GetLocation(), AcceptanceRadiusCm);
    if (Result == EPathFollowingRequestResult::AlreadyAtGoal) return EStateTreeRunStatus::Succeeded;
    if (Result == EPathFollowingRequestResult::Failed)
    {
        const int32 Failures = User->NoteMoveFailure();
        UE_LOG(LogICLifeObject, Warning, TEXT("[Life.MoveFailed] NPC=%s Object=%s Reason=RequestFailed Failures=%d"),
            *User->GetUserID().ToString(), User->GetClaimedObject() ? *User->GetClaimedObject()->GetDebugID().ToString() : TEXT("?"), Failures);
        return EStateTreeRunStatus::Failed;
    }
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FICSTTask_LifeMoveToSlot::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User || !Data.AIController || !User->HasClaim()) return EStateTreeRunStatus::Failed;

    if (User->IsAtClaimedSlot())
    {
        ICLifeNodes::StopIfMoving(Data.AIController);
        User->ResetMoveFailures();
        return EStateTreeRunStatus::Succeeded;
    }

    const TCHAR* FailReason = nullptr;
    if (Data.AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
    {
        // 이동은 끝났는데 도착 거리 밖: 부분 경로로 멈춘 경우. 조금 넉넉하게 한 번 더 본다.
        if (User->IsAtClaimedSlot(User->ArrivalRadiusCm * 0.5f))
        {
            User->ResetMoveFailures();
            return EStateTreeRunStatus::Succeeded;
        }
        FailReason = TEXT("PathEnded");
    }
    else if (User->GetPlaySecond() - Data.StartPlaySecond > TimeoutPlaySeconds)
    {
        FailReason = TEXT("Timeout");
    }

    if (FailReason)
    {
        const int32 Failures = User->NoteMoveFailure();
        UE_LOG(LogICLifeObject, Warning, TEXT("[Life.MoveFailed] NPC=%s Object=%s Reason=%s Failures=%d"),
            *User->GetUserID().ToString(), User->GetClaimedObject() ? *User->GetClaimedObject()->GetDebugID().ToString() : TEXT("?"), FailReason, Failures);
        return EStateTreeRunStatus::Failed;
    }
    return EStateTreeRunStatus::Running;
}

void FICSTTask_LifeMoveToSlot::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    ICLifeNodes::StopIfMoving(Data.AIController);
}

// ============================================================ Occupy

FICSTTask_LifeOccupySlot::FICSTTask_LifeOccupySlot()
{
    bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FICSTTask_LifeOccupySlot::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    ICLifeNodes::StopIfMoving(Data.AIController);
    return User->BeginUse() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FICSTTask_LifeOccupySlot::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    const UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    return User && User->HasClaim() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

void FICSTTask_LifeOccupySlot::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    if (UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController)) User->EndUse();
}

// ============================================================ Animation

FICSTTask_LifePlayAnimation::FICSTTask_LifePlayAnimation()
{
    bShouldCallTick = false;
}

EStateTreeRunStatus FICSTTask_LifePlayAnimation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User) return EStateTreeRunStatus::Failed;
    User->PlayPresentation(); // 애니메이션이 없어도 생활 행동은 계속 유지한다.
    return EStateTreeRunStatus::Running;
}

void FICSTTask_LifePlayAnimation::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    if (UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController)) User->StopPresentation();
}

// ============================================================ Release

FICSTTask_LifeReleaseSlot::FICSTTask_LifeReleaseSlot()
{
    bShouldCallTick = false;
}

EStateTreeRunStatus FICSTTask_LifeReleaseSlot::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    if (UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController)) User->ReleaseAll(Reason);
    return EStateTreeRunStatus::Succeeded;
}

// ============================================================ Recover

EStateTreeRunStatus FICSTTask_LifeRecoverPath::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User || !User->HasClaim()) return EStateTreeRunStatus::Failed;
    if (User->GetMoveFailureCount() > User->MaxMoveFailures)
    {
        UE_LOG(LogICLifeObject, Warning, TEXT("[Life.MoveGiveUp] NPC=%s Object=%s Failures=%d → 대체 행동"),
            *User->GetUserID().ToString(), User->GetClaimedObject() ? *User->GetClaimedObject()->GetDebugID().ToString() : TEXT("?"), User->GetMoveFailureCount());
        User->ReleaseAll(TEXT("MoveFailed"));
        return EStateTreeRunStatus::Failed;
    }
    Data.StartPlaySecond = User->GetPlaySecond();
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FICSTTask_LifeRecoverPath::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    APawn* Pawn = Data.AIController ? Data.AIController->GetPawn() : nullptr;
    if (!User || !Pawn || !User->HasClaim()) return EStateTreeRunStatus::Failed;

    const double Elapsed = User->GetPlaySecond() - Data.StartPlaySecond;
    const bool bVisible = Pawn->WasRecentlyRendered(0.25f);
    const FName ObjectID = User->GetClaimedObject() ? User->GetClaimedObject()->GetDebugID() : NAME_None;

    if (bVisible)
    {
        if (Elapsed < RepathDelayPlaySeconds) return EStateTreeRunStatus::Running;
        UE_LOG(LogICLifeObject, Log, TEXT("[Life.PathRetry] NPC=%s Object=%s Anchor=%s Failures=%d"),
            *User->GetUserID().ToString(), *ObjectID.ToString(), *User->GetRoutineRef().DestinationAnchorID.ToString(), User->GetMoveFailureCount());
        return EStateTreeRunStatus::Succeeded; // → Move To Slot 재시도
    }

    if (Elapsed < OffscreenRecoveryPlaySeconds) return EStateTreeRunStatus::Running;

    // 화면 밖: 안전한 목적지 앵커(없으면 슬롯)로 옮긴 뒤 다시 걸어가게 한다.
    FTransform Target;
    const bool bAnchor = User->ResolveAnchor(User->GetRoutineRef().DestinationAnchorID, Target);
    if (!bAnchor && !User->GetClaimTransform(Target)) return EStateTreeRunStatus::Failed;
    FVector Location = Target.GetLocation();
    if (!bAnchor)
        if (const ACharacter* Character = Cast<ACharacter>(Pawn))
            Location.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const bool bTeleported = Pawn->TeleportTo(Location, FRotator(0.0, Target.Rotator().Yaw, 0.0));
    UE_LOG(LogICLifeObject, Warning, TEXT("[Life.PathRecovered] NPC=%s Object=%s Target=%s Teleported=%d"),
        *User->GetUserID().ToString(), *ObjectID.ToString(), bAnchor ? *User->GetRoutineRef().DestinationAnchorID.ToString() : TEXT("Slot"), bTeleported ? 1 : 0);
    return bTeleported ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

// ============================================================ Wait At Anchor

EStateTreeRunStatus FICSTTask_LifeWaitAtAnchor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User || !Data.AIController) return EStateTreeRunStatus::Failed;

    Data.bArrived = false;
    Data.NextRetryPlaySecond = User->GetPlaySecond() + User->RetryIntervalPlaySeconds;

    const FICLifeRoutine& Routine = User->GetRoutineRef();
    FTransform Anchor;
    if (User->ResolveAnchor(Routine.DestinationAnchorID, Anchor))
    {
        const EPathFollowingRequestResult::Type Result = ICLifeNodes::MoveTo(Data.AIController, Anchor.GetLocation(), 50.f);
        if (Result == EPathFollowingRequestResult::Failed)
            UE_LOG(LogICLifeObject, Warning, TEXT("[Life.AnchorMoveFailed] NPC=%s Anchor=%s"), *User->GetUserID().ToString(), *Routine.DestinationAnchorID.ToString());
    }
    else
    {
        Data.bArrived = true; // 앵커가 로드되지 않았으면 제자리에서 기다린다.
    }

    if (Routine.WantsObject())
    {
        User->Say(FString::Printf(TEXT("[생활] %s 빈 오브젝트 없음 → %s (%s)"), *User->GetUserID().ToString(),
            User->GetFallback() == EICLifeFallback::SleepOnFloor ? TEXT("바닥에서 수면") : TEXT("앵커에서 대기"),
            *Routine.ActivityTag.ToString()), FColor::Orange);
    }
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FICSTTask_LifeWaitAtAnchor::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController);
    if (!User || !Data.AIController) return EStateTreeRunStatus::Failed;
    const FICLifeRoutine& Routine = User->GetRoutineRef();

    if (!Data.bArrived)
    {
        FTransform Anchor;
        const bool bHasAnchor = User->ResolveAnchor(Routine.DestinationAnchorID, Anchor);
        const bool bClose = bHasAnchor && FVector::DistSquared2D(User->GetOwner()->GetActorLocation(), Anchor.GetLocation()) <= FMath::Square(User->ArrivalRadiusCm);
        if (!bHasAnchor || bClose || Data.AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
        {
            Data.bArrived = true;
            ICLifeNodes::StopIfMoving(Data.AIController);
            if (bHasAnchor) ICLifeNodes::FaceRotation(Data.AIController, Anchor.Rotator());
            if (Routine.WantsObject() && User->GetFallback() == EICLifeFallback::SleepOnFloor)
            {
                User->SetFloorSleeping(true);
                User->Say(FString::Printf(TEXT("[생활] %s 바닥에서 잠듦"), *User->GetUserID().ToString()), FColor::Orange);
            }
        }
    }

    if (bRetryObjects && Routine.WantsObject() && User->GetPlaySecond() >= Data.NextRetryPlaySecond)
    {
        Data.NextRetryPlaySecond = User->GetPlaySecond() + User->RetryIntervalPlaySeconds;
        if (User->FindCandidates() > 0) return EStateTreeRunStatus::Succeeded; // → Find Object부터 다시
    }
    return EStateTreeRunStatus::Running;
}

void FICSTTask_LifeWaitAtAnchor::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
    FInstanceDataType& Data = Context.GetInstanceData(*this);
    ICLifeNodes::StopIfMoving(Data.AIController);
    ICLifeNodes::SetOrientToMovement(Data.AIController, true);
    if (UICLifeObjectUserComponent* User = ICLifeNodes::GetUser(Data.AIController)) User->SetFloorSleeping(false);
}
