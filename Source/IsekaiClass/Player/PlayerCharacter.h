#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"
class UCameraComponent;
class UPlayerInventory;
class UPlayerCrafting;
class USpringArmComponent;
class UStaticMeshComponent;
UCLASS()
class ISEKAICLASS_API APlayerPrototypeCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    APlayerPrototypeCharacter();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPlayerInventory> Inventory;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPlayerCrafting> Crafting;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PlaceholderBody;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement") float CameraSensitivity=1.0f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movement") float FinalMoveSpeed=500.f;
    UFUNCTION(BlueprintCallable) void ApplyPreset(int32 Preset);
};
