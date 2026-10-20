#pragma once
#include "CoreMinimal.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Data/Rows/World/ICSpawnerRow.h"
#include "ICEnemyBase.generated.h"
UCLASS(Blueprintable)
class ISEKAICLASS_API AICEnemyBase : public AICCharacterBase
{
    GENERATED_BODY()
public:
    AICEnemyBase();
    bool InitializeEnemy(FName SpawnerID,const FICSpawnSlotDefinition& Slot,const FTransform& Origin);
    virtual void HandleZeroHP() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool FlushToWorldState_Implementation(UICWorldStateSubsystem* State) override;
    const FICMonsterRow& GetMonsterDefinition() const { return Definition; }
    FName GetSpawnKey() const { return SpawnKey; }
    bool OutsideLeash() const;
    void BeginReturn();
    void FinishReturn();
    bool IsReturning() const { return bReturning; }
    FVector GetOrigin() const { return OriginTransform.GetLocation(); }
private:
    FICMonsterRow Definition;
    FICSpawnSlotDefinition SlotDefinition;
    FName SpawnerDefinitionID,SpawnKey;
    FTransform OriginTransform;
    bool bReturning=false,bDefeatCommitted=false;
};
