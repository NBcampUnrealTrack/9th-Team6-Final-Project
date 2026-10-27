#pragma once

#include "CoreMinimal.h"
#include "Interaction/ICInteractionActor.h"
#include "ICNPCContainerActor.generated.h"

class AICPlayerController;

/**
 * A world object owned by one NPC, such as that NPC's chest, wardrobe, or bookshelf.
 *
 * This actor owns only NPC-side concerns: identity of the owner and the affinity
 * gate. Generic item transfer, storage UI, and save serialization remain owned by
 * the shared inventory system.
 */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICNPCContainerActor : public AICInteractionActor
{
    GENERATED_BODY()

public:
    AICNPCContainerActor();

    /** Stable NPC definition ID that owns this personal container. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Container")
    FName OwnerNPCID;

    /** Current design rule: personal NPC containers unlock at affinity 200. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Container", meta=(ClampMin="200", ClampMax="200"))
    int32 RequiredAffinity = 200;

    /** Shown when the player has not reached the affinity requirement. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Container")
    FText LockedMessage;

    UFUNCTION(BlueprintPure, Category="IC|NPC Container")
    bool CanAccessContainer(const AICPlayerController* Requester) const;

    virtual bool TryInteract_Implementation(AICPlayerController* Controller) override;
};
