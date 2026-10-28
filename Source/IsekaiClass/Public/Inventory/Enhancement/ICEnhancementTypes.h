#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Inventory/Types/ICInventoryTypes.h"
#include "ICEnhancementTypes.generated.h"
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEnhancementOreRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ItemID;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1")) int32 Count=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ClampMax="1")) double SuccessBonus=0;
};
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICEnhancementTarget
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FPlayerItemInstance Item;
    UPROPERTY(BlueprintReadOnly) FName OwnerID;
    UPROPERTY(BlueprintReadOnly) FText Location;
};
