#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ICLoadingScreenSubsystem.generated.h"

UCLASS()
class ISEKAICLASS_API UICLoadingScreenSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void Arm();

private:
    UPROPERTY(Transient) TObjectPtr<class UFont> LoadingFont;
    bool bArmed = false;
    void BeforeMap(const FString& MapName);
    void AfterMap(UWorld* World);
    void TravelFailed(UWorld* World, ETravelFailure::Type Failure, const FString& Reason);
};
