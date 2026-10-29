#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveLoadWidget.generated.h"
class UButton;
class UTextBlock;
class UVerticalBox;
class UWorld;
class UICSaveSubsystem;
class UICSaveSlotWidget;
class UICLoadConfirmWidget;
class UImage;
class UTexture2D;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICSaveLoadClosed);
UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICSaveLoadWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICSaveLoadClosed OnClosed;

    UFUNCTION(BlueprintCallable, Category="IC|Save UI")
    void Configure(bool bInAllowSave, TSoftObjectPtr<UWorld> InGameplayMap);
    UFUNCTION(BlueprintCallable, Category="IC|Save UI") void RefreshSlots();
protected:
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent) override;
    virtual FReply NativeOnAnalogValueChanged(const FGeometry& Geometry,const FAnalogInputEvent& Event) override;
    UPROPERTY(BlueprintReadOnly,meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_SaveLoad_Delete;
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UVerticalBox> IC_SaveLoad_SlotList;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_SaveLoad_Load;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_SaveLoad_Close;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_SaveLoad_Save;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_SaveLoad_Title;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_SaveLoad_Detail;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_SaveLoad_Notice;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveLoad_Party0;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveLoad_Party1;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveLoad_Party2;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveLoad_Party3;
    UPROPERTY(EditDefaultsOnly, Category="IC|Save UI") TSubclassOf<UICLoadConfirmWidget> ConfirmWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="IC|Portraits") TMap<FName, TObjectPtr<UTexture2D>> PortraitTextures;
    UPROPERTY(EditDefaultsOnly, Category="IC|Save UI") TSubclassOf<UICSaveSlotWidget> SlotWidgetClass;
private:
    UPROPERTY() TArray<TObjectPtr<UICSaveSlotWidget>> SlotRows;
    UPROPERTY() TArray<FICSaveSlotInfo> DisplaySlots;
    UPROPERTY() TObjectPtr<UICLoadConfirmWidget> ConfirmScreen;
    UFUNCTION() void HandleLoadConfirmed();
    UFUNCTION() void HandleLoadCancelled();
    TSoftObjectPtr<UWorld> GameplayMap;
    bool bAllowSave = false;
    bool bClosing = false;
    double NextStickMove=0;
    UFUNCTION() void HandleDeleteClicked();
    UFUNCTION() void HandleMenuChanged(const FString& Menu);
    UFUNCTION() void HandleNoticeChanged(const FString& Notice);
    UFUNCTION() void HandleSaveStarted(int32 SlotIndex);
    void CloseConfirmScreen();
    void CloseView();
    UICSaveSubsystem* GetSaves() const;
    void UpdateSelection();
    void SetNotice(const FText& Text);
    UFUNCTION() void HandleSlotPicked(int32 SlotIndex);
    UFUNCTION() void HandleLoadClicked();
    UFUNCTION() void HandleSaveClicked();
    UFUNCTION() void HandleCloseClicked();
    UFUNCTION() void HandleSaveFinished(int32 SlotIndex, bool bSuccess, const FString& Message);
};
