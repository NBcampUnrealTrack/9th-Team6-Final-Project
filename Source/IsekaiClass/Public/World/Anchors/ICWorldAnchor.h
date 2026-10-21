#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICWorldAnchor.generated.h"
UCLASS(Blueprintable)
class ISEKAICLASS_API AICWorldAnchor : public AActor
{
    GENERATED_BODY()
public:
    AICWorldAnchor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditInstanceOnly,BlueprintReadOnly,Category="IC|Anchor") FName AnchorID;
};
