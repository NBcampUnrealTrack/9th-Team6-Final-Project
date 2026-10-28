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
    virtual void Tick(float DeltaSeconds) override;
    virtual bool FlushToWorldState_Implementation(UICWorldStateSubsystem* State) override;
    const FICMonsterRow& GetMonsterDefinition() const { return Definition; }
    FName GetSpawnKey() const { return SpawnKey; }
    bool OutsideLeash() const;
    bool IsOutsideLeash(const FVector& WorldLocation) const;
    void BeginReturn();
    void FinishReturn();
    bool IsReturning() const { return bReturning; }
    FVector GetOrigin() const { return OriginTransform.GetLocation(); }
    void PlayHitReaction();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|UI") TObjectPtr<class UWidgetComponent> HealthBar;
private:
    FICMonsterRow Definition;
    FICSpawnSlotDefinition SlotDefinition;
    FName SpawnerDefinitionID,SpawnKey;
    FTransform OriginTransform;
    bool bReturning=false,bDefeatCommitted=false;
    bool bEnemyInitialized=false;
    FGuid PresentedAction;
    double HitAnimationUntil=0;
    UPROPERTY(Transient) TObjectPtr<class UAnimSequence> PresentedAnimation;
    void PresentAnimation(class UAnimSequence* Animation, bool Loop, float Rate=1);
};
