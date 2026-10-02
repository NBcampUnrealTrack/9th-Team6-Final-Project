#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICMainMenuWidget.generated.h"

class UButton;
class UWorld;

UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    void FocusStartButton();

protected:
    virtual void NativeOnInitialized() override;

    // Select the actual gameplay map in WBP_MainMenu > Class Defaults.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
    TSoftObjectPtr<UWorld> StartMap;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Main Menu")
    TObjectPtr<UButton> BTN_Start;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Main Menu")
    TObjectPtr<UButton> BTN_Load;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Main Menu")
    TObjectPtr<UButton> BTN_Settings;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Main Menu")
    TObjectPtr<UButton> BTN_Quit;

    // Implement these in the Widget Blueprint; no save/settings system is assumed.
    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
    void OnLoadRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
    void OnSettingsRequested();

    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
    void OnStartMapNotConfigured();

private:
    UFUNCTION() void HandleStartClicked();
    UFUNCTION() void HandleLoadClicked();
    UFUNCTION() void HandleSettingsClicked();
    UFUNCTION() void HandleQuitClicked();
};
