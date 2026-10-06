#pragma once
#include "CoreMinimal.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICPlayerItemTypes.generated.h"

USTRUCT(BlueprintType)
struct FPlayerEquipmentStats
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) float Attack=0.f;
    UPROPERTY(BlueprintReadOnly) float Defense=0.f;
    UPROPERTY(BlueprintReadOnly) float MaxHP=0.f;
    UPROPERTY(BlueprintReadOnly) float MaxMP=0.f;
    UPROPERTY(BlueprintReadOnly) float MaxSP=0.f;
    UPROPERTY(BlueprintReadOnly) float AttackSpeed=0.f;
    UPROPERTY(BlueprintReadOnly) float MoveSpeed=0.f;
    UPROPERTY(BlueprintReadOnly) float CriticalRate=0.f;
};
