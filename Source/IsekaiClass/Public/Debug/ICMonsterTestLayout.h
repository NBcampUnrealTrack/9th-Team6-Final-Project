#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICMonsterTestLayout.generated.h"

/** Runtime anchors use the regular persistent spawn / respawn system. */
UCLASS(Blueprintable)
class ISEKAICLASS_API AICMonsterTestLayout : public AActor
{
    GENERATED_BODY()
public:
    AICMonsterTestLayout();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|Monster Test") FName SpawnerID=TEXT("SPN_R01_001");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IC|Monster Test") float SpacingCm=700;
    UFUNCTION(BlueprintPure, Category="IC|Monster Test") FTransform OriginTransform(int32 Index) const;
    UPROPERTY(Transient) TArray<TObjectPtr<class AICWorldAnchor>> GeneratedAnchors;
};
