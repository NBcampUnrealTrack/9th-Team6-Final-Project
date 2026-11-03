#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ICQuestGuideComponent.generated.h"

class AICQuestGuideLight;
class APawn;
class UICDefinitionSubsystem;
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
    float RepeatInterval = .25f; // Delay after the previous effect has completely disappeared.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(Units="cm"))
    float GuideHeight = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0", Units="cm"))
    float ArrivalRadius = 150;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="1", Units="cm"))
    float TailLength = 900.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide", meta=(ClampMin="0.01", Units="s"))
    float FadeOutDuration = .6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide|Navigation", meta=(ClampMin="1", Units="cm"))
    FVector NavigationProjectionExtent = FVector(100, 100, 200);

    // Visual guidance may cross NavMesh steps without changing character movement or AI navigation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide|Navigation")
    bool bBridgeNavigationSteps = true;

    // Horizontal gap between NavMesh boundaries. Vertical separation is unrestricted.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IC|Quest Guide|Navigation", meta=(ClampMin="1", Units="cm"))
    float StepBridgeMaxHorizontalGap = 200.f;

    UFUNCTION(BlueprintPure, Category="IC|Quest Guide")
    FName GetGuidedQuestID() const { return GuidedQuestID; }

    UFUNCTION(BlueprintPure, Category="IC|Quest Guide")
    bool IsQuestGuideEnabled(FName QuestID) const { return bGuideEnabled && !PausedQuestIDs.Contains(QuestID); }

    UFUNCTION(BlueprintCallable, Category="IC|Quest Guide")
    void SetQuestGuideEnabled(FName QuestID, bool bEnabled);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FICQuestGuideSequentialPulsesCheck;
#endif
    UFUNCTION() void RefreshGuide();
    UFUNCTION() void HandleWorldRestored();
    UFUNCTION() void HandleLightDestroyed(AActor* Light);
    bool ResolveTarget(const FICQuestRow& Definition, const FICQuestState& Quest,
        const FICWorldState& World, FVector& Location, FName& TargetKey) const;
    void ResetPulse(bool bImmediate = true);
    void StopActiveLights(bool bImmediate);
    void SchedulePulse(float Delay);
    bool SelectTarget(const FICWorldState& World, UICDefinitionSubsystem& Definitions,
        FName& QuestID, FName& TargetKey, FVector& Location) const;
    bool BuildGuidePath(APawn& Pawn, const FVector& Goal, TArray<FVector>& Points) const;
    void SpawnPulse(APawn& Pawn, const TArray<FVector>& Points);

    UPROPERTY(Transient) TArray<TObjectPtr<AICQuestGuideLight>> ActiveLights;
    UPROPERTY(Transient) TObjectPtr<UICWorldStateSubsystem> State;
    FTimerHandle RefreshTimer;
    FTimerHandle PulseTimer;
    FName GuidedQuestID;
    FName GuidedTargetKey;
    UPROPERTY(Transient) TSet<FName> PausedQuestIDs;
    double NextPulseTime = 0;
};
