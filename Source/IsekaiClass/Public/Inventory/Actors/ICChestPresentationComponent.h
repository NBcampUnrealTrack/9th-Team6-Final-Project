#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ICChestPresentationComponent.generated.h"
class UStaticMeshComponent;
class AICPlayerController;

/** Reusable hinge: the storage UI is mounted only after the visible lid finishes opening. */
UCLASS(ClassGroup=(IC),meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICChestPresentationComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UICChestPresentationComponent();
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function) override;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Lid;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Chest") FVector HingeOffset;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Chest") FVector LidClosedOffset;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Chest") FRotator OpenRotation=FRotator(0,0,-105);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Chest",meta=(ClampMin="0.1")) float AnimationSeconds=.7f;
    UFUNCTION(BlueprintPure,Category="Chest") float GetOpenAmount() const{return OpenAmount;}
    bool OpenForPlayer(AICPlayerController* Player,FName Container,bool Home);
    UFUNCTION(BlueprintCallable,Category="Chest") void CloseChest();
private:
    float OpenAmount=0;
    bool bOpening=false,bWaitingForUI=false;
    FName ContainerID;
    bool bHome=false;
    TWeakObjectPtr<AICPlayerController> PendingPlayer;
    UFUNCTION() void ResetChest();
    void ApplyPose();
};
