#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ICCraftingStationWidget.generated.h"
class AICCraftingStation;
class UWidgetTree;
class UICCraftingStationWidget;

UCLASS()
class ISEKAICLASS_API UICRecipeChoiceButton : public UButton
{
    GENERATED_BODY()
public:
    FName RecipeID;
    TWeakObjectPtr<UICCraftingStationWidget> StationUI;
    UFUNCTION() void Choose();
};

/** Named widgets are authored into WBP by BuildLayout; C++ is also a fallback. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICCraftingStationWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UICCraftingStationWidget(const FObjectInitializer& Initializer);
    static void BuildLayout(UWidgetTree* Tree);
    UFUNCTION(BlueprintCallable) void OpenStation(AICCraftingStation* Station);
    UFUNCTION(BlueprintCallable) void SelectRecipe(FName RecipeID);
    UFUNCTION(BlueprintCallable) void ShowRecipeList();
    UFUNCTION(BlueprintCallable) void MakeRecipe();
    UFUNCTION(BlueprintCallable) void CloseStation();
    UFUNCTION(BlueprintCallable) void CraftTab();
    UFUNCTION(BlueprintCallable) void EnhanceTab();
    UFUNCTION(BlueprintPure) bool IsOpen() const {return bOpen;}
    bool HandleStationKey(FKey Key);
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly) FName SelectedRecipe;
    UPROPERTY(BlueprintReadOnly) FGuid ActiveJob;
    UPROPERTY(BlueprintReadOnly) bool bDetails=false;
protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<AICCraftingStation> Workstation;
    TArray<FName> Recipes;
    bool bOpen=false;
    bool bEnhancing=false;
    float RefreshAt=0;
    void RefreshList();
    void RefreshDetails();
    void RefreshProgress();
    void SetPage(int32 Page);
    void Highlight(int32 Index);
};
