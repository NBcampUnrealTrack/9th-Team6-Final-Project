#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"
#include "Data/Rows/Life/ICProductionTypes.h"
#include "ICProductionRow.generated.h"


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICProductionRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production")
    EICLifeActivity Activity = EICLifeActivity::Crafting;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production")
    EICProductionResultKind ResultKind = EICProductionResultKind::BagItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements")
    TArray<FICProductionToolRequirement> Tools;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements")
    TArray<FICProductionMaterialRequirement> Materials;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements")
    TArray<FName> WorkAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements")
    TArray<FICConditionGroup> StartConditions;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cost", meta=(ClampMin="0.0", Units="s"))
    double DurationPlaySeconds = 5.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cost", meta=(ClampMin="0.0"))
    double SPCost = 5.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production")
    bool bAllowRepeat = true;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirements", meta=(ClampMin="0.0", Units="cm"))
    double MaximumWorkDistanceCm = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result")
    TArray<FICItemAmount> Results;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Result")
    FICDefinitionRef ResultDefinition;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(EditCondition="ResultKind == EICProductionResultKind::InstallStorage"))
    TArray<FName> InstallationAnchorIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(EditCondition="ResultKind == EICProductionResultKind::InstallStorage"))
    TSoftClassPtr<AActor> InstalledActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(ClampMin="1", EditCondition="ResultKind == EICProductionResultKind::InstallStorage"))
    int32 StorageCapacityAdded = 40;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<UAnimMontage> WorkMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TSoftObjectPtr<USoundBase> CompletionSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    FSoftObjectPath WorkVFX;
};
