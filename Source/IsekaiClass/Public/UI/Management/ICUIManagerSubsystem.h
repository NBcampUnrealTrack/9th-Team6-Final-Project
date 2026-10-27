#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "ICUIManagerSubsystem.generated.h"
class UUserWidget;
class APlayerController;
class UICUIRootWidget;
UCLASS()
class ISEKAICLASS_API UICUIManagerSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override
    {
        return true;
    }
    virtual UWorld* GetTickableGameObjectWorld() const override
    {
        return GetWorld();
    }
    virtual TStatId GetStatId() const override;
    UFUNCTION(BlueprintCallable, Category = "IC|UI")
    bool OpenScreen(UUserWidget* Widget, bool PauseWorld, int32 ZOrder = 10);
    UFUNCTION(BlueprintCallable, Category = "IC|UI") void CloseScreen(UUserWidget* Widget);
    UFUNCTION(BlueprintCallable, Category = "IC|UI")
    void SetMenuContext(APlayerController* Controller, UUserWidget* Focus, bool MenuOpen);
    UFUNCTION(BlueprintCallable, Category = "IC|UI") void CloseAllScreens();
    UFUNCTION(BlueprintCallable, Category = "IC|UI") void SetHUD(UUserWidget* Widget);
    void AddHUDWidget(UUserWidget* Widget);

private:
    UPROPERTY(Transient) TObjectPtr<UICUIRootWidget> Root;
    bool EnsureRoot();
    struct FScreen
    {
        TWeakObjectPtr<UUserWidget> Widget;
        bool Pause = false;
        int32 Z = 0;
    };
    TArray<FScreen> Screens;
    TWeakObjectPtr<APlayerController> MenuController;
    TWeakObjectPtr<UUserWidget> MenuFocus;
    bool bMenuOpen = false;
    bool bKeyboardNavigationRegistered = false;
    void RefreshContext();
};
