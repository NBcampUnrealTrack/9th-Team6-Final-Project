#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Combat/ICCombatTypes.h"
#include "ICSynergyRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICSynergyRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy") TArray<FName> MemberNPCIDs;
    /** Zero retains the original all-members rule. Positive values count this NPC pool. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Synergy") int32 MinimumMembers=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Synergy") EICWeaponFamily RequiredWeaponFamily=EICWeaponFamily::None;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Synergy") bool bCountPlayer=true;
    /** Highest priority in a group wins. Separate additive groups sum their percentages. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Synergy") FName ExclusiveGroup;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Synergy") int32 Priority=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Synergy") bool bAdditiveWithOtherGroups=false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy") FICConditionGroup ActivationConditions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Synergy") TArray<FICStatModifier> StatModifiers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TSoftObjectPtr<UNiagaraSystem> ActivationVFX;
};
