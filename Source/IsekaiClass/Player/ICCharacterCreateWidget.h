#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "ICCharacterCreateWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
class UButton;
class UTexture2D;
class UImage;

UCLASS(Abstract, Blueprintable)
class ISEKAICLASS_API UICCharacterCreateWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void RefreshState();
    UFUNCTION(BlueprintCallable, Category="Character Creation") void SelectPreset(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Character Creation") void SelectJob(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Character Creation") void CreateCharacter();
    UFUNCTION(BlueprintCallable, Category="Character Creation") void OpenLoadScreen();
    UFUNCTION(BlueprintCallable, Category="Character Creation") void OpenQuitScreen();
    UFUNCTION(BlueprintPure, Category="Character Creation") int32 GetSelectedPreset() const;
    UFUNCTION(BlueprintPure, Category="Character Creation") int32 GetSelectedJob() const;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UEditableTextBox> IC_Create_Name;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> IC_Create_Confirm;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_Create_Notice;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_Create_Warrior;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_Create_Archer;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_Create_Mage;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_Create_Healer;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Job Images") TObjectPtr<UTexture2D> IC_Create_Job_Selected;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> IC_Create_Weapon_Icon;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images") TObjectPtr<UTexture2D> IC_Create_Weapon_Warrior;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images") TObjectPtr<UTexture2D> IC_Create_Weapon_Archer;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images") TObjectPtr<UTexture2D> IC_Create_Weapon_Mage;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Weapon Images") TObjectPtr<UTexture2D> IC_Create_Weapon_Healer;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_Create_WeaponName;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> IC_Create_WeaponDescription;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UButton> IC_Create_Back;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Creation|Navigation") FName MainMenuLevelName = TEXT("L_IC_MainMenu");
    // Gender and other presentation can still be updated in Blueprint.
    UFUNCTION(BlueprintImplementableEvent, Category="Character Creation")
    void OnCreationStateChanged(int32 Preset, int32 Job, bool bCanEdit);
private:
    TArray<FButtonStyle> DefaultJobStyles;
    void UpdateJobButtons(int32 Job, bool bCanEdit);
    UFUNCTION() void HandleWarriorClicked();
    UFUNCTION() void HandleArcherClicked();
    UFUNCTION() void HandleMageClicked();
    UFUNCTION() void HandleHealerClicked();
    UFUNCTION() void HandleBackClicked();
    UFUNCTION() void HandleNameChanged(const FText& Text);
};
