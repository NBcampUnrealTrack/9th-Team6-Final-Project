#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICQuestMenuWidget.generated.h"

class UWidgetTree;
class UICWorldStateSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICQuestPicked,FName,QuestID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FICQuestMenuAction);

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuestMenuEntryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) FName QuestID;
    UPROPERTY(BlueprintReadOnly) bool bSelected=false;
    UPROPERTY(BlueprintAssignable) FICQuestPicked OnPicked;
    void Present(FName ID,FText Title,FText Summary,bool Selected);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
private:
    float Pulse=0;
    UFUNCTION() void Pick();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuestDetailWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) FName QuestID;
    UPROPERTY(BlueprintAssignable) FICQuestMenuAction OnStart;
    UPROPERTY(BlueprintAssignable) FICQuestMenuAction OnAbandon;
    UPROPERTY(BlueprintAssignable) FICQuestMenuAction OnBack;
    void Present(FName ID);
    void HighlightAction(int32 Index);
    void Scroll(float Delta);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void Start();
    UFUNCTION() void Abandon();
    UFUNCTION() void Back();
    UFUNCTION() void ToggleGuide();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuestMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TArray<FName> QuestIDs;
    UPROPERTY(BlueprintReadOnly) int32 SelectedIndex=0;
    UPROPERTY(BlueprintReadOnly) FName SelectedQuest;
    UPROPERTY(BlueprintReadOnly) bool bShowingDetails=false;
    UPROPERTY(BlueprintReadOnly) bool bReadingDetails=false;
    /** 0 abandon, 1 start, 2 guide toggle, 3 back. */
    UPROPERTY(BlueprintReadOnly) int32 ActionIndex=1;
    UFUNCTION(BlueprintCallable) void RefreshQuests();
    UFUNCTION(BlueprintCallable) bool SelectQuest(int32 Index);
    UFUNCTION(BlueprintCallable) bool OpenQuest(FName QuestID);
    UFUNCTION(BlueprintCallable) void BackToList();
    UFUNCTION(BlueprintCallable) bool StartSelectedQuest();
    UFUNCTION(BlueprintCallable) bool AbandonSelectedQuest();
    void Activate();
    bool HandleMenuKey(FKey Key,bool bRepeat=false);
    FText NavigationHint() const;
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> BoundState;
    bool bRefreshing=false;
    void RefreshSelection();
    UFUNCTION() void CharacterChanged(FName ID);
    UFUNCTION() void QuestPicked(FName ID);
    UFUNCTION() void StartAction();
    UFUNCTION() void AbandonAction();
};
