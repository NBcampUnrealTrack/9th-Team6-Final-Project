#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ICQuestGuideComponent.generated.h"

class AICQuestGuideLight;
class UICWorldStateSubsystem;
struct FICQuestRow;
struct FICQuestState;
struct FICWorldState;

/** Local presentation only. Quest acceptance and completion remain in ICQuestSubsystem. */
UCLASS(ClassGroup=(IC), meta=(BlueprintSpawnableComponent))
class ISEKAICLASS_API UICQuestGuideComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UICQuestGuideComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide")
    bool bGuideEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide")
    TSubclassOf<AICQuestGuideLight> GuideLightClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="1", Units="cm"))
    float GuideDistance = 1500; // Maximum accumulated path distance per spawned effect.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="1", Units="cm/s"))
    float GuideSpeed = 600;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0.1", Units="s"))
    float RepeatInterval = 3; // Spawn cadence, independent of previous effects.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(Units="cm"))
    float GuideHeight = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0", Units="cm"))
    float ArrivalRadius = 150;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="1", Units="cm"))
    float TailLength = 220.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0.01", Units="s"))
    float FadeOutDuration = .6f;

    UFUNCTION(BlueprintPure, Category="IC|Quest Guide")
    FName GetGuidedQuestID() const { return GuidedQuestID; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UFUNCTION() void RefreshGuide();
    UFUNCTION() void HandleWorldRestored();
    bool ResolveTarget(const FICQuestRow& Definition, const FICQuestState& Quest,
        const FICWorldState& World, FVector& Location, FName& TargetKey) const;
    void ResetPulse(bool bImmediate = true);

    UPROPERTY(Transient) TArray<TObjectPtr<AICQuestGuideLight>> ActiveLights;
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> State;
    FTimerHandle RefreshTimer;
    FTimerHandle PulseTimer;
    FName GuidedQuestID;
    FName GuidedTargetKey;
    double NextPulseTime = 0;
};
