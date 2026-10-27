#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ICCharacterProfileRow.generated.h"

class UTexture2D;

// Row names are the actual character IDs: CHR_PLAYER or NPC_R01_001, etc.
USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICCharacterProfileRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Profile", meta=(DisplayName="Main Setting (주 설정)")) FText MainSetting;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Profile", meta=(MultiLine="true", DisplayName="Story (이야기)")) FText Story;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Profile", meta=(DisplayName="Pet Portrait (설정된 펫 이미지)")) TSoftObjectPtr<UTexture2D> PetPortrait;
};
