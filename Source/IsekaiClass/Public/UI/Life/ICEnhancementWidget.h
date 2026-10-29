#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "UI/Framework/ICActionButtonWidget.h"
#include "Inventory/Enhancement/ICEnhancementTypes.h"
#include "ICEnhancementWidget.generated.h"
class UICEnhancementWidget;
UCLASS()
class ISEKAICLASS_API UICEnhancementChoiceButton : public UICActionButtonWidget
{
    GENERATED_BODY()
public:
    FGuid WeaponID;FName OreID;TWeakObjectPtr<UICEnhancementWidget> Screen;
    UFUNCTION() void Choose();
};
UCLASS(Blueprintable)
class ISEKAICLASS_API UICEnhancementWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static void BuildLayout(UWidgetTree* Tree);
    UFUNCTION(BlueprintCallable) void Activate();
    UFUNCTION(BlueprintCallable) void SelectWeapon(FGuid ID);
    UFUNCTION(BlueprintCallable) void SelectOre(FName ID);
    UFUNCTION(BlueprintCallable) void Attempt();
    UFUNCTION(BlueprintCallable) void Cancel();
    bool HandleKey(FKey Key);
    UPROPERTY(BlueprintReadOnly) FGuid SelectedWeapon;
    UPROPERTY(BlueprintReadOnly) FName SelectedOre;
    UPROPERTY(BlueprintReadOnly) bool bDetails=false;
protected:
    virtual void NativeConstruct() override;
private:
    TArray<FICEnhancementTarget> Weapons;TArray<FName> Ores;int32 WeaponIndex=0,OreIndex=0;
    void RefreshWeapons();void RefreshDetails();
};
