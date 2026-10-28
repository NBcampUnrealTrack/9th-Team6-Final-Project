#include "AI/ICAIController.h"
#include "NPC/Character/ICNPCBase.h"
#include "Character/Base/ICCharacterBase.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Debug/ICLog.h"

void AICAIController::ResetCompanionFollow()
{
    bRegrouping=false;bDirectFollowing=false;LastFollowProgressAt=LastFollowRequestAt=-1;
    LastFollowProgressPosition=LastFollowRequestGoal=FVector::ZeroVector;
}

bool AICAIController::RequestDirectCompanionMove(AICNPCBase* NPC,const FVector& Goal)
{
    // Outside navigation coverage, only walk a corridor with clear body space
    // and continuous walkable ground. Walls, gaps and steep drops reject it.
    auto* Capsule=NPC->GetCapsuleComponent();auto* Movement=NPC->GetCharacterMovement();
    const FVector Start=NPC->GetActorLocation();const float Half=Capsule->GetScaledCapsuleHalfHeight();
    const int32 Steps=FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(Start,Goal)/100.f),1,40);
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams Query;Query.AddIgnoredActor(NPC);Query.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
    float FloorZ=Start.Z-Half;FVector End=Goal;
    const float MaxRise=FMath::Max(Movement->MaxStepHeight+5.f,float(FVector::Dist2D(Start,Goal)/Steps));
    for(int32 Index=0;Index<=Steps;++Index)
    {
        const FVector At=FMath::Lerp(Start,Goal,float(Index)/Steps);FHitResult Floor;
        if(!GetWorld()->LineTraceSingleByObjectType(Floor,At+FVector(0,0,100),At-FVector(0,0,Half+150),Objects,Query)
            ||Floor.ImpactNormal.Z<Movement->GetWalkableFloorZ()||FMath::Abs(Floor.ImpactPoint.Z-FloorZ)>MaxRise)return false;
        FloorZ=Floor.ImpactPoint.Z;if(Index==Steps)End=Floor.ImpactPoint;
    }
    const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius()*.9f,Half*.85f);
    if(GetWorld()->SweepTestByObjectType(Start,End+FVector(0,0,Half+2),FQuat::Identity,Objects,Shape,Query))return false;
    return MoveToLocation(End,40.f,false,false,false,false,nullptr,false)!=EPathFollowingRequestResult::Failed;
}

void AICAIController::DecideCompanionFollow(AICNPCBase* NPC)
{
    auto* GI=GetGameInstance();auto* State=GI?GI->GetSubsystem<UICWorldStateSubsystem>():nullptr;
    auto* Time=GI?GI->GetSubsystem<UICTimeSubsystem>():nullptr;auto* ASC=NPC->GetICAbilitySystem();
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    const auto* Member=State?State->GetStoredCharacter(NPC->GetCharacterID()):nullptr;
    if(!Member||!Player||!Time||!ASC||Member->PartySlot<0||Member->IsDowned()||bInteractionHold)return;
    auto* Definitions=GI->GetSubsystem<UICDefinitionSubsystem>();FICDefinitionResult Lookup;FICPartySettingsRow Defaults;
    const auto* Rules=Definitions?Definitions->Find<FICPartySettingsRow>(TEXT("CFG_PARTY_001"),Lookup):nullptr;if(!Rules)Rules=&Defaults;
    const double Now=Time->GetClock().PlaySecond;
    auto* Path=GetPathFollowingComponent();
    // Reissuing MoveTo while a smart/simple link is in progress can cancel traversal.
    if(Path&&(Path->IsFollowingNavLink()||Path->GetCurrentCustomLinkOb()))
    {LastFollowProgressAt=Now;LastFollowProgressPosition=NPC->GetActorLocation();return;}
    if(TryRecoverCompanion())return;
    const double PlayerDistance=FVector::Dist2D(NPC->GetActorLocation(),Player->GetActorLocation());
    if(PlayerDistance>Rules->ChaseDistanceCm)
    {
        if(!bRegrouping)ASC->CancelAction();
        bRegrouping=true;Target.Reset();OrderedTarget.Reset();ClearFocus(EAIFocusPriority::Gameplay);
    }
    else if(bRegrouping&&PlayerDistance<350.)bRegrouping=false;
    if(!ASC->CanAcceptMovementInput())
    {StopMovement();LastFollowProgressAt=Now;LastFollowProgressPosition=NPC->GetActorLocation();return;}
    if(!bRegrouping&&DecideCompanionCombat(NPC))
    {LastFollowProgressAt=-1;LastFollowRequestAt=-1;bDirectFollowing=false;return;}
    ClearFocus(EAIFocusPriority::Gameplay);
    const int32 Slot=FMath::Clamp(Member->PartySlot,0,2);
    const FVector Offset=Rules->FormationOffsetsCm.IsValidIndex(Slot)?Rules->FormationOffsetsCm[Slot]:FVector(Slot==1?-180:-140,(Slot-1)*110,0);
    const FVector Goal=Player->GetActorLocation()+Player->GetActorRotation().RotateVector(Offset);
    if(FVector::DistSquared(NPC->GetActorLocation(),Goal)<=FMath::Square(60.f))
    {
        StopMovement();NPC->GetCharacterMovement()->StopMovementImmediately();
        NPC->GetCharacterMovement()->bOrientRotationToMovement=false;
        SetControlRotation(Player->GetActorRotation());NPC->SetActorRotation(Player->GetActorRotation());
        LastFollowProgressAt=Now;LastFollowRequestAt=-1;bDirectFollowing=false;return;
    }
    NPC->GetCharacterMovement()->bOrientRotationToMovement=true;
    if(LastFollowProgressAt<0||FVector::DistSquared2D(LastFollowProgressPosition,NPC->GetActorLocation())>=FMath::Square(25.f))
    {LastFollowProgressAt=Now;LastFollowProgressPosition=NPC->GetActorLocation();}
    const double StalledFor=Now-LastFollowProgressAt;
    const bool RequestDue=!Path||Path->GetStatus()==EPathFollowingStatus::Idle||LastFollowRequestAt<0
        ||(Now-LastFollowRequestAt>=.1&&(FVector::DistSquared(LastFollowRequestGoal,Goal)>FMath::Square(40.f)||StalledFor>=.5));
    if(RequestDue)
    {
        LastFollowRequestAt=Now;LastFollowRequestGoal=Goal;const bool WasDirect=bDirectFollowing;
        bool Direct=(WasDirect||StalledFor>=.5)&&RequestDirectCompanionMove(NPC,Goal);
        if(!Direct)
        {
            const auto Result=MoveToLocation(Goal,40.f,false,true,true,false,nullptr,true);
            if(Result==EPathFollowingRequestResult::AlreadyAtGoal)
            {LastFollowProgressAt=Now;LastFollowProgressPosition=NPC->GetActorLocation();return;}
            if(Result==EPathFollowingRequestResult::Failed)Direct=RequestDirectCompanionMove(NPC,Goal);
        }
        bDirectFollowing=Direct;
        if(Direct&&!WasDirect)
        {
            LastFollowProgressAt=Now;LastFollowProgressPosition=NPC->GetActorLocation();
            UE_LOG(LogICCore,Verbose,TEXT("[Party.DirectFollow] NPC=%s Goal=%s"),*NPC->GetCharacterID().ToString(),*Goal.ToString());
        }
    }
    if(Rules->PathFailurePlaySeconds>0&&Now-LastFollowProgressAt>=Rules->PathFailurePlaySeconds)
        if(TryRecoverCompanion(true))return;
}
