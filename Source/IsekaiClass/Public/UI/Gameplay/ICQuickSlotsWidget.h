#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICQuickSlotsWidget.generated.h"

class UICWorldStateSubsystem;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuickSlotsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="QuickSlots") void RefreshQuickSlots();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> BoundState;
};
