#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICCompanionGroupWidget.generated.h"

class UICWorldStateSubsystem;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICCompanionGroupWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Companions") void RefreshCompanions();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> BoundState;
    bool bRefreshing = false;
    UFUNCTION() void CharacterChanged(FName CharacterID);
};
