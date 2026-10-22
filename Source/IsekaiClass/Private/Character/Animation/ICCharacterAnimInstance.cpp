#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Character/Base/ICCharacterBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
struct FICCharacterAnimProxy : FAnimInstanceProxy
{
    FAnimNode_BlendSpacePlayer_Standalone Ground;
    FAnimNode_SequencePlayer_Standalone Air, Bed;
    FAnimNode_TwoWayBlend AirBlend, SleepBlend;
    FAnimNode_Slot Slot;

    explicit FICCharacterAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
    {
        AirBlend.A.SetLinkNode(&Ground); AirBlend.B.SetLinkNode(&Air);
        SleepBlend.A.SetLinkNode(&AirBlend); SleepBlend.B.SetLinkNode(&Bed);
        Slot.Source.SetLinkNode(&SleepBlend); Slot.SlotName = TEXT("DefaultSlot");
        Air.SetLoopAnimation(true); Bed.SetLoopAnimation(true);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Slot; }
    virtual void GetCustomNodes(TArray<FAnimNode_Base*>& Out) override
    { Out = {&Ground, &Air, &Bed, &AirBlend, &SleepBlend, &Slot}; }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* IC = CastChecked<UICCharacterAnimInstance>(Instance);
        Ground.SetBlendSpace(IC->Locomotion); Ground.SetPosition(FVector(IC->Speed,0,0));
        Air.SetSequence(IC->Jump); Bed.SetSequence(IC->Sleep);
        AirBlend.Alpha = FMath::FInterpTo(AirBlend.Alpha, IC->bAirborne && IC->Jump ? 1.f : 0.f, DeltaSeconds, 12.f);
        // The current graph uses a sustained sleep clip; do not blend a standing body through the mattress.
        SleepBlend.Alpha = IC->bSleeping && IC->Sleep ? 1.f : 0.f;
    }
};
}

FAnimInstanceProxy* UICCharacterAnimInstance::CreateAnimInstanceProxy() { return new FICCharacterAnimProxy(this); }
void UICCharacterAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }

void UICCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    auto* Character = Cast<AICCharacterBase>(TryGetPawnOwner());
    if (!Character) return;
    auto* Profile = Character->GetAnimationProfile();
    if (!Profile) return;
    auto* ASC = Character->GetICAbilitySystem();
    const auto* Set = Profile->FindSet(ASC ? ASC->GetEquippedWeaponFamily() : Profile->DefaultFamily);
    if (!Set) return;
    Locomotion = Set->Locomotion; Jump = Set->Jump; Sleep = Profile->Sleep;
    Speed = Character->GetVelocity().Size2D(); bAirborne = Character->GetCharacterMovement()->IsFalling(); bSleeping = false;
    FTransform BedAnchor;
    const FVector Position=Character->GetActorLocation();
    const float Distance=bHasPreviousPosition?FVector::Dist2D(Position,PreviousPosition):0;
    if(bHasPreviousPosition&&FVector::DistSquared(Position,PreviousPosition)>FMath::Square(1000.f))
        Character->GetMesh()->ResetAnimInstanceDynamics(ETeleportType::ResetPhysics);
    if(DeltaSeconds>0 && Distance<1000)Speed=FMath::Max(Speed,Distance/DeltaSeconds);
    PreviousPosition=Position;bHasPreviousPosition=true;
    if (auto* NPC = Cast<AICNPCBase>(Character))
    {
        auto* GI = Character->GetGameInstance();
        auto* Definitions = GI ? GI->GetSubsystem<UICDefinitionSubsystem>() : nullptr;
        auto* Time = GI ? GI->GetSubsystem<UICTimeSubsystem>() : nullptr;
        auto* State = GI ? GI->GetSubsystem<UICWorldStateSubsystem>() : nullptr;
        const auto* Stored = State ? State->GetStoredCharacter(Character->GetCharacterID()) : nullptr;
        FICDefinitionResult Result;
        if (Definitions && Time && Stored && Stored->Mode == EICCharacterMode::Scheduled)
            if (const auto* Schedule = Definitions->Find<FICScheduleRow>(NPC->GetNPCDefinition().ScheduleID, Result))
                for (const auto& Segment : Schedule->Segments)
                {
                    const int32 Minute = int32(Time->GetClock().GameMinute) % 1440;
                    if (Minute >= Segment.StartGameMinute && Minute < Segment.EndGameMinute)
                    {
                        FTransform Destination;auto* Spawns=Character->GetWorld()->GetSubsystem<UICSpawnSubsystem>();
                        bSleeping = Segment.Activity == EICScheduleActivity::Sleep && Speed < 5.f && !bAirborne
                            && Spawns && Spawns->ResolveLoadedAnchor(Segment.DestinationAnchorID,Destination)
                            && FVector::DistSquared2D(Position,Destination.GetLocation())<=FMath::Square(150.f);
                        if(bSleeping)BedAnchor=Destination;
                        break;
                    }
                }
    }
    Character->UpdatePresentationTransform(Set->GroundOffsetCm,bSleeping?&BedAnchor:nullptr);
    if (ASC)
    {
        const float HP = ASC->GetNumericAttribute(UICCombatAttributeSet::GetHealthAttribute());
        if (PreviousHP >= 0 && HP < PreviousHP && HP > 0 && Set->Hit) Montage_Play(Set->Hit);
        PreviousHP = HP;
    }
}
