#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ICDebugCheatManager.generated.h"

UCLASS()
class ISEKAICLASS_API UICDebugCheatManager : public UCheatManager
{
    GENERATED_BODY()
public:
    UFUNCTION(Exec) void ICCreateTestPlayer();
    UFUNCTION(Exec) void ICAddGameMinutes(double Minutes);
    /** Advance the game calendar by hours, including fractional hours. */
    UFUNCTION(Exec) void ICAddGameHours(double Hours);
    UFUNCTION(Exec) void ICGiveItem(FName ItemId, int32 Count);
    /** Give one of each starter weapon together. Requires room for all four. */
    UFUNCTION(Exec) void ICGiveStarterWeapons();
    UFUNCTION(Exec) void ICEquipStarterWeapons();
    UFUNCTION(Exec) void ICRecruitTestParty();
    /** Unlock every registered pet by bringing its essence count to at least 100. */
    UFUNCTION(Exec) void ICGiveAllPetEssence();
    /** Assign unlocked pets to the player and current companions for a follow preview. */
    UFUNCTION(Exec) void ICEquipTestPets();
    UFUNCTION(Exec) void ICGiveGold();
    UFUNCTION(Exec) void ICSetAffinity(FName NpcId, int32 Value);
    /** Increase a registered NPC's current affinity, capped at five hearts (500). */
    UFUNCTION(Exec) void ICAddAffinity(FName NpcId, int32 Amount);
    UFUNCTION(Exec) void ICTeleport(float X, float Y, float Z);
    UFUNCTION(Exec) void ICCompleteActiveQuests();
    UFUNCTION(Exec) void ICInteractNearestNPC();
    UFUNCTION(Exec) void ICEndNPCInteraction();
    UFUNCTION(Exec) void ICShopOffers(FName ServiceID);
    UFUNCTION(Exec) void ICShopBuy(FName ServiceID, FName ItemID, int32 Count);
    UFUNCTION(Exec) void ICShopSell(FName ServiceID, FName ItemID, int32 Count);
    /** 모든 생활 오브젝트의 슬롯 점유 상태와 사용 NPC ID를 Output Log에 출력한다. */
    UFUNCTION(Exec) void ICLifeObjectDump();
    /** 1이면 생활 오브젝트 머리 위에 점유 상태를 표시한다(ic.LifeObject.Debug와 같음). */
    UFUNCTION(Exec) void ICLifeObjectDebug(int32 bEnabled);
    /** 가장 가까운 NPC 액터를 제거한다(제거 후 점유가 남지 않는지 확인용). */
    UFUNCTION(Exec) void ICDestroyNearestNPC();
private:
    TWeakObjectPtr<class AICNPCBase> DebugInteractingNPC;
};
