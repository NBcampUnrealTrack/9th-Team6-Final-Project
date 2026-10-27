#pragma once
#include "Character/Base/ICCharacterBase.h"
#include "ICRanchAnimal.generated.h"

/** A huntable life animal. Persistent identity is the placed PointID, not a monster spawn slot. */
UCLASS()
class ISEKAICLASS_API AICRanchAnimal : public AICCharacterBase
{
    GENERATED_BODY()
public:
    AICRanchAnimal();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Life") FName DefinitionID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="IC|Life") FName PointID;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void HandleZeroHP() override;
    UFUNCTION(BlueprintCallable,Category="IC|Life") void RefreshAnimal();
private:
    FTimerHandle RefreshTimer;
    bool bRefreshing=false;
    FName PointKey() const;
    UFUNCTION() void PersistVitals();
    UFUNCTION() void GameTimeJumped(double From,double To);
};
