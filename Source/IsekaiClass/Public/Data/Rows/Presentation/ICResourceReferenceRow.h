#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICResourceReferenceRow.generated.h"

class USkeleton;

UENUM(BlueprintType)
enum class EICResourceKind : uint8
{
    SkeletalMesh , StaticMesh , Material , Texture , Animation , AnimationBlueprint ,
    Montage , Rig , Retargeter , VFX , Sound , BGM , MapTexture , LevelSequence , Blueprint , Other
};


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICResourceReferenceRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") EICResourceKind Kind = EICResourceKind::Other;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") TSoftObjectPtr<UObject> Asset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") TArray<FICDefinitionRef> UsedBy;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") TSoftObjectPtr<USkeleton> Skeleton;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") FName SkeletonGroup;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") TArray<FName> RequiredSockets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") TArray<FName> RequiredNotifyNames;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Specification", meta=(Units="cm")) FVector DimensionsCm = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Specification") FVector ImportScale = FVector::OneVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Specification") FString AxisAndGripNotes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Provenance") FString SourceURL;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Provenance") FString License;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Provenance") FString Attribution;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Verification") bool bVerifiedInRepresentativeScene = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Verification") bool bVerifiedForCooking = false;
};
