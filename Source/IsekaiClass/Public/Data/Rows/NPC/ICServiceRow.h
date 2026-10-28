#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICServiceRow.generated.h"

UENUM(BlueprintType)
enum class EICServiceType : uint8 { Shop , Enhancement , Warp , Inn  };

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICServiceHours
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hours", meta=(ClampMin="0", ClampMax="1439", Units="min")) int32 OpenGameMinute = 360;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hours", meta=(ClampMin="1", ClampMax="1440", Units="min")) int32 CloseGameMinute = 1200;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICAffinityDiscountTier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price", meta=(ClampMin="0", ClampMax="100")) int32 MinimumAffinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price", meta=(ClampMin="0", ClampMax="1")) double DiscountRatio = 0.0;
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICServiceRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") EICServiceType Type = EICServiceType::Shop;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") FName WorkAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") bool bRequireWorkSchedule=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") TArray<FICServiceHours> OpeningHours;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") FName ShopID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") FName WarpID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Service") FICConditionGroup Availability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price", meta=(ClampMin="0")) int32 BaseCostGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price") TArray<FICAffinityDiscountTier> AffinityDiscounts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FName IconResourceID;
};
