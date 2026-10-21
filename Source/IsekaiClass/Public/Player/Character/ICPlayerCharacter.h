#pragma once
#include "CoreMinimal.h"
#include "Character/Base/ICCharacterBase.h"
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
class ISEKAICLASS_API AICPlayerCharacter : public AICCharacterBase
{
    GENERATED_BODY()
public:
    AICPlayerCharacter();
    virtual void HandleZeroHP() override;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UICInventoryComponent> Inventory;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UICCraftingComponent> Crafting;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IC|Appearance") TArray<FICPlayerAppearancePreset> AppearancePresets;
    UPROPERTY(BlueprintReadOnly,Category="IC|Appearance") int32 CurrentPreset=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement") float CameraSensitivity=1.0f;
    UFUNCTION(BlueprintCallable) void ApplyPreset(int32 Preset);
};
