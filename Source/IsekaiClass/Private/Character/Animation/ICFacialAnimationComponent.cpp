#include "Character/Animation/ICFacialAnimationComponent.h"
#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Character/Base/ICCharacterBase.h"
#include "NPC/Character/ICNPCBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "HAL/PlatformTime.h"

namespace
{
const FName MouthTargets[] = {TEXT("Fcl_MTH_A"), TEXT("Fcl_MTH_I"), TEXT("Fcl_MTH_U"), TEXT("Fcl_MTH_E"), TEXT("Fcl_MTH_O")};
constexpr float BlinkClose = .055f, BlinkHold = .025f, BlinkOpen = .10f;

int32 MouthForCharacter(TCHAR Character)
{
    // Modern Hangul medial vowels, in Unicode order: a, ae, ya, yae, eo ... i.
    static constexpr int32 HangulMouth[] = {0,3,0,3,4,3,4,3,4,0,3,3,4,2,4,3,1,2,2,1,1};
    const uint32 Code = uint32(Character);
    if (Code >= 0xAC00 && Code <= 0xD7A3) return HangulMouth[((Code - 0xAC00) / 28) % 21];
    if (Code >= 0x314F && Code <= 0x3163) return HangulMouth[Code - 0x314F];
    switch (FChar::ToLower(Character))
    {
    case TEXT('a'): return 0;
    case TEXT('i'): case TEXT('y'): return 1;
    case TEXT('u'): return 2;
    case TEXT('e'): return 3;
    case TEXT('o'): return 4;
    default: return FChar::IsAlnum(Character) ? 0 : INDEX_NONE;
    }
}
}

UICFacialAnimationComponent::UICFacialAnimationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UICFacialAnimationComponent::InitializeFace(USkeletalMeshComponent* Mesh)
{
    ResetFace();
    if (FaceMesh) FaceMesh->RemoveTickPrerequisiteComponent(this);
    FaceMesh = Mesh;
    BoundMesh = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
    BlinkTargets.Reset();
    bFaceReady = false; bSpeechReady = false;
    if (const auto* Asset = BoundMesh.Get())
    {
        if (Asset->FindMorphTarget(TEXT("Fcl_EYE_Close"))) BlinkTargets.Add(TEXT("Fcl_EYE_Close"));
        else for (FName Name : {FName(TEXT("Fcl_EYE_Close_L")), FName(TEXT("Fcl_EYE_Close_R"))})
            if (Asset->FindMorphTarget(Name)) BlinkTargets.Add(Name);
        bFaceReady = !BlinkTargets.IsEmpty();
        bSpeechReady = Cast<AICNPCBase>(GetOwner()) != nullptr;
        for (FName Name : MouthTargets) bSpeechReady &= Asset->FindMorphTarget(Name) != nullptr;
    }
    if (FaceMesh && (bFaceReady || bSpeechReady))
    {
        // Feed component overrides before the skeletal mesh evaluates its existing post-process graph.
        FaceMesh->AddTickPrerequisiteComponent(this);
        AddTickPrerequisiteActor(GetOwner());
    }
    BlinkRandom.Initialize(int32(FPlatformTime::Cycles()) ^ GetTypeHash(GetOwner()->GetFName()));
    BlinkWait = BlinkRandom.FRandRange(1.f, 3.f);
    SetComponentTickEnabled(bFaceReady || bSpeechReady);
}

void UICFacialAnimationComponent::PlayDialogueLine(const FText& Line, FName SpeakerID)
{
    StopDialogue();
    const auto* NPC = Cast<AICNPCBase>(GetOwner());
    if (!bSpeechReady || !NPC || SpeakerID.IsNone() || SpeakerID != NPC->GetNPCDefinition().ID) return;
    float End = 0.f;
    for (TCHAR Character : Line.ToString())
    {
        const int32 Mouth = MouthForCharacter(Character);
        const float Duration = FChar::IsWhitespace(Character) ? .065f
            : Character == TEXT(',') || Character == TEXT(';') || Character == TEXT(':') ? .18f
            : Mouth == INDEX_NONE ? .28f : .14f;
        Speech.Add({Mouth, End, End + Duration});
        End += Duration;
    }
}

bool UICFacialAnimationComponent::IsSpeaking() const
{
    return bSpeechReady && Speech.IsValidIndex(SpeechIndex);
}

void UICFacialAnimationComponent::StopDialogue()
{
    Speech.Reset(); SpeechIndex = 0; SpeechSeconds = 0.f;
    // Player faces never write or clear mouth overrides, including on load/preset changes.
    if (!bSpeechReady) return;
    bool bChanged = false;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(MouthTargets); ++Index)
    {
        bChanged |= MouthWeights[Index] > 0.f;
        MouthWeights[Index] = 0.f;
        if (FaceMesh) FaceMesh->SetMorphTarget(MouthTargets[Index], 0.f);
    }
    // Gift/menu screens can pause before another mesh tick. Publish the closed mouth now.
    if (bChanged && FaceMesh && FaceMesh->IsRegistered())
    {
        FaceMesh->HandleExistingParallelEvaluationTask(true, true);
        FaceMesh->RefreshBoneTransforms();
        FaceMesh->HandleExistingParallelEvaluationTask(true, true);
        FaceMesh->MarkRenderDynamicDataDirty();
    }
}

void UICFacialAnimationComponent::SetBlink(float Weight)
{
    if (FaceMesh) for (FName Name : BlinkTargets) FaceMesh->SetMorphTarget(Name, Weight);
}

void UICFacialAnimationComponent::ResetFace()
{
    StopDialogue(); SetBlink(0.f);
    BlinkSeconds = -1.f; BlinkWait = BlinkRandom.FRandRange(2.5f, 5.5f);
}

void UICFacialAnimationComponent::UpdateBlink(float DeltaSeconds)
{
    if (BlinkSeconds < 0.f)
    {
        BlinkWait -= DeltaSeconds;
        if (BlinkWait > 0.f) return;
        BlinkSeconds = 0.f;
    }
    // Preserve a visible closed-eye sample when a slow frame spans the whole blink.
    BlinkSeconds += FMath::Min(DeltaSeconds, BlinkClose + BlinkHold);
    if (BlinkSeconds >= BlinkClose + BlinkHold + BlinkOpen)
    {
        SetBlink(0.f); BlinkSeconds = -1.f; BlinkWait = BlinkRandom.FRandRange(2.5f, 5.5f);
        return;
    }
    const float Weight = BlinkSeconds < BlinkClose ? FMath::SmoothStep(0.f, BlinkClose, BlinkSeconds)
        : 1.f - FMath::SmoothStep(BlinkClose + BlinkHold, BlinkClose + BlinkHold + BlinkOpen, BlinkSeconds);
    SetBlink(Weight);
}

void UICFacialAnimationComponent::UpdateSpeech(float DeltaSeconds)
{
    SpeechSeconds += DeltaSeconds;
    while (Speech.IsValidIndex(SpeechIndex) && SpeechSeconds >= Speech[SpeechIndex].End) ++SpeechIndex;
    const FSpeechCue* Cue = Speech.IsValidIndex(SpeechIndex) ? &Speech[SpeechIndex] : nullptr;
    const float Progress = Cue ? (SpeechSeconds - Cue->Start) / (Cue->End - Cue->Start) : 0.f;
    // Modest opening with smooth transitions; spaces and punctuation return to a closed mouth.
    const float Opening = .65f * (.35f + .65f * FMath::Sin(PI * Progress));
    const float Blend = 1.f - FMath::Exp(-24.f * DeltaSeconds);
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(MouthTargets); ++Index)
    {
        MouthWeights[Index] = FMath::Lerp(MouthWeights[Index], Cue && Cue->Mouth == Index ? Opening : 0.f, Blend);
        if (MouthWeights[Index] < .001f) MouthWeights[Index] = 0.f;
        FaceMesh->SetMorphTarget(MouthTargets[Index], MouthWeights[Index]);
    }
    if (!Cue && Speech.Num()) { Speech.Reset(); SpeechIndex = 0; SpeechSeconds = 0.f; }
}

void UICFacialAnimationComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
    if (!FaceMesh || (!bFaceReady && !bSpeechReady)) return;
    if (BoundMesh.Get() != FaceMesh->GetSkeletalMeshAsset()) { InitializeFace(FaceMesh); return; }
    const auto* NPC = Cast<AICNPCBase>(GetOwner());
    const auto* Character = Cast<AICCharacterBase>(GetOwner());
    const auto* Ability = Character ? Character->GetICAbilitySystem() : nullptr;
    const auto* Anim = Cast<UICCharacterAnimInstance>(FaceMesh->GetAnimInstance());
    const auto* Life = GetOwner()->FindComponentByClass<UICPlayerLifeComponent>();
    const bool bSleeping = (Anim && Anim->bSleeping) || (NPC && NPC->IsSleepingForInteraction()) || (Life && Life->IsSleeping());
    if (bSleeping || (Ability && !Ability->IsAlive()))
    { ResetFace(); return; }
    const float Step = FMath::Max(0.f, DeltaSeconds);
    if (bFaceReady) UpdateBlink(Step);
    if (bSpeechReady) UpdateSpeech(Step);
}

void UICFacialAnimationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ResetFace();
    if (FaceMesh) FaceMesh->RemoveTickPrerequisiteComponent(this);
    Super::EndPlay(Reason);
}
