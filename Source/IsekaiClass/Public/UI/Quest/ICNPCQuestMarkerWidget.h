#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICNPCQuestMarkerWidget.generated.h"

UCLASS(Blueprintable)
class ISEKAICLASS_API UICNPCQuestMarkerWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="IC|Quest") void SetNPC(class AICNPCBase* Value);
    UFUNCTION(BlueprintCallable, Category="IC|Quest") void RefreshMarker();
protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
private:
    TWeakObjectPtr<class AICNPCBase> NPC;
    float RefreshRemaining=0;
};
