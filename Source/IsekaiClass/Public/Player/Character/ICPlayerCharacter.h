#pragma once
#include "CoreMinimal.h"
#include "Character/Base/ICCharacterBase.h"
#include "Animation/AnimInstance.h"
#include "ICPlayerCharacter.generated.h"
class UCameraComponent;
class UICInventoryComponent;
class UICCraftingComponent;
class UICQuestGuideComponent;
class UICPartyWarpComponent;
class USpringArmComponent;
class USkeletalMesh;
class UICPlayerLifeComponent;
USTRUCT(BlueprintType)
struct FICPlayerAppearancePreset
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance") TObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance") TSubclassOf<UAnimInstance> AnimClass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance") FTransform MeshTransform=FTransform(FRotator(0,-90,0),FVector(0,0,-88));
};
UCLASS(Blueprintable)
class ISEKAICLASS_API AICPlayerCharacter : public AICCharacterBase
{
    GENERATED_BODY()
public:
    AICPlayerCharacter();
    bool bSprintHeld = false;
    FVector MovementIntent=FVector::ZeroVector;
    bool IsSprinting() const;
    void AdjustCameraDistance(float Axis);
    void UpdateCameraDistance(float DeltaSeconds);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera",meta=(ClampMin="50")) float MinimumCameraDistance=200.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera",meta=(ClampMin="50")) float MaximumCameraDistance=650.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera",meta=(ClampMin="1")) float CameraZoomStep=50.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement",meta=(ClampMin="1")) float SprintSpeedMultiplier=1.5f;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool InitializeState(FName ID) override;
    virtual void RestoreFromWorldState_Implementation(const FICWorldState& Candidate) override;
    virtual void HandleZeroHP() override;
    virtual void Jump() override;
    virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode,uint8 PreviousCustomMode=0) override;
    UFUNCTION(BlueprintCallable, Category="IC|Weapons") bool RequestCycleWeapon();
    UFUNCTION(BlueprintPure, Category="IC|Weapons") bool IsSwitchingWeapon() const;
    void CancelWeaponTransition();
    bool NotifyWeaponVisibility(int32 MontageInstanceID, bool bDraw);
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UICInventoryComponent> Inventory;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UICCraftingComponent> Crafting;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="IC|Life") TObjectPtr<UICPlayerLifeComponent> LifeActions;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="IC|Quest Guide") TObjectPtr<UICQuestGuideComponent> QuestGuide;
    /** Handles paid Warp Stone travel for this player and their active party. */
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="IC|Warp") TObjectPtr<UICPartyWarpComponent> PartyWarp;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance") TArray<FICPlayerAppearancePreset> AppearancePresets;
    UPROPERTY(BlueprintReadOnly,Category="IC|Appearance") int32 CurrentPreset=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement") float CameraSensitivity=1.0f;
    UFUNCTION(BlueprintCallable) void ApplyPreset(int32 Preset);
private:
    float DesiredCameraDistance=-1.f;
    bool bUsingProfileJumpPhysics=false;
    float SavedJumpZVelocity=0.f;
    float SavedJumpGravityScale=0.f;
    void RestoreJumpPhysics();
    enum class EWeaponTransition : uint8 { Idle, Sheathing, Drawing };
    EWeaponTransition WeaponTransition = EWeaponTransition::Idle;
    FGuid PendingWeaponID, TransitionWeaponID;
    uint32 WeaponTransitionSerial = 0;
    int32 WeaponMontageInstanceID = INDEX_NONE;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> WeaponTransitionMontage;
    void StartWeaponTransition(bool bDraw);
    void FinishWeaponTransitionPhase(bool bInterrupted);
    bool ApplyWeaponVisibility(bool bDraw);
    UFUNCTION() void WeaponInventoryChanged();
};
