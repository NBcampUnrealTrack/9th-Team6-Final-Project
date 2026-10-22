#pragma once
#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICCombatCueNotify.generated.h"

UCLASS()
class ISEKAICLASS_API UICCombatCuePayload : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY() FICCombatPresentation Presentation;
    UPROPERTY() bool bImpact = false;
};

UCLASS(Blueprintable)
class ISEKAICLASS_API UICCombatCueNotify : public UGameplayCueNotify_Static
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category="IC|Cue") TSoftObjectPtr<UNiagaraSystem> DefaultVFX;
    UPROPERTY(EditDefaultsOnly, Category="IC|Cue") TSoftObjectPtr<USoundBase> DefaultSound;
    virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
