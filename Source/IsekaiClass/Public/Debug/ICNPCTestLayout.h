#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICNPCTestLayout.generated.h"

class AICWorldAnchor;

/** A single test-map origin; normal spawn and schedule systems still own NPCs. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICNPCTestLayout : public AActor
{
    GENERATED_BODY()
public:
    AICNPCTestLayout();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Test") TArray<FName> NPCIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Test", meta=(ClampMin="100", Units="cm")) float RowSpacingCm = 350;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Test", meta=(ClampMin="200", Units="cm")) float BedDistanceCm = 700;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Test", meta=(Units="cm")) float AnchorHeightCm = 88;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|NPC Test") TSoftClassPtr<AActor> BedClass;
    UFUNCTION(BlueprintPure, Category="IC|NPC Test") FTransform StandingTransform(int32 Index) const;
    UFUNCTION(BlueprintPure, Category="IC|NPC Test") FTransform BedTransform(int32 Index) const;
    UPROPERTY(Transient, BlueprintReadOnly, Category="IC|NPC Test") TArray<TObjectPtr<AICWorldAnchor>> GeneratedAnchors;
    UPROPERTY(Transient, BlueprintReadOnly, Category="IC|NPC Test") TArray<TObjectPtr<AActor>> GeneratedBeds;
};
