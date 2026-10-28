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
    UPROPERTY(BlueprintReadWrite,Category="QuickSlot") int32 SlotNumber=1;
    void SetSlotNumber(int32 Number){SlotNumber=Number;RefreshPresentation();}
protected:
    virtual void NativePreConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
private:
    bool bGamepadHints=false;
    void RefreshPresentation();
    void RefreshCooldown();
};
