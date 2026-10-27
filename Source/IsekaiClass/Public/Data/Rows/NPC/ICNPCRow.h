#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICNPCRow.generated.h"

class AActor;
class UAnimInstance;
class USkeletalMesh;
class UTexture2D;
class UICCharacterAnimationProfile;

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") bool bClassmate = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relationship", meta=(ClampMin="0", ClampMax="500")) int32 InitialAffinity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FName AppearanceResourceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(DisplayName="Portrait (초상화)", ToolTip="동료 HUD에 사용할 이 NPC의 초상화입니다. 파티 슬롯과 관계없이 NPC ID에 맞는 이미지를 지정하세요."))
    TSoftObjectPtr<UTexture2D> Portrait;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftClassPtr<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<UICCharacterAnimationProfile> AnimationProfile;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") FTransform MeshTransform = FTransform(FRotator(0,-90,0),FVector(0,0,-88));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftClassPtr<AActor> ActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI", meta=(AllowedClasses="/Script/StateTreeModule.StateTree")) TSoftObjectPtr<UObject> ScheduleStateTree;
};
