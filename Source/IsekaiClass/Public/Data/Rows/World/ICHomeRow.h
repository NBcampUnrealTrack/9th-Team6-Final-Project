#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICHomeRow.generated.h"

UENUM(BlueprintType)
enum class EICHomeType : uint8 { Player , NPC , Inn  };

UENUM(BlueprintType)
enum class EICHomeContainerType : uint8 { Storage , Wardrobe , Bookshelf  };

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICHomeContainerDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container") FName ContainerID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container") EICHomeContainerType Type = EICHomeContainerType::Storage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container") FName AnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ClampMin="1")) int32 SlotCapacity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container") TArray<FICItemAmount> InitialItems;

};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICHomeRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") EICHomeType Type = EICHomeType::Player;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") FName OwnerNPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") FName EntranceAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") TArray<FName> BedAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") TArray<FName> KitchenAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") TArray<FName> CraftingAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Home") FName InnServiceID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPCContainers", meta=(ClampMin="0", ClampMax="100")) int32 ContainerAccessAffinity = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPCContainers") TArray<FICHomeContainerDefinition> NPCContainers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(EditCondition="Type == EICHomeType::Player")) TArray<FName> StorageInstallAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(EditCondition="Type == EICHomeType::Player")) TArray<FName> FarmPlotAnchorIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(ClampMin="1")) int32 StorageSlotsPerInstallation = 40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome", meta=(ClampMin="1", ClampMax="4")) int32 MaximumStorageInstallations = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerHome") FName SharedStorageGroupID;
};
