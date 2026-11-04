#include "NPC/LifeObject/ICLifeObjectSubsystem.h"

#include "NPC/LifeObject/ICLifeObjectActor.h"
#include "NPC/LifeObject/ICLifeObjectUserComponent.h"
#include "NPC/LifeObject/ICLifeObjectBehaviorDefinition.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "SmartObjectSubsystem.h"
#include "SmartObjectComponent.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Debug/ICLog.h"

namespace ICLifeObjectDebug
{
    static TAutoConsoleVariable<int32> CVarDebug(
        TEXT("ic.LifeObject.Debug"),
        0,
        TEXT("1이면 생활 오브젝트 머리 위에 슬롯별 점유 상태와 사용 NPC ID를 표시합니다."),
        ECVF_Cheat);
}

bool UICLifeObjectSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UICLifeObjectSubsystem::Deinitialize()
{
    // 월드 종료 시점에는 Smart Object 런타임도 함께 정리되므로 기록만 비운다.
    Occupants.Reset();
    Objects.Reset();
    Super::Deinitialize();
}

TStatId UICLifeObjectSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UICLifeObjectSubsystem, STATGROUP_Tickables);
}

UICLifeObjectSubsystem* UICLifeObjectSubsystem::Get(const UObject* WorldContext)
{
    const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    return World ? World->GetSubsystem<UICLifeObjectSubsystem>() : nullptr;
}

bool UICLifeObjectSubsystem::IsDebugEnabled()
{
    return ICLifeObjectDebug::CVarDebug.GetValueOnGameThread() != 0;
}

USmartObjectSubsystem* UICLifeObjectSubsystem::GetSmartObjects() const
{
    return USmartObjectSubsystem::GetCurrent(GetWorld());
}

void UICLifeObjectSubsystem::RegisterObject(AICLifeObjectActor* Object)
{
    if (!Object) return;
    Objects.RemoveAll([](const TWeakObjectPtr<AICLifeObjectActor>& Entry) { return !Entry.IsValid(); });
    for (const TWeakObjectPtr<AICLifeObjectActor>& Entry : Objects)
    {
        if (Entry.Get() == Object) return;
        if (!Object->ObjectID.IsNone() && Entry->ObjectID == Object->ObjectID)
            UE_LOG(LogICLifeObject, Warning, TEXT("[LifeObject.DuplicateID] ID=%s A=%s B=%s"), *Object->ObjectID.ToString(), *Entry->GetName(), *Object->GetName());
    }
    Objects.Add(Object);
    UE_LOG(LogICLifeObject, Verbose, TEXT("[LifeObject.Registered] Object=%s Type=%s Slots=%d"),
        *Object->GetDebugID().ToString(), ICLife::LexType(Object->ObjectType), Object->GetSlotCount());
}

void UICLifeObjectSubsystem::UnregisterObject(AICLifeObjectActor* Object)
{
    if (!Object) return;
    // 이 오브젝트를 쓰던 NPC들에게 먼저 알린다. 각 NPC가 ReleaseAll로 자기 예약을 정리한다.
    TArray<TWeakObjectPtr<UICLifeObjectUserComponent>> Users;
    for (const TPair<FSmartObjectSlotHandle, FICLifeSlotOccupant>& Pair : Occupants)
        if (Pair.Value.Object.Get() == Object) Users.Add(Pair.Value.User);
    for (const TWeakObjectPtr<UICLifeObjectUserComponent>& User : Users)
        if (User.IsValid()) User->NotifyClaimRevoked(TEXT("ObjectUnloaded"));

    // 사용자가 이미 없어진 기록도 남기지 않는다(순회 중에 맵을 바꾸지 않도록 먼저 모은다).
    TArray<FSmartObjectClaimHandle> Leftovers;
    for (const TPair<FSmartObjectSlotHandle, FICLifeSlotOccupant>& Pair : Occupants)
        if (Pair.Value.Object.Get() == Object) Leftovers.Add(Pair.Value.Claim);
    for (const FSmartObjectClaimHandle& Claim : Leftovers)
    {
        Release(Claim, TEXT("ObjectUnloaded.Cleanup"));
        Occupants.Remove(Claim.SlotHandle);
    }
    Objects.RemoveAll([Object](const TWeakObjectPtr<AICLifeObjectActor>& Entry) { return !Entry.IsValid() || Entry.Get() == Object; });
    UE_LOG(LogICLifeObject, Verbose, TEXT("[LifeObject.Unregistered] Object=%s"), *Object->GetDebugID().ToString());
}

int32 UICLifeObjectSubsystem::GetObjectCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AICLifeObjectActor>& Entry : Objects) if (Entry.IsValid()) ++Count;
    return Count;
}

int32 UICLifeObjectSubsystem::GatherCandidates(const UICLifeObjectUserComponent& User, const FICLifeRoutine& Routine, float SearchRadiusCm, TArray<FICLifeObjectCandidate>& OutCandidates) const
{
    OutCandidates.Reset();
    USmartObjectSubsystem* SO = GetSmartObjects();
    const AActor* UserActor = User.GetOwner();
    if (!SO || !UserActor || !Routine.WantsObject()) return 0;

    const FName UserID = User.GetUserID();
    const FGameplayTag Occupation = User.GetOccupationTag();
    const FName Region = User.GetRegionID();

    // 탐색 기준점: 시간표 목적지 앵커(로드되어 있으면) → 없으면 NPC 현재 위치.
    // 넓은 오픈 월드에서 다른 집 오브젝트까지 가다가 시간표가 바뀌지 않도록 반경(기본 50m) 안에서만 찾는다.
    FVector Origin = UserActor->GetActorLocation();
    FTransform AnchorTransform;
    if (!Routine.DestinationAnchorID.IsNone() && User.ResolveAnchor(Routine.DestinationAnchorID, AnchorTransform))
        Origin = AnchorTransform.GetLocation();

    // 반경 안에서 본인이 사용할 수 있는 오브젝트(활동 태그 + 소유자/허용 NPC/직업/지역 조건 통과) 중
    // 하나를 무작위로 고른다. 오브젝트마다 난수를 하나씩 부여해서, 자리가 많은 오브젝트가 더 자주 뽑히지 않게 한다.
    for (const TWeakObjectPtr<AICLifeObjectActor>& Entry : Objects)
    {
        AICLifeObjectActor* Object = Entry.Get();
        if (!Object || !Object->IsSmartObjectReady() || Object->IsPlayerLocked()) continue;
        if (!Object->GetEffectiveActivityTags().HasTag(Routine.ActivityTag)) continue;
        if (!Object->CanBeUsedBy(UserID, Occupation, Region)) continue;
        // Work는 부모 태그라 Work.Cook/Farm 같은 모든 하위 활동에도 매칭된다.
        // 근무 중에는 현재 근무 목적지 앵커에 연결한 전용 오브젝트만 허용한다.
        // 연결 슬롯이 없으면 Fallback이 근무 앵커에서 대기한다.
        const bool bLinkedToDestination = Object->IsLinkedToAnchor(Routine.DestinationAnchorID);
        if (Routine.Activity == EICScheduleActivity::Work && !bLinkedToDestination) continue;
        if (SearchRadiusCm > 0.f && FVector::DistSquared(Origin, Object->GetActorLocation()) > FMath::Square(static_cast<double>(SearchRadiusCm))) continue;

        const double ObjectRoll = FMath::FRand();
        TArray<FSmartObjectSlotHandle> Slots;
        SO->GetAllSlots(Object->SmartObject->GetRegisteredHandle(), Slots);
        for (const FSmartObjectSlotHandle& Slot : Slots)
        {
            if (!SO->CanBeClaimed(Slot, ESmartObjectClaimPriority::Normal)) continue;
            FICLifeObjectCandidate& Candidate = OutCandidates.AddDefaulted_GetRef();
            Candidate.Object = Object;
            Candidate.SlotIndex = Slot.GetSlotIndex();
            // 같은 오브젝트 안에서는 빈 자리 중 앞 번호부터 쓴다.
            Candidate.Score = ObjectRoll + Slot.GetSlotIndex() * 1.0e-6;
        }
    }
    // 같은 NPC가 막 사용한 액터를 즉시 다시 고르지 않게 한다. 다른 액터 후보가 하나라도
    // 있으면 이전 액터의 모든 슬롯을 제외하고, 그것뿐이면 그대로 남겨 생활이 멈추지 않는다.
    if (const AICLifeObjectActor* LastUsed = User.GetLastUsedObject())
    {
        const bool bHasAlternativeObject = OutCandidates.ContainsByPredicate([LastUsed](const FICLifeObjectCandidate& Candidate)
        {
            return Candidate.Object.IsValid() && Candidate.Object.Get() != LastUsed;
        });
        if (bHasAlternativeObject)
        {
            OutCandidates.RemoveAll([LastUsed](const FICLifeObjectCandidate& Candidate)
            {
                return Candidate.Object.Get() == LastUsed;
            });
        }
    }
    OutCandidates.Sort([](const FICLifeObjectCandidate& A, const FICLifeObjectCandidate& B) { return A.Score < B.Score; });
    return OutCandidates.Num();
}

bool UICLifeObjectSubsystem::TryClaim(UICLifeObjectUserComponent& User, AICLifeObjectActor& Object, int32 SlotIndex, FSmartObjectClaimHandle& OutClaim)
{
    OutClaim = FSmartObjectClaimHandle::InvalidHandle;
    USmartObjectSubsystem* SO = GetSmartObjects();
    FSmartObjectSlotHandle Slot;
    if (!SO || Object.IsPlayerLocked() || !Object.GetSlotHandle(SlotIndex, Slot)) return false;
    if (!SO->CanBeClaimed(Slot, ESmartObjectClaimPriority::Normal)) return false;

    const FSmartObjectActorUserData UserData(User.GetOwner());
    const FConstStructView UserView = FConstStructView::Make(UserData);
    // Definition 에셋에 선택 조건(Preconditions)을 넣었다면 여기서 함께 검사된다.
    if (!SO->EvaluateSelectionConditions(Slot, UserView)) return false;

    // 같은 우선순위(Normal)끼리는 이미 예약된 슬롯을 빼앗을 수 없으므로, 동시에 요청해도 한 명만 성공한다.
    const FSmartObjectClaimHandle Claim = SO->MarkSlotAsClaimed(Slot, ESmartObjectClaimPriority::Normal, UserView);
    if (!Claim.IsValid()) return false;

    SO->RegisterSlotInvalidationCallback(Claim, FOnSlotInvalidated::CreateUObject(&User, &UICLifeObjectUserComponent::HandleSlotInvalidated));

    FICLifeSlotOccupant& Occupant = Occupants.FindOrAdd(Slot);
    Occupant.Object = &Object;
    Occupant.User = &User;
    Occupant.UserID = User.GetUserID();
    Occupant.SlotIndex = SlotIndex;
    Occupant.Claim = Claim;
    Occupant.bOccupied = false;
    Occupant.SinceRealSeconds = FPlatformTime::Seconds();

    OutClaim = Claim;
    UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.Claim] Object=%s Slot=%d NPC=%s"), *Object.GetDebugID().ToString(), SlotIndex, *Occupant.UserID.ToString());
    return true;
}

const USmartObjectBehaviorDefinition* UICLifeObjectSubsystem::MarkOccupied(const FSmartObjectClaimHandle& Claim, bool& bOutMarked)
{
    bOutMarked = false;
    USmartObjectSubsystem* SO = GetSmartObjects();
    if (!SO || !Claim.IsValid()) return nullptr;
    const USmartObjectBehaviorDefinition* Behavior = SO->MarkSlotAsOccupied(Claim, UICLifeObjectBehaviorDefinition::StaticClass());
    bOutMarked = Behavior != nullptr;
    if (FICLifeSlotOccupant* Occupant = Occupants.Find(Claim.SlotHandle))
    {
        if (Occupant->Claim == Claim) Occupant->bOccupied = true;
        if (!bOutMarked)
        {
            // 예약(Claim)은 유지되므로 중복 사용은 계속 막힌다. Definition에 행동 정의를 넣으라는 안내만 남긴다.
            UE_LOG(LogICLifeObject, Warning, TEXT("[LifeObject.NoBehaviorDefinition] Object=%s Slot=%d Definition 에셋의 Default Behavior Definitions에 'IC Life Object Behavior'를 추가하세요."),
                Occupant->Object.IsValid() ? *Occupant->Object->GetDebugID().ToString() : TEXT("?"), Occupant->SlotIndex);
        }
        UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.Occupy] Object=%s Slot=%d NPC=%s"),
            Occupant->Object.IsValid() ? *Occupant->Object->GetDebugID().ToString() : TEXT("?"), Occupant->SlotIndex, *Occupant->UserID.ToString());
    }
    return Behavior;
}

void UICLifeObjectSubsystem::Release(const FSmartObjectClaimHandle& Claim, FName Reason, bool bFromInvalidation)
{
    if (!Claim.IsValid()) return;
    USmartObjectSubsystem* SO = GetSmartObjects();
    // 무효화 콜백 안에서는 Smart Object 런타임을 건드리지 않는다(이미 정리 중인 슬롯이다). 기록만 지운다.
    if (SO && !bFromInvalidation)
    {
        SO->UnregisterSlotInvalidationCallback(Claim);
        SO->MarkSlotAsFree(Claim); // 이미 풀린 슬롯이면 false를 반환할 뿐 안전하다.
    }
    if (const FICLifeSlotOccupant* Occupant = Occupants.Find(Claim.SlotHandle); Occupant && Occupant->Claim == Claim)
    {
        UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.Release] Object=%s Slot=%d NPC=%s Reason=%s"),
            Occupant->Object.IsValid() ? *Occupant->Object->GetDebugID().ToString() : TEXT("?"), Occupant->SlotIndex,
            *Occupant->UserID.ToString(), *Reason.ToString());
        Occupants.Remove(Claim.SlotHandle);
    }
}

bool UICLifeObjectSubsystem::GetSlotTransform(const FSmartObjectClaimHandle& Claim, FTransform& OutTransform) const
{
    const USmartObjectSubsystem* SO = GetSmartObjects();
    if (!SO || !Claim.IsValid()) return false;
    const TOptional<FTransform> Transform = SO->GetSlotTransform(Claim.SlotHandle);
    if (!Transform.IsSet()) return false;
    OutTransform = Transform.GetValue();
    return true;
}

bool UICLifeObjectSubsystem::IsClaimAlive(const FSmartObjectClaimHandle& Claim) const
{
    if (!Claim.IsValid()) return false;
    const FICLifeSlotOccupant* Occupant = Occupants.Find(Claim.SlotHandle);
    if (!Occupant || Occupant->Claim != Claim || !Occupant->Object.IsValid()) return false;
    const USmartObjectSubsystem* SO = GetSmartObjects();
    if (!SO || !SO->IsClaimedSmartObjectValid(Claim)) return false;
    // 오브젝트는 살아 있어도 슬롯이 다른 경로로 풀렸다면 더 이상 내 예약이 아니다.
    const ESmartObjectSlotState State = SO->GetSlotState(Claim.SlotHandle);
    return State == ESmartObjectSlotState::Claimed || State == ESmartObjectSlotState::Occupied;
}

bool UICLifeObjectSubsystem::TryLockForPlayer(AICLifeObjectActor& Object, AActor* Player, FText& OutFailure)
{
    if(Object.IsPlayerLocked()||Object.IsOccupiedByAnyNPC())
    {OutFailure=NSLOCTEXT("ICLifeObject","BedAlreadyUsed","다른 사람이 사용 중인 침대입니다.");return false;}
    TArray<const FICLifeSlotOccupant*> Current;
    GetOccupantsOf(Object, Current);
    for (const FICLifeSlotOccupant* Occupant : Current)
    {
        // 이미 누워 있거나(점유) 침대로 가는 중(예약)인 NPC가 있으면 플레이어는 사용할 수 없다.
        if (Occupant->bOccupied || Object.ObjectType == EICLifeObjectType::Bed)
        {
            OutFailure = NSLOCTEXT("ICLifeObject", "InUseByNPC", "다른 사람이 사용 중입니다.");
            UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.PlayerRejected] Object=%s NPC=%s"), *Object.GetDebugID().ToString(), *Occupant->UserID.ToString());
            return false;
        }
    }
    Object.SetPlayerLockedInternal(true, Player);
    SetSlotsEnabled(Object, false);
    UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.PlayerLock] Object=%s"), *Object.GetDebugID().ToString());
    return true;
}

void UICLifeObjectSubsystem::UnlockForPlayer(AICLifeObjectActor& Object)
{
    if (!Object.IsPlayerLocked()) return;
    Object.SetPlayerLockedInternal(false, nullptr);
    SetSlotsEnabled(Object, true);
    UE_LOG(LogICLifeObject, Log, TEXT("[LifeObject.PlayerUnlock] Object=%s"), *Object.GetDebugID().ToString());
}

void UICLifeObjectSubsystem::SetSlotsEnabled(AICLifeObjectActor& Object, bool bEnabled) const
{
    USmartObjectSubsystem* SO = GetSmartObjects();
    if (!SO || !Object.IsSmartObjectReady()) return;
    TArray<FSmartObjectSlotHandle> Slots;
    SO->GetAllSlots(Object.SmartObject->GetRegisteredHandle(), Slots);
    for (const FSmartObjectSlotHandle& Slot : Slots) SO->SetSlotEnabled(Slot, bEnabled);
}

void UICLifeObjectSubsystem::GetOccupantsOf(const AICLifeObjectActor& Object, TArray<const FICLifeSlotOccupant*>& OutOccupants) const
{
    OutOccupants.Reset();
    for (const TPair<FSmartObjectSlotHandle, FICLifeSlotOccupant>& Pair : Occupants)
        if (Pair.Value.Object.Get() == &Object) OutOccupants.Add(&Pair.Value);
    OutOccupants.Sort([](const FICLifeSlotOccupant& A, const FICLifeSlotOccupant& B) { return A.SlotIndex < B.SlotIndex; });
}

void UICLifeObjectSubsystem::Tick(float DeltaTime)
{
    WatchdogAccumulator += DeltaTime;
    if (WatchdogAccumulator >= 1.f)
    {
        WatchdogAccumulator = 0.f;
        RunWatchdog();
        UpdatePlayerLocks();
    }

    const bool bDebug = IsDebugEnabled();
    DebugAccumulator += DeltaTime;
    if (bDebug && DebugAccumulator >= 0.25f)
    {
        DebugAccumulator = 0.f;
        RefreshDebugLabels();
    }
    else if (!bDebug && bDebugWasEnabled)
    {
        for (const TWeakObjectPtr<AICLifeObjectActor>& Entry : Objects)
            if (AICLifeObjectActor* Object = Entry.Get()) Object->SetDebugText(Object->GetDebugID().ToString(), false);
    }
    bDebugWasEnabled = bDebug;
}

void UICLifeObjectSubsystem::RunWatchdog()
{
    // 사용자가 사라졌거나 더 이상 그 예약을 들고 있지 않은데 남아 있는 기록 = 영구 점유 위험.
    TArray<FSmartObjectClaimHandle> Orphans;
    for (const TPair<FSmartObjectSlotHandle, FICLifeSlotOccupant>& Pair : Occupants)
    {
        const FICLifeSlotOccupant& Occupant = Pair.Value;
        const UICLifeObjectUserComponent* User = Occupant.User.Get();
        if (!User || !User->OwnsClaim(Occupant.Claim) || !Occupant.Object.IsValid()) Orphans.Add(Occupant.Claim);
    }
    for (const FSmartObjectClaimHandle& Claim : Orphans)
    {
        ++OrphanReleaseCount;
        UE_LOG(LogICLifeObject, Warning, TEXT("[LifeObject.OrphanReleased] Slot=%s 주인 없는 예약을 강제로 해제했습니다. 해제 누락 경로를 확인하세요."), *LexToString(Claim.SlotHandle));
        Release(Claim, TEXT("Watchdog"));
        Occupants.Remove(Claim.SlotHandle);
    }
}

void UICLifeObjectSubsystem::UpdatePlayerLocks()
{
    for (const TWeakObjectPtr<AICLifeObjectActor>& Entry : Objects)
    {
        AICLifeObjectActor* Object = Entry.Get();
        if (!Object || !Object->IsPlayerLocked()) continue;
        const AActor* Player = Object->GetLockingPlayer();
        const bool bPlayerAway = !Player
            || FVector::DistSquared2D(Player->GetActorLocation(), Object->GetActorLocation()) > FMath::Square(Object->PlayerLockReleaseDistanceCm);
        if (bPlayerAway) UnlockForPlayer(*Object);
    }
}

void UICLifeObjectSubsystem::RefreshDebugLabels()
{
    USmartObjectSubsystem* SO = GetSmartObjects();
    for (const TWeakObjectPtr<AICLifeObjectActor>& Entry : Objects)
    {
        AICLifeObjectActor* Object = Entry.Get();
        if (!Object) continue;
        // TextRender 기본 폰트는 한글이 깨질 수 있어 영문으로 표시한다.
        FString Text = FString::Printf(TEXT("%s [%s]"), *Object->GetDebugID().ToString(), ICLife::LexType(Object->ObjectType));
        if (!Object->IsSmartObjectReady()) Text += TEXT("\n(SmartObject not ready)");
        if (Object->IsPlayerLocked()) Text += TEXT("\nPLAYER LOCK");
        const int32 SlotCount = Object->GetSlotCount();
        for (int32 Index = 0; Index < SlotCount; ++Index)
        {
            FString Line = FString::Printf(TEXT("\n[%d] Free"), Index);
            FSmartObjectSlotHandle Slot;
            if (Object->GetSlotHandle(Index, Slot))
            {
                if (const FICLifeSlotOccupant* Occupant = Occupants.Find(Slot))
                    Line = FString::Printf(TEXT("\n[%d] %s %s"), Index, Occupant->bOccupied ? TEXT("Occupied") : TEXT("Claimed"), *Occupant->UserID.ToString());
                else if (SO && SO->GetSlotState(Slot) != ESmartObjectSlotState::Free)
                    Line = FString::Printf(TEXT("\n[%d] Busy(untracked)"), Index);
            }
            Text += Line;
        }
        Object->SetDebugText(Text, true);
    }
}

void UICLifeObjectSubsystem::DumpToLog() const
{
    UE_LOG(LogICLifeObject, Display, TEXT("[LifeObject.Dump] Objects=%d ActiveClaims=%d OrphanReleased=%d"), GetObjectCount(), Occupants.Num(), OrphanReleaseCount);
    for (const TWeakObjectPtr<AICLifeObjectActor>& Entry : Objects)
    {
        const AICLifeObjectActor* Object = Entry.Get();
        if (!Object) continue;
        TArray<const FICLifeSlotOccupant*> Current;
        GetOccupantsOf(*Object, Current);
        FString Users;
        for (const FICLifeSlotOccupant* Occupant : Current)
            Users += FString::Printf(TEXT(" [%d:%s:%s]"), Occupant->SlotIndex, *Occupant->UserID.ToString(), Occupant->bOccupied ? TEXT("Occupied") : TEXT("Claimed"));
        UE_LOG(LogICLifeObject, Display, TEXT("  %s Type=%s Region=%s Anchor=%s Owner=%s Slots=%d Ready=%d PlayerLock=%d Users=%s"),
            *Object->GetDebugID().ToString(), ICLife::LexType(Object->ObjectType), *Object->RegionID.ToString(), *Object->LinkedAnchorID.ToString(),
            *Object->OwnerNPCID.ToString(), Object->GetSlotCount(), Object->IsSmartObjectReady() ? 1 : 0, Object->IsPlayerLocked() ? 1 : 0,
            Users.IsEmpty() ? TEXT(" -") : *Users);
    }
}
