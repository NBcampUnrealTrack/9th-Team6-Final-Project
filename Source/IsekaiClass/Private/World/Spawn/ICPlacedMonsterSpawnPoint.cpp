#include "World/Spawn/ICPlacedMonsterSpawnPoint.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AICPlacedMonsterSpawnPoint::AICPlacedMonsterSpawnPoint()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("SpawnOrigin"));
    PrimaryActorTick.bCanEverTick=false;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void AICPlacedMonsterSpawnPoint::EndPlay(const EEndPlayReason::Type Reason)
{
    if(auto* Spawns=GetWorld()->GetSubsystem<UICSpawnSubsystem>())Spawns->UnregisterPlacedMonster(this);
    Super::EndPlay(Reason);
}
