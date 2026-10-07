#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "PlayerPrototypeWidget.generated.h"
class APlayerPrototypeController;
class UVerticalBox;
class APlayerInventoryPreview;
class UICCharacterCreateWidget;

UCLASS()
class ISEKAICLASS_API UPlayerPrototypeButton : public UButton
{
    GENERATED_BODY()
public:
    UPlayerPrototypeButton() { InitIsFocusable(false); }
    UPROPERTY() TObjectPtr<APlayerPrototypeController> Controller;
    FString Command;
    int32 Index=0;
    UFUNCTION() void Invoke();
};

UCLASS()
class ISEKAICLASS_API UPlayerPrototypeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    void RefreshCurrentScreen();
    UFUNCTION() void NameChanged(const FText& Text);
private:
    UPROPERTY() TObjectPtr<UICCharacterCreateWidget> CharacterCreateScreen;
    UPROPERTY() TObjectPtr<APlayerInventoryPreview> Preview;
    float PreviewRefresh=0.f;
    void Build();
};
