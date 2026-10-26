#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICGatherProgressWidget.generated.h"

UCLASS(Blueprintable)
class ISEKAICLASS_API UICGatherProgressWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Life") void SetGatherProgress(const FText& ResourceName, float Value);
    UPROPERTY(BlueprintReadOnly, Category="IC|Life") float Progress = 0;
protected:
    virtual void NativeOnInitialized() override;
};
