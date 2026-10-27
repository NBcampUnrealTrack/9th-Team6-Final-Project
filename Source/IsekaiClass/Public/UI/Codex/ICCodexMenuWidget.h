#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICCodexMenuWidget.generated.h"

class UWidgetTree;
class UTexture2D;
class UICCodexSubsystem;
class UICWorldStateSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICCodexCellPicked,int32,Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICCodexAction);

UCLASS(Blueprintable)
class ISEKAICLASS_API UICCodexCellWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) int32 CellIndex=0;
    UPROPERTY(BlueprintReadOnly) bool bUnknown=false;
    UPROPERTY(BlueprintAssignable) FICCodexCellPicked OnPicked;
    void Present(int32 Index,FText Name,FText Progress,UTexture2D* Icon,bool Unknown,bool Selected,float Width=88,float Height=96);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void Pick();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICCodexPopupWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) int32 Choice=0;
    UPROPERTY(BlueprintAssignable) FICCodexAction OnPrimary;
    UPROPERTY(BlueprintAssignable) FICCodexAction OnBack;
    void Present(FText Title,FText Description,UTexture2D* Icon,FText Primary,FText Back,bool HasPrimary=true,bool HasBack=true,int32 DefaultChoice=0);
    bool HandleKey(FKey Key,bool Repeat=false);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    void RefreshChoice();
    UFUNCTION() void PrimaryAction();
    UFUNCTION() void BackAction();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICCodexTargetsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TArray<FName> Members;
    UPROPERTY(BlueprintReadOnly) int32 SelectedIndex=0;
    UPROPERTY(BlueprintAssignable) FICCodexCellPicked OnPicked;
    UPROPERTY(BlueprintAssignable) FICCodexAction OnBack;
    void Present(UICCodexSubsystem* Codex);
    bool HandleKey(FKey Key,bool Repeat=false);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void Pick(int32 Index);
    UFUNCTION() void Back();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICCodexMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    /** -1 home, 0 skills, 1 pets, 2 products, 3 recipes. */
    UPROPERTY(BlueprintReadOnly) int32 Category=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 CategoryFocus=0;
    UPROPERTY(BlueprintReadOnly) int32 SubTab=0;
    UPROPERTY(BlueprintReadOnly) int32 SelectedCell=0;
    UPROPERTY(BlueprintReadOnly) int32 EquippedIndex=0;
    /** 0 right categories, 1 equipped skills/subtabs, 2 grid. */
    UPROPERTY(BlueprintReadOnly) int32 NavigationArea=0;
    /** 0 none, 1 detail, 2 character selection, 3 confirmation; notice overlays these. */
    UPROPERTY(BlueprintReadOnly) int32 Popup=0;
    UPROPERTY(BlueprintReadOnly) bool bNoticeOpen=false;
    UPROPERTY(BlueprintReadOnly) TArray<FName> Cells;
    UPROPERTY(BlueprintAssignable) FICCodexAction OnNavigationChanged;
    UFUNCTION(BlueprintCallable) void Activate();
    UFUNCTION(BlueprintCallable) void RefreshCodex();
    UFUNCTION(BlueprintCallable) void SelectCategory(int32 Index);
    UFUNCTION(BlueprintCallable) void SelectSubTab(int32 Index);
    UFUNCTION(BlueprintCallable) bool OpenCell(int32 Index);
    UFUNCTION(BlueprintCallable) void ClosePopup();
    UFUNCTION(BlueprintPure) bool IsModalOpen() const{return Popup!=0||bNoticeOpen;}
    bool HandleMenuKey(FKey Key,bool Repeat=false);
    FText NavigationHint() const;
    static bool BuildLayout(UWidgetTree* Tree);
private:
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> BoundState;
    bool bRefreshing=false;
    int32 GridColumns=0;
    FName DetailID;
    int32 DetailPet=INDEX_NONE;
    FName PendingCharacter;
    int32 PendingPreviousPet=INDEX_NONE;
    int32 PendingPet=INDEX_NONE;
    UICCodexSubsystem* Codex() const;
    int32 Columns() const;
    void RefreshGrid();
    void RefreshDetail();
    void NotifyNavigation();
    void ShowNotice(FText Text);
    void ResolvePet(bool Accept);
    UFUNCTION() void CharacterChanged(FName ID);
    UFUNCTION() void CellPicked(int32 Index);
    UFUNCTION() void Category0();
    UFUNCTION() void Category1();
    UFUNCTION() void Category2();
    UFUNCTION() void Category3();
    UFUNCTION() void Tab0();
    UFUNCTION() void Tab1();
    UFUNCTION() void Tab2();
    UFUNCTION() void Tab3();
    UFUNCTION() void Tab4();
    UFUNCTION() void DetailAction();
    UFUNCTION() void MemberPicked(int32 Index);
    UFUNCTION() void ConfirmPet();
    UFUNCTION() void CancelPet();
    UFUNCTION() void DismissNotice();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
};
