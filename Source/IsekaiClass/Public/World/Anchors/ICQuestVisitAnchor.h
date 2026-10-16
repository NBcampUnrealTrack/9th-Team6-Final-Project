#pragma once

#include "CoreMinimal.h"
#include "World/Anchors/ICWorldAnchor.h"
#include "ICQuestVisitAnchor.generated.h"

class USphereComponent;

UCLASS(Blueprintable)
class ISEKAICLASS_API AICQuestVisitAnchor : public AICWorldAnchor
{
    GENERATED_BODY()

public:
    AICQuestVisitAnchor();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="IC|Visit", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USphereComponent> VisitArea;

    UFUNCTION()
    void HandleVisit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool IsFromSweep,
        const FHitResult& SweepResult);
};
