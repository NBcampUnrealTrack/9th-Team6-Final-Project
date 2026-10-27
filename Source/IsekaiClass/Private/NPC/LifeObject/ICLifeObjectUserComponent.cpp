#include "NPC/LifeObject/ICLifeObjectUserComponent.h"

#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "NPC/LifeObject/ICLifeObjectSubsystem.h"
#include "NPC/LifeObject/ICLifeObjectBehaviorDefinition.h"
#include "NPC/Character/ICNPCBase.h"
#include "NPC/Schedule/ICNPCScheduleLibrary.h"
#include "AI/ICAIController.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/NPC/ICScheduleRow.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "SmartObjectSubsystem.h"
#include "Animation/AnimMontage.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Debug/ICLog.h"

UICLifeObjectUserComponent::UICLifeObjectUserComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UICLifeObjectUserComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // NPC 파괴·언로드·레벨 이동 등 어떤 이유로 사라져도 예약을 남기지 않는다.
    ReleaseAll(TEXT("EndPlay"));
    Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------- 정보

AICNPCBase* UICLifeObjectUserComponent::GetNPC() const
{
    return Cast<AICNPCBase>(GetOwner());
}

AAIController* UICLifeObjectUserComponent::GetAIController() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    return Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
}

FName UICLifeObjectUserComponent::GetUserID() const
{
    if (const AICNPCBase* NPC = GetNPC())
    {
        const FName ID = NPC->GetCharacterID();
        return ID.IsNone() ? NPC->NPCDefinitionID : ID;
    }
    return GetOwner() ? FName(*GetOwner()->GetName()) : NAME_None;
}

FGameplayTag UICLifeObjectUserComponent::GetOccupationTag() const
{
    const AICNPCBase* NPC = GetNPC();
    return NPC ? NPC->GetNPCDefinition().OccupationTag : FGameplayTag();
}

FName UICLifeObjectUserComponent::GetRegionID() const
{
    const AICNPCBase* NPC = GetNPC();
    return NPC ? NPC->GetNPCDefinition().RegionID : NAME_None;
}

EICLifeFallback UICLifeObjectUserComponent::GetFallback() const
{
    const bool bSleep = Routine.Activity == EICScheduleActivity::Sleep
        || (Routine.ActivityTag.IsValid() && Routine.ActivityTag.MatchesTag(ICLifeTags::Sleep));
    return bSleep ? EICLifeFallback::SleepOnFloor : EICLifeFallback::WaitAtAnchor;
}

bool UICLifeObjectUserComponent::ResolveAnchor(FName AnchorID, FTransform& OutTransform) const
{
    const UICSpawnSubsystem* Spawns = GetWorld() ? GetWorld()->GetSubsystem<UICSpawnSubsystem>() : nullptr;
    return Spawns && !AnchorID.IsNone() && Spawns->ResolveLoadedAnchor(AnchorID, OutTransform);
}

double UICLifeObjectUserComponent::GetPlaySecond() const
{
    const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (const UICTimeSubsystem* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr) return Time->GetClock().PlaySecond;
    return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void UICLifeObjectUserComponent::Say(const FString& Message, const FColor& Color) const
{
    UE_LOG(LogICLifeObject, Log, TEXT("%s"), *Message);
#if !UE_BUILD_SHIPPING
    if (bPrintToScreen && GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, Message);
#endif
}

// ---------------------------------------------------------------- 일과

bool UICLifeObjectUserComponent::RefreshRoutine(bool bForce)
{
    // 시간표는 게임 분 단위라 같은 분 안에서는 다시 계산할 필요가 없다(매 틱 DT 조회 방지).
    {
        const UGameInstance* ClockGI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
        const UICTimeSubsystem* Clock = ClockGI ? ClockGI->GetSubsystem<UICTimeSubsystem>() : nullptr;
        const int32 Minute = Clock ? FMath::FloorToInt(Clock->GetClock().GameMinute) : INDEX_NONE;
        if (!bForce && Minute != INDEX_NONE && Minute == LastRoutineGameMinute) return false;
        LastRoutineGameMinute = Minute;
    }
    FICLifeRoutine Next;
    const AICNPCBase* NPC = GetNPC();
    const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UICDefinitionSubsystem* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    const UICTimeSubsystem* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr;
    if (NPC && Definitions && Time)
    {
        FICDefinitionResult Result;
        if (const FICScheduleRow* Schedule = Definitions->Find<FICScheduleRow>(NPC->GetNPCDefinition().ScheduleID, Result))
        {
            const FICNPCScheduleResolution Resolved = UICNPCScheduleLibrary::ResolveSchedule(*Schedule, Time->GetClock().GameMinute);
            if (Resolved.bFound)
            {
                Next.bValid = true;
                Next.SegmentID = Resolved.SegmentID;
                Next.Activity = Resolved.Activity;
                Next.ActivityTag = ICLife::ResolveActivityTag(Resolved.Activity, Resolved.ActivityTag);
                Next.DestinationAnchorID = Resolved.DestinationAnchorID;
                Next.StartGameMinute = Resolved.StartGameMinute;
                Next.EndGameMinute = Resolved.EndGameMinute;
            }
        }
    }
    const bool bChanged = !Next.IsSameSegment(Routine);
    Routine = Next;
    if (bChanged)
    {
        UE_LOG(LogICLifeObject, Log, TEXT("[Life.Routine] NPC=%s Segment=%s Activity=%s Anchor=%s"),
            *GetUserID().ToString(), *Routine.SegmentID.ToString(), *Routine.ActivityTag.ToString(), *Routine.DestinationAnchorID.ToString());
    }
    return bChanged;
}

bool UICLifeObjectUserComponent::ShouldHoldLife() const
{
    const AICNPCBase* NPC = GetNPC();
    if (!NPC) return true;
    if (NPC->IsInteracting()) return true;
    if (const AICAIController* AI = Cast<AICAIController>(NPC->GetController()); AI && AI->IsInteractionHeld()) return true;
    const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UICWorldStateSubsystem* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
    if (!State) return true;
    const FICCharacterState Snapshot = State->GetSnapshot(NPC->GetCharacterID());
    return Snapshot.Mode != EICCharacterMode::Scheduled || Snapshot.IsDowned();
}

void UICLifeObjectUserComponent::RequestReschedule(FName Reason)
{
    bRescheduleRequested = true;
    PendingRescheduleReason = Reason;
    UE_LOG(LogICLifeObject, Verbose, TEXT("[Life.RescheduleRequested] NPC=%s Reason=%s"), *GetUserID().ToString(), *Reason.ToString());
}

FName UICLifeObjectUserComponent::ConsumeRescheduleRequest()
{
    if (!bRescheduleRequested) return NAME_None;
    bRescheduleRequested = false;
    const FName Reason = PendingRescheduleReason;
    PendingRescheduleReason = NAME_None;
    return Reason.IsNone() ? FName(TEXT("Requested")) : Reason;
}

// ---------------------------------------------------------------- 탐색·예약

int32 UICLifeObjectUserComponent::FindCandidates()
{
    Candidates.Reset();
    const UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this);
    if (!Life || !Routine.WantsObject()) return 0;
    const int32 Count = Life->GatherCandidates(*this, Routine, SearchRadiusCm, Candidates);
    UE_LOG(LogICLifeObject, Verbose, TEXT("[Life.Find] NPC=%s Activity=%s Candidates=%d"), *GetUserID().ToString(), *Routine.ActivityTag.ToString(), Count);
    return Count;
}

bool UICLifeObjectUserComponent::ClaimBestCandidate()
{
    if (HasClaim()) return true;
    UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this);
    if (!Life) return false;
    for (const FICLifeObjectCandidate& Candidate : Candidates)
    {
        AICLifeObjectActor* Object = Candidate.Object.Get();
        if (!Object) continue;
        FSmartObjectClaimHandle NewClaim;
        if (!Life->TryClaim(*this, *Object, Candidate.SlotIndex, NewClaim)) continue;
        Claim = NewClaim;
        ClaimedObject = Object;
        ClaimedSlotIndex = Candidate.SlotIndex;
        MoveFailureCount = 0;
        Say(FString::Printf(TEXT("[생활] %s → %s 슬롯 %d 예약"), *GetUserID().ToString(), *Object->GetDebugID().ToString(), ClaimedSlotIndex));
        return true;
    }
    UE_LOG(LogICLifeObject, Log, TEXT("[Life.NoFreeSlot] NPC=%s Activity=%s Candidates=%d"), *GetUserID().ToString(), *Routine.ActivityTag.ToString(), Candidates.Num());
    return false;
}

bool UICLifeObjectUserComponent::HasClaim() const
{
    const UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this);
    return Claim.IsValid() && ClaimedObject.IsValid() && Life && Life->IsClaimAlive(Claim);
}

AICLifeObjectActor* UICLifeObjectUserComponent::GetClaimedObject() const
{
    return ClaimedObject.Get();
}

bool UICLifeObjectUserComponent::GetClaimTransform(FTransform& OutTransform) const
{
    const UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this);
    return Life && Claim.IsValid() && Life->GetSlotTransform(Claim, OutTransform);
}

bool UICLifeObjectUserComponent::IsAtClaimedSlot(float ExtraToleranceCm) const
{
    FTransform Slot;
    if (!GetOwner() || !GetClaimTransform(Slot)) return false;
    return FVector::DistSquared2D(GetOwner()->GetActorLocation(), Slot.GetLocation()) <= FMath::Square(ArrivalRadiusCm + ExtraToleranceCm);
}

// ---------------------------------------------------------------- 점유·연출

bool UICLifeObjectUserComponent::BeginUse()
{
    if (!HasClaim()) return false;
    if (bUsing) return true;
    UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this);
    AICLifeObjectActor* Object = ClaimedObject.Get();
    if (!Life || !Object) return false;

    Life->MarkOccupied(Claim, bOccupiedMarked);
    bUsing = true;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    FTransform Slot;
    if (Character && Object->bSnapUserToSlot && GetClaimTransform(Slot))
    {
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
            bRestoreOrientToMovement = Movement->bOrientRotationToMovement;
            Movement->bOrientRotationToMovement = false;
        }
        if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
        {
            // 의자·침대 메시와 캡슐이 겹쳐도 밀려나지 않게 사용 중에만 서로 무시한다.
            Capsule->IgnoreActorWhenMoving(Object, true);
            bCollisionIgnored = true;
        }
        const FRotator Facing(0.0, Slot.Rotator().Yaw, 0.0);
        const FVector Location(Slot.GetLocation().X, Slot.GetLocation().Y, Character->GetActorLocation().Z);
        Character->SetActorLocationAndRotation(Location, Facing, false, nullptr, ETeleportType::TeleportPhysics);
        if (AAIController* AI = GetAIController()) AI->SetControlRotation(Facing);
    }

    Say(FString::Printf(TEXT("[생활] %s → %s 사용 시작 (%s, 슬롯 %d, 활동 %s)"),
        *GetUserID().ToString(), *Object->GetDebugID().ToString(), ICLife::LexType(Object->ObjectType), ClaimedSlotIndex,
        *Routine.ActivityTag.ToString()), FColor::Green);
    return true;
}

void UICLifeObjectUserComponent::EndUse()
{
    StopPresentation();
    if (bCollisionIgnored)
    {
        if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
            if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
                if (AICLifeObjectActor* Object = ClaimedObject.Get()) Capsule->IgnoreActorWhenMoving(Object, false);
        bCollisionIgnored = false;
    }
    if (bRestoreOrientToMovement)
    {
        if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement()) Movement->bOrientRotationToMovement = true;
        bRestoreOrientToMovement = false;
    }
    bUsing = false;
    bOccupiedMarked = false;
}

bool UICLifeObjectUserComponent::PlayPresentation()
{
    AICLifeObjectActor* Object = ClaimedObject.Get();
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!bUsing || !Object || !Character) return false;

    const FICLifeSlotPresentation* Presentation = Object->FindActorPresentation(ClaimedSlotIndex);
    if (!Presentation)
    {
        if (USmartObjectSubsystem* SO = USmartObjectSubsystem::GetCurrent(GetWorld()))
        {
            const UICLifeObjectBehaviorDefinition* Behavior = Cast<UICLifeObjectBehaviorDefinition>(
                SO->GetBehaviorDefinition(Claim, UICLifeObjectBehaviorDefinition::StaticClass()));
            if (Behavior && Behavior->Presentation.IsSet()) Presentation = &Behavior->Presentation;
        }
    }
    if (!Presentation)
    {
        Say(FString::Printf(TEXT("[생활] %s : %s 애니메이션 미지정 (연출 생략)"), *GetUserID().ToString(), *Object->GetDebugID().ToString()), FColor::Silver);
        return true;
    }
    UAnimMontage* Montage = Presentation->Montage.LoadSynchronous();
    if (!Montage) return false;
    Character->PlayAnimMontage(Montage, Presentation->PlayRate, Presentation->StartSection);
    PlayingMontage = Montage;
    Say(FString::Printf(TEXT("[생활] %s : %s 재생"), *GetUserID().ToString(), *Montage->GetName()), FColor::Green);
    return true;
}

void UICLifeObjectUserComponent::StopPresentation()
{
    if (UAnimMontage* Montage = PlayingMontage.Get())
        if (ACharacter* Character = Cast<ACharacter>(GetOwner())) Character->StopAnimMontage(Montage);
    PlayingMontage.Reset();
}

bool UICLifeObjectUserComponent::IsSleeping() const
{
    return GetFallback() == EICLifeFallback::SleepOnFloor && (bUsing || bFloorSleeping);
}

// ---------------------------------------------------------------- 해제

void UICLifeObjectUserComponent::ReleaseAll(FName Reason)
{
    if (bReleasing) return;
    TGuardValue<bool> Guard(bReleasing, true);

    const bool bHadSomething = Claim.IsValid() || bUsing || bFloorSleeping;
    const FName ObjectName = ClaimedObject.IsValid() ? ClaimedObject->GetDebugID() : NAME_None;

    EndUse();
    bFloorSleeping = false;
    if (Claim.IsValid())
        if (UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this)) Life->Release(Claim, Reason);

    Claim.Invalidate();
    ClaimedObject.Reset();
    ClaimedSlotIndex = INDEX_NONE;
    Candidates.Reset();
    MoveFailureCount = 0;

    if (bHadSomething)
    {
        Say(FString::Printf(TEXT("[생활] %s 점유 해제 (%s, 이유: %s)"), *GetUserID().ToString(),
            ObjectName.IsNone() ? TEXT("오브젝트 없음") : *ObjectName.ToString(), *Reason.ToString()), FColor::Yellow);
    }
}

void UICLifeObjectUserComponent::HandleSlotInvalidated(const FSmartObjectClaimHandle& InvalidatedClaim, ESmartObjectSlotState State)
{
    if (!OwnsClaim(InvalidatedClaim)) return;
    UE_LOG(LogICLifeObject, Warning, TEXT("[Life.SlotInvalidated] NPC=%s Object=%s Slot=%d"),
        *GetUserID().ToString(), ClaimedObject.IsValid() ? *ClaimedObject->GetDebugID().ToString() : TEXT("?"), ClaimedSlotIndex);
    EndUse();
    if (UICLifeObjectSubsystem* Life = UICLifeObjectSubsystem::Get(this)) Life->Release(Claim, TEXT("SlotInvalidated"), true);
    Claim.Invalidate();
    ClaimedObject.Reset();
    ClaimedSlotIndex = INDEX_NONE;
    RequestReschedule(TEXT("SlotInvalidated"));
}

void UICLifeObjectUserComponent::NotifyClaimRevoked(FName Reason)
{
    ReleaseAll(Reason);
    RequestReschedule(Reason);
}
