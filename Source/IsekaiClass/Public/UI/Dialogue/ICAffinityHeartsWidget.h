#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICAffinityHeartsWidget.generated.h"

UCLASS(Blueprintable)
class ISEKAICLASS_API UICAffinityHeartsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affinity", meta=(ClampMin="0", ClampMax="500"))
    int32 Affinity = 0;
    UFUNCTION(BlueprintCallable, Category="Affinity") void SetAffinity(int32 Value);
protected:
    virtual void NativePreConstruct() override;
private:
    void RefreshHearts();
};
