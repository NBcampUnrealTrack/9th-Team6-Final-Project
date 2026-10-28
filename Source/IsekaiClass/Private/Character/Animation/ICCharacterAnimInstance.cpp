#include "Character/Animation/ICCharacterAnimInstance.h"
#include "Character/Animation/ICCharacterAnimationProfile.h"
#include "Character/Base/ICCharacterBase.h"
#include "Player/Character/ICPlayerCharacter.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Combat/GAS/Attributes/ICAttributeSets.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "NPC/Character/ICNPCBase.h"
#include "Life/Production/ICPlayerLifeComponent.h"
#include "Time/ICTimeSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
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
    FAnimNode_BlendSpacePlayer_Standalone Ground, CombatGround;
    FAnimNode_TwoWayBlend CombatBlend;
    FAnimNode_SequencePlayer_Standalone Air, Bed, Sprint, Launch, Fall, Landing;
    FAnimNode_TwoWayBlend AirBlend, SleepBlend, SprintBlend, LaunchBlend, FallBlend, LandingBlend;
    EICAirAnimationState PreviousAirState=EICAirAnimationState::Grounded;
    FAnimNode_Slot Slot;

    explicit FICCharacterAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
    {
        CombatBlend.A.SetLinkNode(&Ground); CombatBlend.B.SetLinkNode(&CombatGround);
        SprintBlend.A.SetLinkNode(&CombatBlend); SprintBlend.B.SetLinkNode(&Sprint);
        LaunchBlend.A.SetLinkNode(&Air); LaunchBlend.B.SetLinkNode(&Launch);
        FallBlend.A.SetLinkNode(&LaunchBlend); FallBlend.B.SetLinkNode(&Fall);
        AirBlend.A.SetLinkNode(&SprintBlend); AirBlend.B.SetLinkNode(&FallBlend);
        LandingBlend.A.SetLinkNode(&AirBlend); LandingBlend.B.SetLinkNode(&Landing);
        SleepBlend.A.SetLinkNode(&LandingBlend); SleepBlend.B.SetLinkNode(&Bed);
        Slot.Source.SetLinkNode(&SleepBlend); Slot.SlotName = TEXT("DefaultSlot");
        Air.SetLoopAnimation(true); Bed.SetLoopAnimation(true);
        Sprint.SetLoopAnimation(true);
        Launch.SetLoopAnimation(false);Fall.SetLoopAnimation(true);Landing.SetLoopAnimation(false);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Slot; }
    virtual void GetCustomNodes(TArray<FAnimNode_Base*>& Out) override
    { Out = {&Ground, &CombatGround, &CombatBlend, &Air, &Bed, &Sprint, &Launch, &Fall, &Landing, &LaunchBlend, &FallBlend, &LandingBlend, &SprintBlend, &AirBlend, &SleepBlend, &Slot}; }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* IC = CastChecked<UICCharacterAnimInstance>(Instance);
        Ground.SetBlendSpace(IC->Locomotion); Ground.SetPosition(FVector(IC->Speed,0,0));
        CombatGround.SetBlendSpace(IC->CombatLocomotion?IC->CombatLocomotion:IC->Locomotion);
        CombatGround.SetPosition(IC->LocalMoveVelocity);
        CombatBlend.Alpha=FMath::FInterpTo(CombatBlend.Alpha,IC->bCombatMovement&&IC->CombatLocomotion?1.f:0.f,DeltaSeconds,10.f);
        Air.SetSequence(IC->Jump); Bed.SetSequence(IC->Sleep);
        Launch.SetSequence(IC->JumpStart?IC->JumpStart:IC->Jump);
        Fall.SetSequence(IC->FallLoop?IC->FallLoop:IC->Jump);
        Landing.SetSequence(IC->Land);
        if(PreviousAirState!=IC->AirState)
        {
            if(IC->AirState==EICAirAnimationState::JumpStart)Launch.SetAccumulatedTime(0);
            if(IC->AirState==EICAirAnimationState::Rising)Air.SetAccumulatedTime(0);
            if(IC->AirState==EICAirAnimationState::Falling)Fall.SetAccumulatedTime(0);
            if(IC->AirState==EICAirAnimationState::Landing)Landing.SetAccumulatedTime(0);
            PreviousAirState=IC->AirState;
        }
        LaunchBlend.Alpha=FMath::FInterpTo(LaunchBlend.Alpha,IC->AirState==EICAirAnimationState::JumpStart?1.f:0.f,DeltaSeconds,20.f);
        FallBlend.Alpha=FMath::FInterpTo(FallBlend.Alpha,IC->AirState==EICAirAnimationState::Falling?1.f:0.f,DeltaSeconds,12.f);
        LandingBlend.Alpha=FMath::FInterpTo(LandingBlend.Alpha,IC->AirState==EICAirAnimationState::Landing&&IC->Land?1.f:0.f,DeltaSeconds,20.f);
        Sprint.SetSequence(IC->Sprint);
        SprintBlend.Alpha = FMath::FInterpTo(SprintBlend.Alpha, IC->bSprinting && IC->Sprint ? 1.f : 0.f, DeltaSeconds, 10.f);
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
    JumpStart=Set->JumpStart;FallLoop=Set->FallLoop;Land=Set->Land;
    const float VerticalSpeed=Character->GetVelocity().Z;
    const bool Teleported=bHasPreviousPosition&&FVector::DistSquared(Character->GetActorLocation(),PreviousPosition)>FMath::Square(1000.f);
    auto ChangeAirState=[this](EICAirAnimationState State){if(AirState!=State){AirState=State;AirStateSeconds=0;}};
    AirStateSeconds+=FMath::Max(0.f,DeltaSeconds);
    if(!bAirStateInitialized||Teleported)
    {
        AirState=bAirborne?(VerticalSpeed>0?EICAirAnimationState::Rising:EICAirAnimationState::Falling):EICAirAnimationState::Grounded;
        AirStateSeconds=0;AirborneSeconds=0;bWasAirborne=bAirborne;bAirStateInitialized=true;
    }
    else if(bAirborne)
    {
        if(!bWasAirborne){AirborneSeconds=0;ChangeAirState(VerticalSpeed>10&&JumpStart?EICAirAnimationState::JumpStart:EICAirAnimationState::Falling);}
        AirborneSeconds+=FMath::Max(0.f,DeltaSeconds);
        if(VerticalSpeed<=0)ChangeAirState(EICAirAnimationState::Falling);
        else if(AirState==EICAirAnimationState::JumpStart&&AirStateSeconds>=FMath::Min(.2f,JumpStart?JumpStart->GetPlayLength():0.f))ChangeAirState(EICAirAnimationState::Rising);
    }
    else if(bWasAirborne)
        ChangeAirState(Land&&AirborneSeconds>.08f&&Character->GetCharacterMovement()->IsMovingOnGround()?EICAirAnimationState::Landing:EICAirAnimationState::Grounded);
    else if(AirState==EICAirAnimationState::Landing&&AirStateSeconds>=FMath::Min(Speed>10?.12f:.3f,Land?Land->GetPlayLength():0.f))ChangeAirState(EICAirAnimationState::Grounded);
    bWasAirborne=bAirborne;
    Sprint = Set->Sprint;
    const auto* Player = Cast<AICPlayerCharacter>(Character);
    CombatLocomotion=Set->CombatLocomotion;
    bCombatMovement=Player&&ASC&&ASC->HasRecentCombat();
    LocalMoveVelocity=Character->GetActorRotation().UnrotateVector(Character->GetVelocity());
    LocalMoveVelocity.Z=0;
    LocalMoveVelocity=LocalMoveVelocity.GetClampedToMaxSize(450.f);
    bSprinting = Player && Player->IsSprinting() && Speed > 10.f;
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
    const bool PlayerInBed=Player&&Player->LifeActions&&Player->LifeActions->IsSleeping();
    if(PlayerInBed){bSleeping=Player->LifeActions->IsSleepPoseActive();BedAnchor=Player->LifeActions->GetSleepAnchor();}
    Character->UpdatePresentationTransform(Set->GroundOffsetCm,bSleeping?&BedAnchor:nullptr);
    if (ASC)
    {
        const float HP = ASC->GetNumericAttribute(UICCombatAttributeSet::GetHealthAttribute());
        if (PreviousHP >= 0 && HP < PreviousHP && HP > 0 && Set->Hit) Montage_Play(Set->Hit);
        PreviousHP = HP;
    }
}
