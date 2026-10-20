#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICSaveSlotWidget.generated.h"
class UButton;
class UTextBlock;
class UTexture2D;
class UImage;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICSlotPicked, int32, SlotIndex);
UCLASS(Blueprintable)
class ISEKAICLASS_API UICSaveSlotWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    int32 GetSlotIndex() const { IC_TRACE_FUNCTION();return SlotIndex; }
    UPROPERTY(BlueprintAssignable) FICSlotPicked OnSlotPicked;
    void Configure(int32 InSlot, const FText& InLabel);
    void SetSelected(bool bSelected);
    void ConfigureData(const FICSaveSlotInfo& InInfo, UTexture2D* InThumbnail, const TMap<FName, TObjectPtr<UTexture2D>>& Portraits);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_SaveSlot_Button;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_SaveSlot_Label;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveSlot_Thumbnail;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveSlot_Party0;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveSlot_Party1;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveSlot_Party2;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveSlot_Party3;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_SaveSlot_Party4;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images") TObjectPtr<UTexture2D> IC_Load_Slot_Normal;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images") TObjectPtr<UTexture2D> IC_Load_Slot_Hover;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images") TObjectPtr<UTexture2D> IC_Load_Slot_Pressed;
    UPROPERTY(EditDefaultsOnly, Category="IC|Slot Images") TObjectPtr<UTexture2D> IC_Load_Slot_Selected;
    UFUNCTION(BlueprintImplementableEvent) void OnSelectionChanged(bool bSelected);
private:
    int32 SlotIndex = INDEX_NONE;
    FText Label;
    bool bIsSelected = false;
    UPROPERTY() FICSaveSlotInfo Info;
    UPROPERTY() TObjectPtr<UTexture2D> Thumbnail;
    UPROPERTY() TMap<FName, TObjectPtr<UTexture2D>> PortraitTextures;
    void ApplyData();
    UFUNCTION() void HandleClicked();
};
