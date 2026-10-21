#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICSynergyRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSynergyRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy") TArray<FName> MemberNPCIDs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy") FICConditionGroup ActivationConditions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy") TArray<FICStatModifier> StatModifiers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UNiagaraSystem> ActivationVFX;
};
