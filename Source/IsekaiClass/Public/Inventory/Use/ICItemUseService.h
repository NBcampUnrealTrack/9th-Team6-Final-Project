#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "World/State/ICWorldStateTypes.h"
#include "ICItemUseService.generated.h"
class UICInventoryComponent;
class UICDefinitionSubsystem;

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICItemUseResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bSuccess=false;
    UPROPERTY(BlueprintReadOnly) FName ItemID;
    UPROPERTY(BlueprintReadOnly) FName TargetID;
    UPROPERTY(BlueprintReadOnly) FString Field;
    UPROPERTY(BlueprintReadOnly) FString Reason;
    UPROPERTY(BlueprintReadOnly) FICCharacterState Character;
    UPROPERTY(BlueprintReadOnly) FICInventoryState Inventory;
    UPROPERTY(BlueprintReadOnly) EPlayerMainStat GrowthStat=EPlayerMainStat::None;
    UPROPERTY(BlueprintReadOnly) int32 GrowthAmount=0;
};

UCLASS()
class ISEKAICLASS_API UICItemUseService : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:

    UFUNCTION(BlueprintCallable,Category="IC|Items")
    static FICItemUseResult Prepare(UICInventoryComponent* Inventory,UICDefinitionSubsystem* Definitions,FName ItemID,const FICCharacterState& Target);
};
