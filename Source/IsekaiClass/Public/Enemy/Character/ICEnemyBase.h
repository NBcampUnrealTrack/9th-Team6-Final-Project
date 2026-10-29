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
    virtual void BeginPlay() override;
    virtual void PostActorCreated() override;
    virtual void PostLoad() override;
#if WITH_EDITOR
    virtual void PostEditImport() override;
#endif
    bool InitializeEnemy(FName SpawnerID,const FICSpawnSlotDefinition& Slot,const FTransform& Origin,bool bRestoreSavedSlot=false);
    bool HasInitializedEnemy() const { return bEnemyInitialized; }
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="IC|Monster") FName MonsterID;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="IC|Monster|Placement") bool bInitializeWhenPlaced=true;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="IC|Monster|Placement",meta=(ClampMin="50",Units="cm")) double PlacedLeashRadiusCm=1000.;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,NonPIEDuplicateTransient,Category="IC|Monster|Placement") FGuid PlacementGuid;
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
    UFUNCTION(BlueprintPure,Category="IC|UI") bool ShouldShowHealthBar() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|UI") TObjectPtr<class UWidgetComponent> HealthBar;
private:
    FICMonsterRow Definition;
    FICSpawnSlotDefinition SlotDefinition;
    FName SpawnerDefinitionID,SpawnKey;
    FTransform OriginTransform;
    bool bReturning=false,bDefeatCommitted=false;
    bool bEnemyInitialized=false;
    bool bPreserveSavedSlot=false;
    FGuid PresentedAction;
    double HitAnimationUntil=0;
    double HealthBarHitUntil=-1;
    UPROPERTY(Transient) TObjectPtr<class UAnimSequence> PresentedAnimation;
    void PresentAnimation(class UAnimSequence* Animation, bool Loop, float Rate=1);
};
