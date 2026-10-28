#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ICMapPresentationRow.generated.h"

/** Shared by the HUD overview and the clickable travel map. Projection/image live in DT_IC_Map. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICMapPresentationRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName MapID=TEXT("MAP_TEST_001");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) int32 TravelCostGold=500;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.3")) float LoadingSeconds=1.2f;
    /** Test-map convenience. Disable to require visiting each team's Warp Stone first. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bUnlockPlacedPortals=true;
};
