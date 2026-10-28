#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICInventoryMenuWidget.generated.h"

class UWidgetTree;
class UTexture2D;
class UICWorldStateSubsystem;
class UICInventoryComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICInventoryCellPicked,int32,Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICItemConfirmation,bool,bAccept);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICInventoryAction);

UCLASS(Blueprintable)
class ISEKAICLASS_API UICInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) int32 CellIndex=0;
    UPROPERTY(BlueprintAssignable) FICInventoryCellPicked OnPicked;
    void Present(int32 Index,FText Name,UTexture2D* Icon,int32 Count,bool bSelected);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void Pick();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICItemConfirmWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) int32 Choice=1;
    UPROPERTY(BlueprintAssignable) FICItemConfirmation OnResolved;
    void Present(FText Message);
    bool HandleKey(FKey Key,bool bRepeat);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    void RefreshChoice();
    UFUNCTION() void Accept();
    UFUNCTION() void Cancel();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICInventoryItemPopupWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) int32 ActionIndex=0;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnEquip;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnUse;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnQuick;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnTrash;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnBack;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnLock;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnSplit;
    void Present(FText Name,FText Description,UTexture2D* Icon,EPlayerItemKind Kind,bool CanDiscard,bool CanBind,bool CanUse=true);
    void SetMessage(FText Message);
    void ConfigureStack(bool Locked,int32 Count,bool AllowLock,bool AllowSplit);
    bool HandleKey(FKey Key,bool Repeat);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    TArray<FName> Buttons;
    void RefreshSelection();
    UFUNCTION() void Equip();
    UFUNCTION() void Use();
    UFUNCTION() void Quick();
    UFUNCTION() void Trash();
    UFUNCTION() void Back();
    UFUNCTION() void Lock();
    UFUNCTION() void Split();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICInventoryMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TArray<FName> Members;
    UPROPERTY(BlueprintReadOnly) FName SelectedCharacter;
    UPROPERTY(BlueprintReadOnly) int32 SelectedMember=0;
    UPROPERTY(BlueprintReadOnly) EPlayerItemTab ItemTab=EPlayerItemTab::Normal;
    UPROPERTY(BlueprintReadOnly) int32 SelectedItem=0;
    /** 0 roster, 1 weapons, 2 item tabs, 3 bag, 4 item popup, 5 quick slots. */
    UPROPERTY(BlueprintReadOnly) int32 NavigationArea=0;
    UPROPERTY(BlueprintReadOnly) int32 EquipmentIndex=0;
    UPROPERTY(BlueprintReadOnly) int32 ActionIndex=0;
    UPROPERTY(BlueprintReadOnly) int32 QuickIndex=0;
    UPROPERTY(BlueprintReadOnly) bool bPickingQuickSlot=false;
    UPROPERTY(BlueprintReadOnly) bool bPickingUseTarget=false;
    UPROPERTY(BlueprintReadOnly) bool bConfirming=false;
    UPROPERTY(BlueprintReadOnly) bool bDetailOpen=false;
    UPROPERTY(BlueprintAssignable) FICInventoryAction OnNavigationChanged;
    UFUNCTION(BlueprintCallable) void RefreshInventory();
    UFUNCTION(BlueprintCallable) bool SelectMember(int32 Index);
    UFUNCTION(BlueprintCallable) void SelectItemTab(EPlayerItemTab Tab);
    UFUNCTION(BlueprintCallable) bool SelectItem(int32 Index);
    UFUNCTION(BlueprintCallable) bool OpenItem(int32 Index);
    UFUNCTION(BlueprintCallable) void CloseItem();
    UFUNCTION(BlueprintCallable) bool BeginQuickSlot();
    UFUNCTION(BlueprintCallable) bool ChooseQuickSlot(int32 Index);
    UFUNCTION(BlueprintCallable) bool BeginTrash();
    UFUNCTION(BlueprintCallable) bool EquipSelected();
    UFUNCTION(BlueprintCallable) bool BeginUseSelected();
    UFUNCTION(BlueprintCallable) bool ChooseUseTarget(int32 Index);
    UFUNCTION(BlueprintCallable) void CancelUseTarget();
    UFUNCTION(BlueprintCallable) void SortItems();
    UFUNCTION(BlueprintCallable) void CycleFilter();
    UFUNCTION(BlueprintCallable) void LockSelected();
    UFUNCTION(BlueprintCallable) void SplitSelected();
    bool IsSearchFocused() const;
    UFUNCTION(BlueprintCallable) void ResolveConfirmation(bool bAccept);
    bool HandleMenuKey(FKey Key,bool bRepeat=false);
    bool IsBusy() const {return bDetailOpen||bPickingQuickSlot||bConfirming||bPickingUseTarget;}
    FText NavigationHint() const;
    void Activate();
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> BoundState;
    TArray<int32> VisibleBagIndices;
    TArray<int32> EquipmentCells;
    bool bRefreshing=false;
    bool bTrashPending=false;
    FString SearchText;
    int32 FilterKind=0;
    UFUNCTION() void SearchChanged(const FText& Text);
    int32 PendingBagIndex=INDEX_NONE;
    FPlayerItemInstance PendingItem;
    FName PendingPreviousQuick;
    FGuid DetailInstance;
    UICInventoryComponent* Inventory() const;
    FText ItemName(FName ID) const;
    FPlayerItemInstance CurrentItem() const;
    bool CanQuickSlot() const;
    bool CanTrash() const;
    bool PendingStillValid() const;
    void RefreshEquipment();
    void RefreshItems();
    void RefreshPresentation();
    void CancelPending();
    void RefreshDetail();
    void NotifyNavigation();
    UFUNCTION() void CharacterChanged(FName ID);
    UFUNCTION() void PortraitPicked(int32 Index);
    UFUNCTION() void ItemPicked(int32 Index);
    UFUNCTION() void EquipmentPicked(int32 Index);
    UFUNCTION() void QuickPicked(int32 Index);
    UFUNCTION() void NormalTab();
    UFUNCTION() void QuestTab();
    UFUNCTION() void QuickAction();
    UFUNCTION() void UseAction();
    UFUNCTION() void UseTargetPicked(int32 Index);
    UFUNCTION() void TrashAction();
    UFUNCTION() void EquipAction();
    UFUNCTION() void CancelAction();
};
