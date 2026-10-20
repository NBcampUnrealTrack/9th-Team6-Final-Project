#include "NPC/Character/ICNPCBase.h"
#include "AI/ICAIController.h"
#include "Combat/GAS/ICAbilitySystemComponent.h"
#include "Data/Definitions/ICDefinitionSubsystem.h"
#include "Data/Rows/NPC/ICServiceRow.h"
#include "World/Spawn/ICSpawnSubsystem.h"
#include "World/State/ICWorldStateSubsystem.h"
#include "Time/ICTimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "StateTree.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Debug/ICLog.h"
#include "Core/Controller/ICPlayerController.h"
AICNPCBase::AICNPCBase()
{
    IC_TRACE_FUNCTION();
    AIControllerClass=AICAIController::StaticClass();
    AutoPossessAI=EAutoPossessAI::Disabled;

    auto* Movement=GetCharacterMovement();
    Movement->MaxWalkSpeed=ScheduleMoveSpeed;

    Movement->bUseRVOAvoidance=true;
    Movement->AvoidanceConsiderationRadius=300.f;
    Movement->AvoidanceWeight=.5f;
}
void AICNPCBase::BeginPlay() { IC_TRACE_FUNCTION();Super::BeginPlay();if(!bInitialized&&!NPCDefinitionID.IsNone())InitializeNPC(NPCDefinitionID); }
bool AICNPCBase::InitializeNPC(FName ID)
{
    IC_TRACE_FUNCTION();auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;FICDefinitionResult R;
    const auto* NPC=D?D->Find<FICNPCRow>(ID,R):nullptr;if(!NPC||!WorldState())return false;
    const auto Kind=static_cast<EICCharacterKind>(static_cast<int32>(NPC->Type)+1);
    if(!WorldState()->EnsureCharacter(ID,Kind))return false;


    auto State=WorldState()->GetSnapshot(ID);const FName AnchorID=!State.CurrentAnchorID.IsNone()?State.CurrentAnchorID:NPC->InitialAnchorID;
    if(!AnchorID.IsNone())if(auto* Spawns=GetWorld()->GetSubsystem<UICSpawnSubsystem>())
    {
        FTransform AnchorTransform;
        if(Spawns->ResolveLoadedAnchor(AnchorID,AnchorTransform))
        {
            State.CurrentAnchorID=AnchorID;State.Position=AnchorTransform.GetLocation();State.Rotation=AnchorTransform.Rotator();
            WorldState()->SetCharacterState(State);SetActorLocationAndRotation(State.Position,State.Rotation);
        }
    }
    Definition=*NPC;NPCDefinitionID=ID;GetCharacterMovement()->MaxWalkSpeed=ScheduleMoveSpeed;if(Kind==EICCharacterKind::Recruitable)EnableCombat(true,false);
    if(!InitializeState(ID))return false;
    if(auto* CharacterMesh=Definition.Mesh.LoadSynchronous())GetMesh()->SetSkeletalMesh(CharacterMesh);
    if(auto* Anim=Definition.AnimationClass.LoadSynchronous())GetMesh()->SetAnimInstanceClass(Anim);
    bInitialized=true;RefreshExecution();return true;
}
void AICNPCBase::RefreshExecution()
{
    IC_TRACE_FUNCTION();if(!bInitialized||!WorldState())return;const auto S=WorldState()->GetSnapshot(CharacterID);
    if(S.IsDowned()){if(auto* AI=Cast<AICAIController>(GetController()))AI->StopExecution();return;}
    GetWorldTimerManager().ClearTimer(DefeatTimer);SetActorEnableCollision(true);
    if(!GetController())SpawnDefaultController();if(auto* AI=Cast<AICAIController>(GetController()))AI->StartExecution(Cast<UStateTree>(Definition.ScheduleStateTree.LoadSynchronous()),nullptr);
}
void AICNPCBase::HandleZeroHP()
{
    IC_TRACE_FUNCTION();if(bZeroHPHandled)return;EndInteraction();if(auto* AI=Cast<AICAIController>(GetController()))AI->StopExecution();

    SetActorEnableCollision(false);GetWorldTimerManager().SetTimer(DefeatTimer,this,&AActor::K2_DestroyActor,1.f,false);
    Super::HandleZeroHP();
}
void AICNPCBase::HoldDownedActor()
{ IC_TRACE_FUNCTION();GetWorldTimerManager().ClearTimer(DefeatTimer);SetActorEnableCollision(false); }
void AICNPCBase::StoredCharacterChanged(FName ID)
{
    IC_TRACE_FUNCTION();const auto Previous=AbilitySystem?AbilitySystem->Capture():FICCharacterState();
    Super::StoredCharacterChanged(ID);
    if(ID==CharacterID&&AbilitySystem){const auto Current=AbilitySystem->Capture();if(Previous.Mode!=Current.Mode||Previous.LifeState!=Current.LifeState)RefreshExecution();}
}
void AICNPCBase::RestoreFromWorldState_Implementation(const FICWorldState& Candidate)
{ IC_TRACE_FUNCTION();Super::RestoreFromWorldState_Implementation(Candidate);RefreshExecution(); }
TArray<FName> AICNPCBase::GetAvailableServices() const
{
    IC_TRACE_FUNCTION();if(!WorldState()||WorldState()->GetSnapshot(CharacterID).Mode!=EICCharacterMode::Scheduled)return {};
    auto* D=GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>();auto* T=GetGameInstance()->GetSubsystem<UICTimeSubsystem>();FICDefinitionResult R;
    const auto* Schedule=D?D->Find<FICScheduleRow>(Definition.ScheduleID,R):nullptr;if(!Schedule||!T)return {};
    const int32 Minute=int32(FMath::FloorToDouble(T->GetClock().GameMinute))%1440;
    for(const auto& Segment:Schedule->Segments)if(Minute>=Segment.StartGameMinute&&Minute<Segment.EndGameMinute&&Segment.Activity==EICScheduleActivity::Work)return Segment.ServiceIDs;
    return {};
}
bool AICNPCBase::TryInteract(AICPlayerController* Requester)
{
    IC_TRACE_FUNCTION();
    if(!bInitialized||!Requester||bInteracting||!WorldState())return false;
    const auto State=WorldState()->GetSnapshot(CharacterID);
    if(State.IsDowned()||State.Mode!=EICCharacterMode::Scheduled)
    {UE_LOG(LogICInteraction,Verbose,TEXT("[NPC.InteractRejected] NPC=%s Mode=%d Downed=%d"),*CharacterID.ToString(),static_cast<int32>(State.Mode),State.IsDowned()?1:0);return false;}
    const TArray<FName> Services=GetAvailableServices();
    const FName ServiceID=Services.Num()>0?Services[0]:NAME_None;
    bInteracting=true;
    if(auto* AI=Cast<AICAIController>(GetController()))AI->SetInteractionHold(true,Requester->GetPawn());
    UE_LOG(LogICInteraction,Log,TEXT("[NPC.Interact] NPC=%s Service=%s"),*CharacterID.ToString(),*ServiceID.ToString());
    OnInteractionStarted.Broadcast(this,Requester,ServiceID);
    return true;
}
void AICNPCBase::EndInteraction()
{
    IC_TRACE_FUNCTION();
    if(!bInteracting)return;
    bInteracting=false;
    if(auto* AI=Cast<AICAIController>(GetController()))
    {
        AI->SetInteractionHold(false,nullptr);
        AI->RestoreScheduleFacing();
    }
    UE_LOG(LogICInteraction,Log,TEXT("[NPC.InteractEnd] NPC=%s"),*CharacterID.ToString());
}
FText AICNPCBase::GetInteractionText() const
{
    IC_TRACE_FUNCTION();
    return FText::FromString(TEXT("F: 대화하기"));
}
TArray<FICNPCInteractionOption> AICNPCBase::GetInteractionOptions() const
{
    IC_TRACE_FUNCTION();
    TArray<FICNPCInteractionOption> Options;
    auto Add=[&Options](EICNPCInteractionOption Kind,const TCHAR* Label,FName ServiceID,bool bEnabled)
    { FICNPCInteractionOption Option;Option.Option=Kind;Option.Label=FText::FromString(Label);Option.ServiceID=ServiceID;Option.bEnabled=bEnabled;Options.Add(Option); };
    Add(EICNPCInteractionOption::Talk,TEXT("대화하기"),NAME_None,true);
    Add(EICNPCInteractionOption::Gift,TEXT("선물하기"),NAME_None,true);

    auto* D=GetGameInstance()?GetGameInstance()->GetSubsystem<UICDefinitionSubsystem>():nullptr;
    if(D)
    {
        const TArray<FName> Working=GetAvailableServices();FICDefinitionResult R;
        for(const FName ID:Definition.ServiceIDs)
        {
            const auto* Service=D->Find<FICServiceRow>(ID,R);
            if(Service&&Service->Type==EICServiceType::Shop){Add(EICNPCInteractionOption::Trade,TEXT("거래하기"),ID,Working.Contains(ID));break;}
        }
    }
    Add(EICNPCInteractionOption::End,TEXT("대화 끝내기"),NAME_None,true);
    return Options;
}
