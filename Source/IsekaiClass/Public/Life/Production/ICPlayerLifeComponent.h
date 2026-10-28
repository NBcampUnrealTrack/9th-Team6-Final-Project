#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "Data/Rows/Presentation/ICSleepPresentationRow.h"
#include "ICPlayerLifeComponent.generated.h"
class AICPlayerCharacter;
class UAnimMontage;
class UUserWidget;
class AICLifeActor;
class AICFishingLine;
class AICLifeObjectActor;
struct FICProductionRow;

/** Temporary work tools/poses and the player's visible sleep transition. */
UCLASS(ClassGroup=(IC),meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICPlayerLifeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICPlayerLifeComponent();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* Tick) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool BeginWork(EICLifeActivity Activity,const FICProductionRow* Production,AActor* Focus);
    UFUNCTION(BlueprintCallable) void EndWork();
    UFUNCTION(BlueprintCallable) bool BeginSleep(AActor* Bed);
    UFUNCTION(BlueprintPure) bool IsWorking() const {return bWorking;}
    UFUNCTION(BlueprintPure) bool IsSleeping() const {return bSleeping;}
    UFUNCTION(BlueprintPure) bool IsSleepPoseActive() const{return bSleeping&&bSleepPoseActive;}
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Sleep") TSoftObjectPtr<UDataTable> SleepPresentationTable;
    UFUNCTION(BlueprintPure,Category="Fishing") AICLifeActor* ResolveFishingCast(FVector& Landing) const;
    UFUNCTION(BlueprintPure,Category="Fishing") AICFishingLine* GetFishingLine() const {return FishingLine;}
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Fishing",meta=(ClampMin="100")) float FishingCastRangeCm=1200.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Fishing") TSoftClassPtr<AICFishingLine> FishingLineClass;
    bool AttachFishingLine(AICLifeActor* Zone,const FVector& Landing);
    FTransform GetSleepAnchor() const;
private:
    bool bWorking=false,bSleeping=false,bSleepCommitted=false;
    float SleepElapsed=0;
    bool bSleepPoseActive=false;
    FICSleepPresentationRow SleepRules;
    void SetSleepPresentation(bool bInBed);
    UPROPERTY(Transient) TObjectPtr<class ACameraActor> SleepCamera;
    TWeakObjectPtr<AActor> SleepPreviousViewTarget;
    FTransform BedTransform;
    FRotator WakeRotation;
    FVector WakePosition;
    TWeakObjectPtr<AICLifeObjectActor> SleepingLifeBed;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> WorkMontage;
    UPROPERTY(Transient) TObjectPtr<AActor> Tool;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> Fade;
    UPROPERTY(Transient) TObjectPtr<AICFishingLine> FishingLine;
};
