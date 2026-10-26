#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICGameMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICMenuActionRequested);

UCLASS(Blueprintable)
class ISEKAICLASS_API UICGameMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Menu") bool SelectTab(FName Tab);
    UFUNCTION(BlueprintPure, Category="Menu") FName GetSelectedTab() const { return SelectedTab; }
    UFUNCTION(BlueprintCallable, Category="Menu") bool HandleMenuKey(FKey Key,bool bRepeat=false);
    UPROPERTY(BlueprintReadOnly, Category="Menu") int32 NavigationArea = 1; // 0: tabs, 1: content, 2: story
    UPROPERTY(BlueprintReadOnly, Category="Menu") int32 SettingsActionIndex = 0;
    void SetSessionPresentation(const FString& Menu, const FText& Message);
    UPROPERTY(BlueprintAssignable, Category="Menu") FICMenuActionRequested OnResumeRequested;
    UPROPERTY(BlueprintAssignable, Category="Menu") FICMenuActionRequested OnBackRequested;
    UPROPERTY(BlueprintAssignable, Category="Menu") FICMenuActionRequested OnSaveRequested;
    UPROPERTY(BlueprintAssignable, Category="Menu") FICMenuActionRequested OnLoadRequested;
    UPROPERTY(BlueprintAssignable, Category="Menu") FICMenuActionRequested OnGameSettingsRequested;
    UPROPERTY(BlueprintAssignable, Category="Menu") FICMenuActionRequested OnQuitRequested;
    static const TArray<FName>& Tabs();
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UPROPERTY(Transient) FName SelectedTab = TEXT("Party");
    FString LastSessionMenu;
    void RefreshNavigation();
    UFUNCTION() void CodexNavigationChanged();
    UFUNCTION() void InventoryNavigationChanged();
    UFUNCTION() void Party();
    UFUNCTION() void Inventory();
    UFUNCTION() void Quest();
    UFUNCTION() void Codex();
    UFUNCTION() void Settings();
    UFUNCTION() void Resume();
    UFUNCTION() void Save();
    UFUNCTION() void Load();
    UFUNCTION() void GameSettings();
    UFUNCTION() void Quit();
};
