#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Rows/World/ICSpawnerRow.h"
#include "ICPlacedMonsterSpawnPoint.generated.h"

class AICEnemyBase;

/** Runtime lifetime follows the placed monster's level, including after death. */
UCLASS(Transient,NotBlueprintable)
class ISEKAICLASS_API AICPlacedMonsterSpawnPoint : public AActor
{
    GENERATED_BODY()
public:
    AICPlacedMonsterSpawnPoint();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    FGuid PlacementGuid;
    UPROPERTY() FName SpawnerID;
    UPROPERTY() FICSpawnSlotDefinition Slot;
    UPROPERTY() TSubclassOf<AICEnemyBase> MonsterClass;
};
