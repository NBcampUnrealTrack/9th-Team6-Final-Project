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
    UFUNCTION(Exec) void ICGiveItem(FName ItemId, int32 Count);
    UFUNCTION(Exec) void ICSetAffinity(FName NpcId, int32 Value);
    UFUNCTION(Exec) void ICTeleport(float X, float Y, float Z);
    UFUNCTION(Exec) void ICCompleteActiveQuests();
    UFUNCTION(Exec) void ICInteractNearestNPC();
    UFUNCTION(Exec) void ICEndNPCInteraction();
    UFUNCTION(Exec) void ICShopOffers(FName ServiceID);
    UFUNCTION(Exec) void ICShopBuy(FName ServiceID, FName ItemID, int32 Count);
    UFUNCTION(Exec) void ICShopSell(FName ServiceID, FName ItemID, int32 Count);
private:
    TWeakObjectPtr<class AICNPCBase> DebugInteractingNPC;
};
