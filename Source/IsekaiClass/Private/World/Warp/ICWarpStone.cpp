#include "World/Warp/ICWarpStone.h"

#include "Core/Controller/ICPlayerController.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "World/Warp/ICPartyWarpComponent.h"

AICWarpStone::AICWarpStone()
{
    Action = TEXT("Warp");
    InteractionText = NSLOCTEXT("ICWarpStone", "Interaction", "워프 돌 조사하기");
}

bool AICWarpStone::TryInteract_Implementation(AICPlayerController* Controller)
{
    FString Reason;
    AICPlayerCharacter* Player = Controller ? Cast<AICPlayerCharacter>(Controller->GetPawn()) : nullptr;
    if (!Player || !Player->PartyWarp) Reason = TEXT("플레이어의 워프 기능을 찾을 수 없습니다.");
    else if (WarpID.IsNone()) Reason = TEXT("이 워프 돌에 WarpID가 설정되지 않았습니다.");
    else if (Player->PartyWarp->UnlockWarp(WarpID, GetActorLocation(), Reason))
    {
        Controller->ShowLocalized(NSLOCTEXT("ICWarp", "StoneRegistered", "워프 돌이 활성화되었습니다."));
        Controller->SetMenu(TEXT("Map"));
        return true;
    }
    // The controller otherwise replaces a false return value with its generic
    // interaction error, hiding the useful warp-specific failure reason.
    if (Controller && !Reason.IsEmpty())
    {
        Controller->Show(FString::Printf(TEXT("[%s] %s"), *GetName(), *Reason));
        return true;
    }
    return false;
}

TArray<FName> AICWarpStone::GetUnlockedDestinations(AICPlayerCharacter* Player) const
{
    return Player && Player->PartyWarp ? Player->PartyWarp->GetUnlockedWarpIDs() : TArray<FName>();
}

bool AICWarpStone::WarpToDestination(AICPlayerCharacter* Player, FName DestinationWarpID, FString& OutReason)
{
    if (!Player || !Player->PartyWarp)
    {
        OutReason = TEXT("플레이어의 워프 기능을 찾을 수 없습니다.");
        return false;
    }
    return Player->PartyWarp->WarpParty(DestinationWarpID, CostGold, OutReason);
}
