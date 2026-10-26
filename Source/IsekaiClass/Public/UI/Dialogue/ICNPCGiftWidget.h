#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICNPCGiftWidget.generated.h"

class AICNPCBase;
class UICInventorySlotWidget;
class UICWorldStateSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICGiftPickerClosed, const FText&, Message);

/** Inventory grid for choosing a gift and confirming before consumption. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICNPCGiftWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void OpenForNPC(AICNPCBase* InNPC);
    UFUNCTION(BlueprintCallable, Category="IC|Dialogue") void ClosePicker();
    UFUNCTION(BlueprintPure, Category="IC|Dialogue") FName GetSelectedItemID() const { return SelectedID; }
    UPROPERTY(BlueprintReadOnly, Category="IC|Dialogue") bool bConfirming=false;
    UPROPERTY(BlueprintAssignable, Category="IC|Dialogue") FICGiftPickerClosed OnClosed;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<AICNPCBase> NPC;
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> BoundState;
    UPROPERTY(Transient) TArray<TObjectPtr<UICInventorySlotWidget>> Cells;
    UPROPERTY(Transient) TSubclassOf<UICInventorySlotWidget> CellClass;
    TArray<FName> ItemIDs;
    TMap<FName,int32> Counts;
    FName SelectedID;
    FName PendingID,PendingNPC;
    int32 SelectedIndex=0;
    bool bBackSelected=false;
    bool bFocusPending=false,bSubmitting=false,bClosed=false;
    void Select(int32 Index,bool Back=false);
    void MoveSelection(int32 Horizontal,int32 Vertical);
    void RefreshPresentation();
    void Finish(const FText& Message);
    UFUNCTION() void RefreshItems();
    UFUNCTION() void ItemPicked(int32 Index);
    UFUNCTION() void ResolveGift(bool Accept);
};
