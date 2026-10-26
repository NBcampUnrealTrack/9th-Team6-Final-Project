#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "ICMovementCooldownWidget.generated.h"

class SICRadialCooldown;

// Reusable clockwise ring. Zero is empty; one is a complete, ready ring.
UCLASS()
class ISEKAICLASS_API UICRadialCooldownBar : public UWidget
{
    GENERATED_BODY()
public:
    static constexpr float Diameter = 112.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cooldown", meta=(ClampMin="0", ClampMax="1")) float Progress = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooldown") FLinearColor FillColor = FLinearColor(1.f,.77f,.3f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooldown", meta=(ClampMin="1")) float Thickness = 14.f;
    UFUNCTION(BlueprintCallable, Category="Cooldown") void SetProgress(float Value);
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;
    static FVector2D ClockwisePoint(float Fraction);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<SICRadialCooldown> Ring;
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICMovementCooldownWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Cooldown") float Progress = 1.f;
    UPROPERTY(BlueprintReadOnly, Category="Cooldown") double RemainingSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Category="Cooldown") bool bReady = true;
    UFUNCTION(BlueprintCallable, Category="Cooldown") void RefreshCooldown();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
};
