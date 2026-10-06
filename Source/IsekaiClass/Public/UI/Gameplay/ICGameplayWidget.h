#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICGameplayWidget.generated.h"

class AICPlayerController;

// Blueprint owns the widget tree, HUD and menu layouts.
UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICGameplayWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="IC|UI") AICPlayerController* GetICPlayerController() const;
    UFUNCTION(BlueprintCallable, Category="IC|UI") void RefreshCurrentScreen();
    UFUNCTION(BlueprintImplementableEvent, Category="IC|UI") void OnRefreshGameplayUI();
};
