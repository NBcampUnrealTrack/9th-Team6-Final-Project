#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICQuestTrackerWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable)
class ISEKAICLASS_API UICQuestTrackerWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> QuestText;

    UFUNCTION()
    void RefreshQuests();
};
