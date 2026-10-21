#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICGiftPreferenceRow.generated.h"

UENUM(BlueprintType)
enum class EICGiftPreference : uint8 { Preferred , Neutral , Disliked  };

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICGiftPreferenceEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift") FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift") EICGiftPreference Preference = EICGiftPreference::Neutral;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICGiftPreferenceRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift") int32 PreferredAffinityDelta = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift") int32 NeutralAffinityDelta = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift") int32 DislikedAffinityDelta = -5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gift") TArray<FICGiftPreferenceEntry> Items;
};
