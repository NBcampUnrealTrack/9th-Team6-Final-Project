#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICNPCRow.generated.h"

class AActor;
class UAnimInstance;
class USkeletalMesh;
class UTexture2D;

UENUM(BlueprintType)
enum class EICNPCRace : uint8 { Human , Elf , Dwarf , Beastfolk  };


USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICNPCRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") EICNPCType Type = EICNPCType::Background;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") EICNPCRace Race = EICNPCRace::Human;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName RegionID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName HomeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName InitialAnchorID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName ScheduleID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") TArray<FName> DialogueIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName GiftPreferenceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") TArray<FName> ServiceIDs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName RecruitCombatID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName ClassID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FGameplayTag OccupationTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") bool bRecruitable = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") bool bEndingTarget = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relationship", meta=(ClampMin="0", ClampMax="500")) int32 InitialAffinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FName AppearanceResourceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftClassPtr<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftClassPtr<AActor> ActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI", meta=(AllowedClasses="/Script/StateTreeModule.StateTree")) TSoftObjectPtr<UObject> ScheduleStateTree;
};
