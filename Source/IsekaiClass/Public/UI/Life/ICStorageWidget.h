#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Types/ICPlayerItemTypes.h"
#include "ICStorageWidget.generated.h"
class UWidgetTree;
class UICInventoryComponent;
class UICWorldStateSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICStorageQuantity,int32,Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICStorageAction);

UCLASS(Blueprintable)
class ISEKAICLASS_API UICStorageTransferPopup : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FICStorageQuantity OnQuantity;
    UPROPERTY(BlueprintAssignable) FICStorageAction OnMove;
    UPROPERTY(BlueprintAssignable) FICStorageAction OnBack;
    void Present(FText Name,FText Description,int32 Count,int32 Maximum,bool ToStorage,FText Notice);
    bool HandleKey(FKey Key,bool Repeat);
    void ResetSelection(){Action=0;}
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    int32 Action=0;
    void Highlight();
    UFUNCTION() void Decrease(){OnQuantity.Broadcast(-1);}
    UFUNCTION() void Increase(){OnQuantity.Broadcast(1);}
    UFUNCTION() void Move(){OnMove.Broadcast();}
    UFUNCTION() void Back(){OnBack.Broadcast();}
};

/** Independent chest screen: a fixed 40-slot chest and the player's bag. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICStorageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) bool OpenStorage(FName ID);
    UFUNCTION(BlueprintCallable) void CloseStorage();
    UFUNCTION(BlueprintCallable) void RefreshStorage();
    UFUNCTION(BlueprintCallable) bool OpenItem(bool FromStorage,int32 Index);
    UFUNCTION(BlueprintCallable) void CloseItem();
    UFUNCTION(BlueprintCallable) void AdjustQuantity(int32 Delta);
    UFUNCTION(BlueprintCallable) bool TransferSelected();
    UFUNCTION(BlueprintPure) bool IsOpen() const{return bOpen;}
    UPROPERTY(BlueprintReadOnly) FName ContainerID;
    UPROPERTY(BlueprintReadOnly) int32 Quantity=1;
    UPROPERTY(BlueprintReadOnly) int32 MaximumQuantity=0;
    UPROPERTY(BlueprintReadOnly) bool bPopupOpen=false;
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<UICInventoryComponent> Inventory;
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> State;
    bool bOpen=false,bRefreshing=false,bSubmitting=false,bStorageSide=true;
    int32 Selected=0,DetailSlot=INDEX_NONE;
    bool bDetailFromStorage=true;
    FGuid DetailInstance;
    FText PopupNotice;
    FPlayerItemInstance CurrentItem() const;
    void RefreshPopup();
    UFUNCTION() void CellPicked(int32 Cell);
    UFUNCTION() void MovePressed(){TransferSelected();}
};
