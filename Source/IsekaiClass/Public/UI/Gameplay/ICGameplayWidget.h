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
    UFUNCTION(BlueprintCallable, Category = "IC|UI") void RefreshSkillCooldown();
    UFUNCTION(BlueprintCallable, Category = "IC|UI") void RefreshTargetInfo();
    UFUNCTION(BlueprintPure,Category="IC|Guide") bool IsControlGuideVisible() const;
    UFUNCTION(BlueprintCallable,Category="IC|Guide") bool AdvanceControlGuide();
    UFUNCTION(BlueprintCallable,Category="IC|Guide") void RefreshControlGuide();
    UFUNCTION(BlueprintPure,Category="IC|Time") static FText FormatGameClock(double GameMinute);
    UFUNCTION(BlueprintImplementableEvent, Category = "IC|UI") void OnRefreshGameplayUI();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event) override;

private:
    int32 ControlGuidePage=INDEX_NONE;
    bool bControlGuideFinished=false;
    UFUNCTION() void Back();
    UFUNCTION() void Resume();
    UFUNCTION() void Save();
    UFUNCTION() void Load();
    UFUNCTION() void Settings();
    UFUNCTION() void GameSettings();
    UFUNCTION() void Quit();
};
