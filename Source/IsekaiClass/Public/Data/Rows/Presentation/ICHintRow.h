#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICHintRow.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EICFirstHintTrigger : uint8 { Movement , Combat , Item , StatGrowth , Production , Gift , Recruit , Sleep  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICHintRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint") EICFirstHintTrigger Trigger = EICFirstHintTrigger::Movement;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint") FGameplayTag ActionPromptTag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint") int32 DisplayOrder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hint") TSoftObjectPtr<UTexture2D> Illustration;
};
