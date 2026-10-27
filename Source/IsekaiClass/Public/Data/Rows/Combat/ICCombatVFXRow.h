#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ICCombatVFXRow.generated.h"
class UNiagaraSystem;

/** Visual controls never alter damage, collision or the authored attack timing. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNiagaraEffect
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bEnabled=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TSoftObjectPtr<UNiagaraSystem> System;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor Color=FLinearColor::White;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float Intensity=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Scale=FVector::OneVector;
    /** Zero follows the particle asset or the projectile's lifetime. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float Duration=0;
    /** Set for imported assets with a fixed authored lifetime, to retime them. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float ReferenceDuration=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Offset=FVector::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FRotator Rotation=FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.01")) float WidthCm=2;
};

/** Shared by DT_IC_Class combo stages and DT_IC_Skill/DT_IC_SkillEffect. */
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCombatVFXRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FText Description;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FICNiagaraEffect Slash;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FICNiagaraEffect Range;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FICNiagaraEffect Projectile;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FICNiagaraEffect Trail;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FICNiagaraEffect Cast;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FICNiagaraEffect Impact;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float SlashSlope=.22f;
    /** Retain the original mesh effect if a Niagara asset is missing. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bLegacyMeshFallback=true;
};
