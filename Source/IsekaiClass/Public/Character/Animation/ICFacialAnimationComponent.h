#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Rows/NPC/ICDialogueRow.h"
#include "ICFacialAnimationComponent.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;

/** Runtime blinking, dialogue expressions and speech shapes. */
UCLASS()
class ISEKAICLASS_API UICFacialAnimationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICFacialAnimationComponent();
    void InitializeFace(USkeletalMeshComponent* Mesh);
    UFUNCTION(BlueprintCallable, Category="IC|Face") void PlayDialogueLine(const FText& Line, FName SpeakerID);
    UFUNCTION(BlueprintCallable, Category="IC|Face") void StopDialogue();
    void SetDialogueExpression(EICDialogueExpression Expression, float Strength);
    void ClearDialogueExpression();
    UFUNCTION(BlueprintPure, Category="IC|Face") bool IsSpeaking() const;
    UFUNCTION(BlueprintPure, Category="IC|Face") bool IsFaceReady() const { return bFaceReady; }
    UFUNCTION(BlueprintPure, Category="IC|Face") bool CanAnimateSpeech() const { return bSpeechReady; }
    void ResetFace();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    struct FSpeechCue { int32 Mouth; float Start, End; };
    enum class EExpressionPart : uint8 { Brow, Eye, Mouth };
    struct FExpressionMorph
    {
        FName Name;
        EICDialogueExpression Expression;
        EExpressionPart Part;
        float Weight = 0.f;
    };
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> FaceMesh;
    TWeakObjectPtr<USkeletalMesh> BoundMesh;
    TArray<FName> BlinkTargets;
    TArray<FSpeechCue> Speech;
    TArray<FExpressionMorph> ExpressionMorphs;
    EICDialogueExpression DialogueExpression = EICDialogueExpression::Neutral;
    float ExpressionStrength = 0.f;
    float BlinkWeight = 0.f;
    FRandomStream BlinkRandom;
    float MouthWeights[5] = {};
    float SpeechSeconds = 0.f;
    int32 SpeechIndex = 0;
    float BlinkWait = 0.f, BlinkSeconds = -1.f;
    bool bFaceReady = false, bSpeechReady = false;
    void SetBlink(float Weight);
    void UpdateBlink(float DeltaSeconds);
    void UpdateSpeech(float DeltaSeconds);
    void UpdateExpression(float DeltaSeconds);
    void RefreshRenderedFace();
};
