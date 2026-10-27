#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICCharacterStatusWidget.generated.h"

class UTexture2D;
struct FICCharacterState;

// Reusable player/companion presentation, supplied with the owning character's state.
UCLASS(Blueprintable)
class ISEKAICLASS_API UICCharacterStatusWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Status") FName CharacterID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status") TObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status", meta=(ClampMin="0", ClampMax="1")) float HPPercent = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status", meta=(ClampMin="0", ClampMax="1")) float MPPercent = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status", meta=(ClampMin="0", ClampMax="1")) float SPPercent = 1.f;
    UFUNCTION(BlueprintCallable, Category="Status") void RefreshPresentation();
    UFUNCTION(BlueprintCallable, Category="Status") void UpdateSkillCooldown(bool bHasSkill, float Progress, double RemainingSeconds);
    void UpdateVitals(const FICCharacterState& State);
protected:
    virtual void NativePreConstruct() override;
private:
    void EnsureSkillCooldownOverlay();
};
