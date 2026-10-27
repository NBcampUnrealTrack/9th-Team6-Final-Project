#include "World/Warp/ICPartyWarpComponent.h"

#include "Character/Base/ICCharacterBase.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/World/ICWarpRow.h"
#include "Debug/ICLog.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICStateParticipant.h"
#include "World/State/ICWorldStateSubsystem.h"

UICPartyWarpComponent::UICPartyWarpComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

AICPlayerCharacter* UICPartyWarpComponent::GetPlayerCharacter() const
{
    return Cast<AICPlayerCharacter>(GetOwner());
}

UICWorldStateSubsystem* UICPartyWarpComponent::GetWorldState() const
{
    const AICPlayerCharacter* Player = GetPlayerCharacter();
    const UGameInstance* GameInstance = Player ? Player->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
}

bool UICPartyWarpComponent::ResolveWarp(FName WarpID, FICWarpRow& OutWarp, FString& OutReason) const
{
    OutReason.Empty();
    if (WarpID.IsNone()) { OutReason = TEXT("워프 ID가 없습니다."); return false; }

    const AICPlayerCharacter* Player = GetPlayerCharacter();
    UGameInstance* GameInstance = Player ? Player->GetGameInstance() : nullptr;
    UICDefinitionSubsystem* Definitions = GameInstance ? GameInstance->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
    if (!Definitions) { OutReason = TEXT("정의 데이터를 찾을 수 없습니다."); return false; }

    FICDefinitionResult Result;
    const FICWarpRow* Warp = Definitions->Find<FICWarpRow>(WarpID, Result);
    if (!Warp)
    {
        OutReason = FString::Printf(TEXT("워프 데이터 '%s'를 찾을 수 없습니다. (%s)"),
            *WarpID.ToString(), Result.Reason.IsEmpty() ? TEXT("DT_IC_Warp 행/카탈로그를 확인하세요.") : *Result.Reason);
        return false;
    }
    if (Warp->ArrivalAnchorID.IsNone())
    {
        OutReason = FString::Printf(TEXT("워프 데이터 '%s'의 Arrival Anchor ID가 비어 있습니다."), *WarpID.ToString());
        return false;
    }
    OutWarp = *Warp;
    return true;
}

bool UICPartyWarpComponent::UnlockWarp(FName WarpID, FVector StoneLocation, FString& OutReason)
{
    FICWarpRow Warp;
    if (!ResolveWarp(WarpID, Warp, OutReason)) return false;

    AICPlayerCharacter* Player = GetPlayerCharacter();
    UICWorldStateSubsystem* State = GetWorldState();
    if (!Player || !State) { OutReason = TEXT("워프 시스템을 초기화할 수 없습니다."); return false; }

    const FVector PlayerLocation = Player->GetActorLocation();
    const double DistanceCm = FVector::Distance(PlayerLocation, StoneLocation);
    if (DistanceCm > Warp.VisitRadiusCm)
    {
        OutReason = FString::Printf(
            TEXT("워프 돌 범위 밖입니다. 거리: %.0fcm / 허용: %.0fcm (플레이어: %s, 돌: %s)"),
            DistanceCm, Warp.VisitRadiusCm, *PlayerLocation.ToCompactString(), *StoneLocation.ToCompactString());
        return false;
    }

    FICWorldState Next = State->ExportState();
    Next.UnlockedWarps.Add(Warp.ID);
    if (!State->ApplyWorldProgress(Next)) { OutReason = TEXT("워프게이트 해금 상태를 저장하지 못했습니다."); return false; }
    UE_LOG(LogICWorldState, Log, TEXT("[Warp.Unlocked] Warp=%s"), *Warp.ID.ToString());
    return true;
}

bool UICPartyWarpComponent::IsWarpUnlocked(FName WarpID) const
{
    const UICWorldStateSubsystem* State = GetWorldState();
    return State && !WarpID.IsNone() && State->ExportState().UnlockedWarps.Contains(WarpID);
}

bool UICPartyWarpComponent::CanWarp(FName DestinationWarpID, int32 CostGold, FString& OutReason) const
{
    if (CostGold < 0) { OutReason = TEXT("워프 비용은 0 이상이어야 합니다."); return false; }
    FICWarpRow Destination;
    if (!ResolveWarp(DestinationWarpID, Destination, OutReason)) return false;
    if (!IsWarpUnlocked(DestinationWarpID)) { OutReason = TEXT("아직 해금하지 않은 워프게이트입니다."); return false; }

    const UICWorldStateSubsystem* State = GetWorldState();
    if (!State || State->ExportState().Gold < CostGold) { OutReason = TEXT("골드가 부족합니다."); return false; }

    const AICPlayerCharacter* Player = GetPlayerCharacter();
    const UICSpawnSubsystem* Spawns = Player ? Player->GetWorld()->GetSubsystem<UICSpawnSubsystem>() : nullptr;
    FTransform ArrivalTransform;
    if (!Spawns || !Spawns->ResolveLoadedAnchor(Destination.ArrivalAnchorID, ArrivalTransform))
    {
        OutReason = TEXT("워프 도착 지점이 현재 맵에 준비되지 않았습니다.");
        return false;
    }
    return true;
}

bool UICPartyWarpComponent::WarpParty(FName DestinationWarpID, int32 CostGold, FString& OutReason)
{
    if (!CanWarp(DestinationWarpID, CostGold, OutReason)) return false;

    FICWarpRow Destination;
    if (!ResolveWarp(DestinationWarpID, Destination, OutReason)) return false;

    AICPlayerCharacter* Player = GetPlayerCharacter();
    UICWorldStateSubsystem* State = GetWorldState();
    UICSpawnSubsystem* Spawns = Player ? Player->GetWorld()->GetSubsystem<UICSpawnSubsystem>() : nullptr;
    FTransform ArrivalTransform;
    if (!Player || !State || !Spawns || !Spawns->ResolveLoadedAnchor(Destination.ArrivalAnchorID, ArrivalTransform))
    {
        OutReason = TEXT("워프 도착 지점을 준비할 수 없습니다.");
        return false;
    }

    const FICWorldState Snapshot = State->ExportState();
    const FTransform PlayerBefore(Player->GetActorRotation(), Player->GetActorLocation());
    if (!Player->TeleportTo(ArrivalTransform.GetLocation(), ArrivalTransform.Rotator()))
    {
        OutReason = TEXT("플레이어를 워프게이트로 이동하지 못했습니다.");
        return false;
    }
    Player->GetCharacterMovement()->StopMovementImmediately();

    struct FMovedCompanion { AICCharacterBase* Character = nullptr; FTransform Before; };
    TArray<FMovedCompanion> MovedCompanions;
    for (const TPair<FName, FICCharacterState>& Pair : Snapshot.Characters)
    {
        const FICCharacterState& Member = Pair.Value;
        if (Member.Kind != EICCharacterKind::Recruitable || Member.Mode != EICCharacterMode::Companion || Member.PartySlot < 0 || Member.IsDowned()) continue;

        AICCharacterBase* Companion = Spawns->FindCharacter(Pair.Key);
        FTransform Placement;
        const FTransform CompanionBefore = Companion ? FTransform(Companion->GetActorRotation(), Companion->GetActorLocation()) : FTransform::Identity;
        if (!Companion || !Spawns->FindSafePartyPlacement(Pair.Key, Placement) || !Companion->TeleportTo(Placement.GetLocation(), Placement.Rotator()))
        {
            for (const FMovedCompanion& Moved : MovedCompanions)
                if (IsValid(Moved.Character)) Moved.Character->TeleportTo(Moved.Before.GetLocation(), Moved.Before.Rotator());
            Player->TeleportTo(PlayerBefore.GetLocation(), PlayerBefore.Rotator());
            OutReason = TEXT("동료가 안전하게 도착할 위치를 찾지 못했습니다.");
            return false;
        }
        Companion->GetCharacterMovement()->StopMovementImmediately();
        MovedCompanions.Add({Companion, CompanionBefore});
    }

    // Positions are persisted only after every actor reached the destination.
    if (!State->SetPlayerTransform(Player->GetActorLocation(), Player->GetActorRotation()))
    {
        OutReason = TEXT("플레이어 위치를 저장하지 못했습니다.");
        return false;
    }
    for (const FMovedCompanion& Moved : MovedCompanions)
        if (IsValid(Moved.Character)) IICStateParticipant::Execute_FlushToWorldState(Moved.Character, State);

    if (!State->SetPlayerGold(Snapshot.Gold - CostGold))
    {
        OutReason = TEXT("워프 비용을 처리하지 못했습니다.");
        return false;
    }

    UE_LOG(LogICWorldState, Log, TEXT("[Warp.Completed] Destination=%s Cost=%d Companions=%d"), *DestinationWarpID.ToString(), CostGold, MovedCompanions.Num());
    return true;
}

TArray<FName> UICPartyWarpComponent::GetUnlockedWarpIDs() const
{
    TArray<FName> WarpIDs;
    if (const UICWorldStateSubsystem* State = GetWorldState())
        for (const FName& WarpID : State->ExportState().UnlockedWarps) WarpIDs.Add(WarpID);
    WarpIDs.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
    return WarpIDs;
}
