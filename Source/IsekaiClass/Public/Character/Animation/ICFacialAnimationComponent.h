#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ICFacialAnimationComponent.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;

/** Runtime-only blinking for players/NPCs and dialogue mouth animation for NPC speakers. */
UCLASS()
class ISEKAICLASS_API UICFacialAnimationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UICFacialAnimationComponent();
    void InitializeFace(USkeletalMeshComponent* Mesh);
    UFUNCTION(BlueprintCallable, Category="IC|Face") void PlayDialogueLine(const FText& Line, FName SpeakerID);
    UFUNCTION(BlueprintCallable, Category="IC|Face") void StopDialogue();
    UFUNCTION(BlueprintPure, Category="IC|Face") bool IsSpeaking() const;
    UFUNCTION(BlueprintPure, Category="IC|Face") bool IsFaceReady() const { return bFaceReady; }
    UFUNCTION(BlueprintPure, Category="IC|Face") bool CanAnimateSpeech() const { return bSpeechReady; }
    void ResetFace();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    struct FSpeechCue { int32 Mouth; float Start, End; };
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> FaceMesh;
    TWeakObjectPtr<USkeletalMesh> BoundMesh;
    TArray<FName> BlinkTargets;
    TArray<FSpeechCue> Speech;
    FRandomStream BlinkRandom;
    float MouthWeights[5] = {};
    float SpeechSeconds = 0.f;
    int32 SpeechIndex = 0;
    float BlinkWait = 0.f, BlinkSeconds = -1.f;
    bool bFaceReady = false, bSpeechReady = false;
    void SetBlink(float Weight);
    void UpdateBlink(float DeltaSeconds);
    void UpdateSpeech(float DeltaSeconds);
};
