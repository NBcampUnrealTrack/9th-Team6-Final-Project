#pragma once

#include "CoreMinimal.h"
#include "Data/Rows/Common/ICDefinitionTypes.h"
#include "ICDialogueRow.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EICDialogueExpression : uint8
{
    Neutral UMETA(DisplayName="기본"),
    Joy UMETA(DisplayName="기쁨"),
    Smile UMETA(DisplayName="미소"),
    Angry UMETA(DisplayName="화남"),
    Sad UMETA(DisplayName="슬픔"),
    Surprised UMETA(DisplayName="놀람"),
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName ChoiceID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FString TextKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NextNodeID;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NodeID;
    // CHR_PLAYER identifies an explicit player line. None remains an unspecified speaker.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName SpeakerNPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FString TextKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NextNodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FICDialogueChoice> Choices;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation") TSoftObjectPtr<UAnimMontage> GestureMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    EICDialogueExpression Expression = EICDialogueExpression::Neutral;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation",
        meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="Expression != EICDialogueExpression::Neutral"))
    float ExpressionStrength = .5f;
};

USTRUCT(BlueprintType)
struct ISEKAICLASS_API FICDialogueRow : public FICDefinitionRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NPCID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName EntryNodeID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FICConditionGroup Availability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") int32 SelectionPriority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FICDialogueNode> Nodes;
};
