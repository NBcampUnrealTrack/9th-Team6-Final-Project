#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Commands/InputChord.h"
#include "Types/SlateEnums.h"
#include "Settings/ICDisplaySettings.h"
#include "ICSettingsWidget.generated.h"

class UICSettingsWidget;
class UWidget;
class UInputKeySelector;
UCLASS()
class ISEKAICLASS_API UICSettingBinding : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UICSettingsWidget> Owner;
    UPROPERTY() TObjectPtr<UWidget> Control;
    FName ID;FString KeyID;FKey DefaultKey;int32 Tab=0;
    FText Label;
    UFUNCTION() void SliderChanged(float Value);
    UFUNCTION() void CheckChanged(bool Value);
    UFUNCTION() void ChoiceChanged(FString Value,ESelectInfo::Type Type);
    UFUNCTION() void KeyChanged(FInputChord Value);
    UFUNCTION() void Clicked();
};

/** C++ behavior and editable WBP controls share the same settings schema. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICSettingsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void SelectTab(int32 Index);
    UFUNCTION(BlueprintCallable) void ApplyOptions();
    UFUNCTION(BlueprintCallable) void ResetOptions();
    UFUNCTION(BlueprintCallable) void KeepDisplay();
    UFUNCTION(BlueprintCallable) void RevertDisplay();
    UFUNCTION(BlueprintPure) bool IsAwaitingDisplayConfirmation() const {return bPreview;}
    bool HandleNavigationKey(FKey Key,bool Repeat=false);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    friend class UICSettingBinding;
    UPROPERTY() TArray<TObjectPtr<UICSettingBinding>> Bindings;
    UPROPERTY() TArray<TObjectPtr<UICSettingBinding>> KeyBindings;
    UPROPERTY(Transient) TObjectPtr<UInputKeySelector> PendingKeySelector;
    TMap<FName,double> Draft,Before;
    TMap<FString,FString> DraftKeys,BeforeKeys;
    TArray<FICMonitor> Displays,OldDisplays;
    TArray<FIntPoint> Resolutions;
    TArray<int32> RefreshRates;
    FString BeforeMonitor;
    int32 MonitorIndex=0,BeforeHz=0,WindowMode=1,BeforeWindowMode=1;
    int32 NavigationArea=1,OptionIndex=0,FooterIndex=1;
    FIntPoint Resolution,BeforeResolution;
    int32 RefreshHz=0,ActiveTab=0;
    bool bRefreshing=false,bPreview=false;
    double ConfirmDeadline=0;
    UICSettingBinding* Bind(UWidget* Control,FName ID);
    void ReadCurrent();
    void BuildKeyBindings();
    void RefreshControls();
    void RefreshDisplays();
    void ChangeOption(FName ID,double Value);
    void ChangeChoice(FName ID,const FString& Value);
    void ChangeKey(UICSettingBinding* Binding,FInputChord Value);
    void WriteDraft();
    void SaveApplied();
    void Status(const FText& Text);
    UFUNCTION() void Close();
    TArray<UWidget*> NavigationOptions() const;
    void RefreshNavigation();
    void AdjustSelected(int32 Direction);
};
