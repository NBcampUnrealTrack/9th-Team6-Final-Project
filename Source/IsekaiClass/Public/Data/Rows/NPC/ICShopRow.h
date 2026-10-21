#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICShopRow.generated.h"

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICShopEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop") FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop") int32 DisplayOrder = 0;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICShopRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop") FName ServiceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop") TArray<FICShopEntry> Entries;
};
