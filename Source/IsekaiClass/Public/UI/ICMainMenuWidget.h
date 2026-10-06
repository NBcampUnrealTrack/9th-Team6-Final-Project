#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICMainMenuWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UWorld;
class UICSaveLoadWidget;

USTRUCT(BlueprintType)
struct FICMainMenuImages
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly, Category="Images") TSoftObjectPtr<UTexture2D> Normal;
    UPROPERTY(EditDefaultsOnly, Category="Images") TSoftObjectPtr<UTexture2D> Hover;
    UPROPERTY(EditDefaultsOnly, Category="Images") TSoftObjectPtr<UTexture2D> Pressed;
    UPROPERTY(EditDefaultsOnly, Category="Images") TSoftObjectPtr<UTexture2D> Disabled;
};

UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UICMainMenuWidget(const FObjectInitializer& ObjectInitializer);
    // Existing controller calls this name; prefer Continue when it is available.
    UFUNCTION(BlueprintCallable, Category="Main Menu") void FocusStartButton();
    UFUNCTION(BlueprintCallable, Category="Main Menu") void RefreshContinueAvailability();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu") TSoftObjectPtr<UWorld> StartMap;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu") TSubclassOf<UICSaveLoadWidget> SaveLoadWidgetClass;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Main Menu") TObjectPtr<UButton> BTN_Continue;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Main Menu") TObjectPtr<UButton> BTN_Start;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Main Menu") TObjectPtr<UButton> BTN_Load;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Main Menu") TObjectPtr<UButton> BTN_Settings;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Main Menu") TObjectPtr<UButton> BTN_Quit;

    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> IMG_Continue;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> IMG_Start;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> IMG_Load;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> IMG_Quit;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> IMG_MainFogA;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> IMG_MainFogB;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> IMG_MainGlow;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_Main_Notice;

    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images") FICMainMenuImages ContinueImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images") FICMainMenuImages StartImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images") FICMainMenuImages LoadImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Button Images") FICMainMenuImages QuitImages;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation") bool bAnimateBackground = true;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation", meta=(ClampMin="1")) float FogPeriodSeconds = 24.f;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation", meta=(ClampMin="1")) float GlowPeriodSeconds = 8.f;
    UPROPERTY(EditDefaultsOnly, Category="Main Menu|Animation", meta=(ClampMin="0", ClampMax="1")) float ExitFadeSeconds = 0.25f;

    // Kept so the archived legacy WBP remains compilable. New menu does not call these.
    UFUNCTION(BlueprintImplementableEvent, Category="Main Menu|Legacy") void OnLoadRequested();
    UFUNCTION(BlueprintImplementableEvent, Category="Main Menu|Legacy") void OnSettingsRequested();
    UFUNCTION(BlueprintImplementableEvent, Category="Main Menu|Legacy") void OnStartMapNotConfigured();
private:
    enum class EExitAction : uint8 { None, NewGame, Continue, Quit };
    EExitAction PendingAction = EExitAction::None;
    float FadeElapsed = 0.f;
    double AnimationSeconds = 0.0;
    FVector2D FogBaseA = FVector2D::ZeroVector;
    FVector2D FogBaseB = FVector2D::ZeroVector;
    bool bContinueAvailable = false;
    bool bContinueBusy = false;
    UPROPERTY() TObjectPtr<UICSaveLoadWidget> SaveLoadScreen;
    UPROPERTY() TArray<TObjectPtr<UTexture2D>> ResolvedImages;
    TArray<int32> LastImageStates;
    void ResolveButtonImages();
    void UpdateButtonImages();
    void UpdateAmbientImages();
    void SetNotice(const FText& Text);
    bool HasValidStartMap() const;
    void BeginExit(EExitAction Action);
    void CompleteExit();
    UFUNCTION() void HandleContinueClicked();
    UFUNCTION() void HandleStartClicked();
    UFUNCTION() void HandleLoadClicked();
    UFUNCTION() void HandleSaveLoadClosed();
    UFUNCTION() void HandleSettingsClicked();
    UFUNCTION() void HandleQuitClicked();
    UFUNCTION() void HandleSaveFinished(int32 SlotIndex, bool bSuccess, const FString& Message);
};
