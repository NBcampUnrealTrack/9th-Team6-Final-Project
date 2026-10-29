#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ICSpawnSubsystem.generated.h"
class AICWorldAnchor;
class AICNPCBase;
class AICEnemyBase;
class AICCharacterBase;
class AICPlacedMonsterSpawnPoint;
struct FICSpawnSlotDefinition;
struct FICWorldState;
UCLASS()
class ISEKAICLASS_API UICSpawnSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    bool RegisterAnchor(AICWorldAnchor* Anchor);
    bool RegisterPlacedMonster(AICEnemyBase* Monster);
    void UnregisterPlacedMonster(AICPlacedMonsterSpawnPoint* Point);
    static bool IsPlacedSpawnerID(FName ID);
    void UnregisterAnchor(AICWorldAnchor* Anchor);
    bool ResolveLoadedAnchor(FName ID,FTransform& Transform) const;
    bool FindSafePartyPlacement(FName CharacterID,FTransform& Transform) const;
    AICNPCBase* ReserveReviveTarget(FName ID,class UICAbilitySystemComponent* Caster,FString& Reason);
    bool CanReviveAtCurrentPlacement(AICCharacterBase* Target) const;
    bool FlushActors();
    bool CanRestoreActors(const FICWorldState& State) const;
    void RestoreActors();
    UFUNCTION(BlueprintPure,Category="IC|Spawn") AICCharacterBase* FindCharacter(FName CharacterID) const;
    UFUNCTION(BlueprintCallable,Category="IC|Spawn") void RefreshSpawns();
private:
    TMap<FName,TWeakObjectPtr<AICWorldAnchor>> Anchors;
    TMap<FName,TWeakObjectPtr<AICNPCBase>> NPCs;
    TMap<FName,TWeakObjectPtr<AICEnemyBase>> Enemies;
    TMap<FName,TWeakObjectPtr<AICPlacedMonsterSpawnPoint>> PlacedMonsterPoints;
    TMap<FName,TWeakObjectPtr<class UICAbilitySystemComponent>> ReviveReservations;
    TSet<FName> NPCsWaitingForAnchor;
    bool bReady=false,bRefreshing=false,bTearingDown=false;
    double NextRefreshAt=-1;
    void RefreshEnemySlot(FName SpawnerID,const FICSpawnSlotDefinition& Slot,const FTransform& Origin,UClass* Class,bool bPlaced);
    UFUNCTION() void WorldRestored();
};
