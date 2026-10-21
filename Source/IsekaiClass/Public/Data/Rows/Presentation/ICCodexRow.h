#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICCodexRow.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EICCodexCategory : uint8 { Item , NPC , Landmark  };

UENUM(BlueprintType)
enum class EICCodexRegistration : uint8 { FirstItemInspectionOrAcquisition , FirstNPCConversation , LandmarkInteraction  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCodexRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex") EICCodexCategory Category = EICCodexCategory::Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex") FICDefinitionRef Subject;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex") EICCodexRegistration Registration = EICCodexRegistration::FirstItemInspectionOrAcquisition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex") int32 DisplayOrder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex") TSoftObjectPtr<UTexture2D> Image;
};
