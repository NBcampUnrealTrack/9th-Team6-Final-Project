#pragma once
#include "CoreMinimal.h"
#include "Character/Base/ICCharacterBase.h"
#include "Data/Rows/Combat/ICMonsterRows.h"
#include "Data/Rows/World/ICSpawnerRow.h"
#include "ICEnemyBase.generated.h"
UCLASS(Blueprintable,meta=(ToolTip="몬스터의 직접 전투 수치·스킬·귀환·사망을 실행합니다. 주 스탯과 SP는 생성하지 않으며 지속 키는 스포너 ID와 슬롯 번호입니다. 생존 상태로 셀을 떠나면 원점·최대 자원으로 초기화됩니다."))
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
