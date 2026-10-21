#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICGameplayWidget.generated.h"

class AICPlayerController;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICGameplayWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category = "IC|UI") AICPlayerController* GetICPlayerController() const;
    UFUNCTION(BlueprintCallable, Category = "IC|UI") void RefreshCurrentScreen();
    UFUNCTION(BlueprintImplementableEvent, Category = "IC|UI") void OnRefreshGameplayUI();

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    UFUNCTION() void Resume();
    UFUNCTION() void Save();
    UFUNCTION() void Load();
    UFUNCTION() void Settings();
    UFUNCTION() void Quit();
};
