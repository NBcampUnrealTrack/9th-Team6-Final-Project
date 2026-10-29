#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICCharacterStatusWidget.generated.h"

class UTexture2D;
class UBorder;
class UICDefinitionSubsystem;
struct FICWorldState;
struct FICCharacterState;

// Reusable player/companion presentation, supplied with the owning character's state.
UCLASS(Blueprintable)
class ISEKAICLASS_API UICCharacterStatusWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Status") FName CharacterID;
    UPROPERTY(BlueprintReadOnly, Category="Status") bool bDowned=false;
    UPROPERTY(BlueprintReadOnly, Category="Status") bool bHasBuff=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status") TObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status", meta=(ClampMin="0", ClampMax="1")) float HPPercent = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status", meta=(ClampMin="0", ClampMax="1")) float MPPercent = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status", meta=(ClampMin="0", ClampMax="1")) float SPPercent = 1.f;
    UFUNCTION(BlueprintCallable, Category="Status") void RefreshPresentation();
    static void BuildSkillCooldownOverlay(class UWidgetTree* Tree);
    UFUNCTION(BlueprintCallable, Category="Status") void UpdateSkillCooldown(bool bHasSkill, float Progress, double RemainingSeconds);
    void UpdateVitals(const FICCharacterState& State);
    void UpdateStatusEffects(const FICCharacterState& State, const FICWorldState& World, UICDefinitionSubsystem* Definitions, double Now);
protected:
    virtual void NativePreConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
private:
    friend class UICBlueprintMaintenanceLibrary;
    float StatusRefreshAt=0;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UUserWidget>> EffectWidgets;
};
