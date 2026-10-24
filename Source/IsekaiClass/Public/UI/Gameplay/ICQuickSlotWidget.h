#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICQuickSlotWidget.generated.h"

class UTexture2D;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuickSlotWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="QuickSlot") FName ItemID;
    UPROPERTY(BlueprintReadOnly, Category="QuickSlot") int32 Count = 0;
    UPROPERTY(BlueprintReadOnly, Category="QuickSlot") TObjectPtr<UTexture2D> Icon;
    UFUNCTION(BlueprintCallable, Category="QuickSlot") void SetItem(FName InItemID, int32 InCount, UTexture2D* InIcon);
protected:
    virtual void NativePreConstruct() override;
private:
    void RefreshPresentation();
};
