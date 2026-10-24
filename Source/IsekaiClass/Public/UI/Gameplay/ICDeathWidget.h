#pragma once
#include "CoreMinimal.h"
#include "UI/Framework/ICScreenWidget.h"
#include "ICDeathWidget.generated.h"

/** The only root menu available until a living save is loaded or the session ends. */
UCLASS(Blueprintable)
class ISEKAICLASS_API UICDeathWidget : public UICScreenWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Death") void LoadGame();
    UFUNCTION(BlueprintCallable, Category="IC|Death") void MainMenu();
    UFUNCTION(BlueprintCallable, Category="IC|Death") void QuitGame();
protected:
    virtual void NativeOnInitialized() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual void GoBack() override;
};
