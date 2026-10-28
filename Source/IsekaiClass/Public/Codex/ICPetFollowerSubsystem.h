#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ICPetFollowerSubsystem.generated.h"
class AICPetFollower;

/** Recreates cosmetic followers from saved codex equipment as party membership changes. */
UCLASS()
class ISEKAICLASS_API UICPetFollowerSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintCallable,Category="Pet") void RefreshFollowers();
    UFUNCTION(BlueprintPure,Category="Pet") AICPetFollower* GetFollower(FName CharacterID) const;
private:
    UPROPERTY(Transient) TMap<FName,TObjectPtr<AICPetFollower>> Followers;
    float RefreshIn=0;
    bool bRefreshing=false;
};
