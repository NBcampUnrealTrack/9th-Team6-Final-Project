#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "ICPlayerCharacter.generated.h"
class UCameraComponent;
class UICInventoryComponent;
class UICCraftingComponent;
class USpringArmComponent;
class USkeletalMesh;
USTRUCT(BlueprintType)
struct FICPlayerAppearancePreset
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance") TObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance") TSubclassOf<UAnimInstance> AnimClass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Appearance") FTransform MeshTransform=FTransform(FRotator(0,-90,0),FVector(0,0,-88));
};
UCLASS(Blueprintable)
class ISEKAICLASS_API AICPlayerCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AICPlayerCharacter();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UICInventoryComponent> Inventory;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UICCraftingComponent> Crafting;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    // Empty presets retain the Mesh and Anim Blueprint authored directly on the Blueprint.
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance") TArray<FICPlayerAppearancePreset> AppearancePresets;
    UPROPERTY(BlueprintReadOnly,Category="IC|Appearance") int32 CurrentPreset=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement") float CameraSensitivity=1.0f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement") float FinalMoveSpeed=500.f;
    // FinalMoveSpeed is the run speed; normal movement uses the walking multiplier.
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Movement",meta=(ClampMin="0",ClampMax="1")) float WalkSpeedMultiplier=0.5f;
    UFUNCTION(BlueprintCallable) void ApplyPreset(int32 Preset);
};
