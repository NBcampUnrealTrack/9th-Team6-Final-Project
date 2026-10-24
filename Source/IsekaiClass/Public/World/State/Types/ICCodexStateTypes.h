#pragma once
#include "CoreMinimal.h"
#include "ICCodexStateTypes.generated.h"

USTRUCT(BlueprintType)
struct FICCodexState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) TSet<FName> AcquiredItemIDs;
    /** Stable zero-based slots, 0..54, assigned explicitly in the monster DT. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) TMap<int32,int32> PetEssence;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame) TMap<FName,int32> CharacterPets;
};
