#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICPartyMenuWidget.generated.h"

class UDataTable;
class UTexture2D;
class UWidgetTree;
class UICWorldStateSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FICPartyPortraitPicked,int32,Index);

UCLASS(Blueprintable)
class ISEKAICLASS_API UICPartyPortraitWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Party") int32 MemberIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category="Party") FName CharacterID;
    UPROPERTY(BlueprintReadOnly, Category="Party") bool bSelected = false;
    UPROPERTY(BlueprintAssignable, Category="Party") FICPartyPortraitPicked OnPicked;
    void Present(int32 Index,FName ID,UTexture2D* Portrait,bool Selected);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void Pick();
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICPartyMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UICPartyMenuWidget(const FObjectInitializer& Initializer);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Party") TSoftObjectPtr<UDataTable> ProfileTable;
    UPROPERTY(BlueprintReadOnly, Category="Party") TArray<FName> Members;
    UPROPERTY(BlueprintReadOnly, Category="Party") int32 SelectedIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category="Party") FName SelectedCharacter;
    UFUNCTION(BlueprintCallable, Category="Party") void RefreshParty();
    UFUNCTION(BlueprintCallable, Category="Party") bool SelectMember(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Party") bool MoveSelection(int32 Direction);
    void ScrollStory(float Amount);
    static bool BuildLayout(UWidgetTree* Tree);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> BoundState;
    bool bRefreshing = false;
    UFUNCTION() void CharacterChanged(FName ID);
    UFUNCTION() void PortraitPicked(int32 Index);
    void RefreshDetails();
};
